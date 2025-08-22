// Minimal IRC server (C++98, single poll, non-blocking)
// Usage: ./ircserv <port> <password>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <cerrno>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <poll.h>

static const int BACKLOG = 128;
static const int MAX_EVENTS = 1024;
static const int BUF_SIZE = 4096;

// ---------- Utilities ----------
static void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) flags = 0;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> out;
    std::string cur;
    for (size_t i=0;i<s.size();++i) {
        if (s[i]==delim) { out.push_back(cur); cur.clear(); }
        else cur += s[i];
    }
    out.push_back(cur);
    return out;
}

static std::string trim_crlf(const std::string& s) {
    std::string r = s;
    while (!r.empty() && (r[r.size()-1]=='\r' || r[r.size()-1]=='\n')) r.erase(r.size()-1);
    return r;
}

static std::string nowPrefix(const std::string& nick, const std::string& user) {
    if (!nick.empty() && !user.empty()) return ":" + nick + "!~" + user + "@localhost ";
    return ":server ";
}

static void enqueue(std::string &outbuf, const std::string& line) {
    outbuf += line;
    if (outbuf.size() > 1<<20) outbuf.erase(0, outbuf.size() - (1<<20)); // crude back-pressure
}

// ---------- Client / Channel ----------
struct Client {
    int fd;
    bool authed;        // PASS ok?
    bool registered;    // NICK+USER done?
    std::string passGiven;
    std::string nick;
    std::string user;
    std::string inbuf;
    std::string outbuf;
    Client(): fd(-1), authed(false), registered(false) {}
};

struct Channel {
    std::string name;
    std::string topic;
    std::set<int> members;      // fds
    std::set<int> operators;    // fds
    Channel(): name(), topic() {}
};

// ---------- Server ----------
class IRCServer {
public:
    IRCServer(const std::string& port, const std::string& password)
        : _port(port), _password(password), _listenfd(-1) {}

    bool init() {
        struct addrinfo hints; std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        struct addrinfo* res = 0;
        int rc = getaddrinfo(NULL, _port.c_str(), &hints, &res);
        if (rc != 0) { std::cerr << "getaddrinfo: " << gai_strerror(rc) << "\n"; return false; }

        for (struct addrinfo* p = res; p; p = p->ai_next) {
            int fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (fd < 0) continue;
            int yes = 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#ifdef SO_REUSEPORT
            setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));
#endif
            if (bind(fd, p->ai_addr, p->ai_addrlen) == 0) {
                _listenfd = fd;
                break;
            }
            close(fd);
        }
        freeaddrinfo(res);
        if (_listenfd < 0) { std::cerr << "bind failed\n"; return false; }
        if (listen(_listenfd, BACKLOG) != 0) { std::cerr << "listen failed\n"; return false; }
        setNonBlocking(_listenfd);

        struct pollfd pfd; pfd.fd = _listenfd; pfd.events = POLLIN; pfd.revents = 0;
        _pfds.push_back(pfd);
        return true;
    }

    void run() {
        std::cout << "Listening on port " << _port << " …\n";
        while (true) {
            // set POLLOUT for clients with pending data
            for (size_t i=0;i<_pfds.size();++i) {
                if (_pfds[i].fd == _listenfd) { _pfds[i].events = POLLIN; continue; }
                Client &c = _clients[_pfds[i].fd];
                _pfds[i].events = POLLIN | (c.outbuf.empty()?0:POLLOUT);
            }

            int rc = poll(&_pfds[0], _pfds.size(), 1000);
            if (rc < 0) { if (errno==EINTR) continue; perror("poll"); break; }

            // Accept new connections
            if (_pfds[0].revents & POLLIN) acceptNew();

            // handle clients
            for (size_t i=1;i<_pfds.size();++i) {
                int fd = _pfds[i].fd;
                short re = _pfds[i].revents;
                if (re & (POLLERR|POLLHUP|POLLNVAL)) { disconnect(fd, "poll error"); continue; }
                if (re & POLLIN)  { if (!readFrom(fd)) { disconnect(fd, "read error"); continue; } }
                if (re & POLLOUT) { if (!flushTo(fd))  { disconnect(fd, "write error"); continue; } }
            }
            // compact pollfds (disconnected are marked fd=-1)
            compactPollfds();
        }
    }

private:
    // ---- network helpers
    void acceptNew() {
        for (;;) {
            struct sockaddr_storage ss; socklen_t slen = sizeof(ss);
            int cfd = accept(_listenfd, (struct sockaddr*)&ss, &slen);
            if (cfd < 0) {
                if (errno==EAGAIN || errno==EWOULDBLOCK) break;
                perror("accept"); break;
            }
            setNonBlocking(cfd);
            struct pollfd pfd; pfd.fd = cfd; pfd.events = POLLIN; pfd.revents = 0;
            _pfds.push_back(pfd);
            Client c; c.fd = cfd;
            _clients[cfd] = c;
            // Minimal greeting (RFC-like numeric not required yet)
            enqueue(_clients[cfd].outbuf, ":server NOTICE * :Welcome! Please PASS, NICK, USER.\r\n");
        }
    }

    bool readFrom(int fd) {
        char buf[BUF_SIZE];
        for (;;) {
            ssize_t n = recv(fd, buf, sizeof(buf), 0);
            if (n > 0) {
                _clients[fd].inbuf.append(buf, n);
                // process complete lines ending with \r\n or \n
                processLines(fd);
            } else if (n == 0) {
                return false; // peer closed
            } else {
                if (errno==EAGAIN || errno==EWOULDBLOCK) break;
                return false;
            }
        }
        return true;
    }

    bool flushTo(int fd) {
        Client &c = _clients[fd];
        while (!c.outbuf.empty()) {
            ssize_t n = send(fd, c.outbuf.c_str(), c.outbuf.size(), 0);
            if (n > 0) {
                c.outbuf.erase(0, n);
            } else {
                if (errno==EAGAIN || errno==EWOULDBLOCK) return true;
                return false;
            }
        }
        return true;
    }

    void compactPollfds() {
        std::vector<struct pollfd> np;
        np.reserve(_pfds.size());
        for (size_t i=0;i<_pfds.size();++i) if (_pfds[i].fd!=-1) np.push_back(_pfds[i]);
        _pfds.swap(np);
    }

    void disconnect(int fd, const std::string& why) {
        (void)why;
        // remove from channels
        for (std::map<std::string, Channel>::iterator it = _chans.begin(); it!=_chans.end(); ++it) {
            it->second.members.erase(fd);
            it->second.operators.erase(fd);
        }
        close(fd);
        _clients.erase(fd);
        for (size_t i=0;i<_pfds.size();++i) if (_pfds[i].fd==fd) _pfds[i].fd = -1;
    }

    // ---- IRC parsing/commands (very small subset)
    void processLines(int fd) {
        Client &c = _clients[fd];
        std::string &in = c.inbuf;
        size_t pos;
        while ((pos = in.find('\n')) != std::string::npos) {
            std::string line = trim_crlf(in.substr(0, pos+1));
            in.erase(0, pos+1);
            if (!line.empty()) handleCommand(fd, line);
        }
    }

    // tokenization: command [params ...] [:trailing...]
    static void parseIrc(const std::string& line, std::string& cmd, std::vector<std::string>& params, std::string& trailing) {
        std::string s = line;
        if (!s.empty() && s[0]==':') { // ignore prefix if any
            size_t sp = s.find(' ');
            s = (sp==std::string::npos) ? "" : s.substr(sp+1);
        }
        size_t colon = s.find(" :");
        if (colon != std::string::npos) {
            trailing = s.substr(colon+2);
            s = s.substr(0, colon);
        } else trailing.clear();
        std::vector<std::string> p = split(s, ' ');
        cmd = p.empty() ? "" : p[0];
        for (size_t i=1;i<p.size();++i) if (!p[i].empty()) params.push_back(p[i]);
    }

    void requireRegistered(int fd) {
        Client &c = _clients[fd];
        if (!c.registered) {
            enqueue(c.outbuf, ":server NOTICE * :You are not registered. Use PASS/NICK/USER.\r\n");
        }
    }

    void maybeFinishRegistration(int fd) {
        Client &c = _clients[fd];
        if (c.authed && !c.nick.empty() && !c.user.empty() && !c.registered) {
            c.registered = true;
            enqueue(c.outbuf, ":server 001 " + c.nick + " :Welcome to the tiny IRC!\r\n");
        }
    }

    void handleCommand(int fd, const std::string& line) {
        std::string cmd, trailing; std::vector<std::string> params;
        parseIrc(line, cmd, params, trailing);
        for (size_t i=0;i<cmd.size();++i) cmd[i] = std::toupper(cmd[i]);

        Client &c = _clients[fd];
        // PASS
        if (cmd == "PASS") {
            if (params.empty()) { enqueue(c.outbuf, ":server 461 * PASS :Not enough parameters\r\n"); return; }
            c.passGiven = params[0];
            c.authed = (c.passGiven == _password);
            enqueue(c.outbuf, c.authed ? ":server NOTICE * :Password accepted\r\n"
                                       : ":server 464 * :Password required/incorrect\r\n");
            maybeFinishRegistration(fd);
            return;
        }
        // NICK
        if (cmd == "NICK") {
            if (params.empty()) { enqueue(c.outbuf, ":server 431 * :No nickname given\r\n"); return; }
            std::string nn = params[0];
            if (_nickInUse(nn, fd)) { enqueue(c.outbuf, ":server 433 * " + nn + " :Nickname is already in use\r\n"); return; }
            c.nick = nn;
            maybeFinishRegistration(fd);
            return;
        }
        // USER
        if (cmd == "USER") {
            if (params.size() < 4 && trailing.empty()) { enqueue(c.outbuf, ":server 461 * USER :Not enough parameters\r\n"); return; }
            c.user = params.empty() ? "user" : params[0];
            maybeFinishRegistration(fd);
            return;
        }
        // PING
        if (cmd == "PING") {
            std::string token = params.empty()? trailing : params[0];
            enqueue(c.outbuf, ":server PONG :" + token + "\r\n");
            return;
        }
        // QUIT
        if (cmd == "QUIT") {
            disconnect(fd, "quit");
            return;
        }

        // From here: must be registered
        if (!c.registered) { requireRegistered(fd); return; }
        // JOIN #chan
        if (cmd == "JOIN") {
            if (params.empty() && trailing.empty()) { enqueue(c.outbuf, ":server 461 " + c.nick + " JOIN :Not enough parameters\r\n"); return; }
            std::string chan = params.empty()? trailing : params[0];
            if (chan.empty() || chan[0] != '#') chan = "#" + chan;
            Channel &ch = _chans[chan];
            ch.name = chan;
            ch.members.insert(fd);
            // First member becomes operator
            if (ch.operators.empty()) ch.operators.insert(fd);
            broadcast(ch, nowPrefix(c.nick,c.user) + "JOIN " + chan + "\r\n");
            if (!ch.topic.empty()) enqueue(c.outbuf, ":server 332 " + c.nick + " " + chan + " :" + ch.topic + "\r\n");
            enqueue(c.outbuf, ":server 353 " + c.nick + " = " + chan + " :" + nickList(chan) + "\r\n");
            enqueue(c.outbuf, ":server 366 " + c.nick + " " + chan + " :End of /NAMES list.\r\n");
            return;
        }
        // PRIVMSG target :message
        if (cmd == "PRIVMSG") {
            if (params.empty()) { enqueue(c.outbuf, ":server 411 " + c.nick + " :No recipient given (PRIVMSG)\r\n"); return; }
            std::string target = params[0];
            std::string msg = trailing;
            if (msg.empty() && params.size()>=2) msg = params[1];
            if (msg.empty()) { enqueue(c.outbuf, ":server 412 " + c.nick + " :No text to send\r\n"); return; }
            if (target.size()>0 && target[0]=='#') {
                std::map<std::string, Channel>::iterator it = _chans.find(target);
                if (it==_chans.end()) { enqueue(c.outbuf, ":server 403 " + target + " :No such channel\r\n"); return; }
                Channel &ch = it->second;
                if (ch.members.find(fd)==ch.members.end()) { enqueue(c.outbuf, ":server 442 " + target + " :You're not on that channel\r\n"); return; }
                std::string line = nowPrefix(c.nick,c.user) + "PRIVMSG " + target + " :" + msg + "\r\n";
                for (std::set<int>::iterator m=ch.members.begin(); m!=ch.members.end(); ++m) if (*m!=fd) enqueue(_clients[*m].outbuf, line);
            } else {
                // user-to-user by nickname
                int tofd = nickToFd(target);
                if (tofd<0) { enqueue(c.outbuf, ":server 401 " + target + " :No such nick\r\n"); return; }
                std::string line = nowPrefix(c.nick,c.user) + "PRIVMSG " + target + " :" + msg + "\r\n";
                enqueue(_clients[tofd].outbuf, line);
            }
            return;
        }
        // TOPIC #chan [:text]
        if (cmd == "TOPIC") {
            if (params.empty()) { enqueue(c.outbuf, ":server 461 " + c.nick + " TOPIC :Not enough parameters\r\n"); return; }
            std::string chan = params[0];
            std::map<std::string, Channel>::iterator it = _chans.find(chan);
            if (it==_chans.end()) { enqueue(c.outbuf, ":server 403 " + chan + " :No such channel\r\n"); return; }
            Channel &ch = it->second;
            if (trailing.empty()) {
                enqueue(c.outbuf, ch.topic.empty()? ":server 331 " + c.nick + " " + chan + " :No topic is set\r\n"
                                                   : ":server 332 " + c.nick + " " + chan + " :" + ch.topic + "\r\n");
            } else {
                if (ch.operators.find(fd)==ch.operators.end()) { enqueue(c.outbuf, ":server 482 " + chan + " :You're not channel operator\r\n"); return; }
                ch.topic = trailing;
                broadcast(ch, nowPrefix(c.nick,c.user) + "TOPIC " + chan + " :" + ch.topic + "\r\n");
            }
            return;
        }
        // KICK #chan nick
        if (cmd == "KICK") {
            if (params.size()<2) { enqueue(c.outbuf, ":server 461 " + c.nick + " KICK :Not enough parameters\r\n"); return; }
            std::string chan = params[0], nn = params[1];
            std::map<std::string, Channel>::iterator it = _chans.find(chan);
            if (it==_chans.end()) { enqueue(c.outbuf, ":server 403 " + chan + " :No such channel\r\n"); return; }
            Channel &ch = it->second;
            if (ch.operators.find(fd)==ch.operators.end()) { enqueue(c.outbuf, ":server 482 " + chan + " :You're not channel operator\r\n"); return; }
            int vict = nickToFd(nn);
            if (vict<0 || ch.members.find(vict)==ch.members.end()) { enqueue(c.outbuf, ":server 441 " + nn + " " + chan + " :They aren't on that channel\r\n"); return; }
            broadcast(ch, nowPrefix(c.nick,c.user) + "KICK " + chan + " " + nn + " :" + (trailing.empty()?"":trailing) + "\r\n");
            ch.members.erase(vict);
        }
        // MODE #chan +o/-o nick  (only o & l/i/t/k flags would be needed; keep minimal here)
        else if (cmd == "MODE") {
            enqueue(c.outbuf, ":server NOTICE " + c.nick + " :MODE parsing minimal in this demo\r\n");
        }
        else {
            enqueue(c.outbuf, ":server 421 " + c.nick + " " + cmd + " :Unknown command\r\n");
        }
    }

    void broadcast(Channel& ch, const std::string& line) {
        for (std::set<int>::iterator m=ch.members.begin(); m!=ch.members.end(); ++m) {
            enqueue(_clients[*m].outbuf, line);
        }
    }

    std::string nickList(const std::string& chan) {
        std::string s;
        Channel &ch = _chans[chan];
        for (std::set<int>::iterator m=ch.members.begin(); m!=ch.members.end(); ++m) {
            const Client &c = _clients[*m];
            if (s.size()) s += " ";
            bool isop = (ch.operators.find(*m)!=ch.operators.end());
            s += (isop?"@":"") + (c.nick.empty()?"?":c.nick);
        }
        return s;
    }

    bool _nickInUse(const std::string& nn, int self) const {
        for (std::map<int, Client>::const_iterator it=_clients.begin(); it!=_clients.end(); ++it) {
            if (it->first==self) continue;
            if (it->second.nick == nn) return true;
        }
        return false;
    }

    int nickToFd(const std::string& nn) const {
        for (std::map<int, Client>::const_iterator it=_clients.begin(); it!=_clients.end(); ++it)
            if (it->second.nick == nn) return it->first;
        return -1;
    }

private:
    std::string _port, _password;
    int _listenfd;
    std::vector<struct pollfd> _pfds;                 // [0] is listen fd
    std::map<int, Client> _clients;                   // fd -> Client
    std::map<std::string, Channel> _chans;            // #name -> Channel
};

// ---------- main ----------
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>\n";
        return 1;
    }
    IRCServer s(argv[1], argv[2]);
    if (!s.init()) return 1;
    s.run();
    return 0;
}
