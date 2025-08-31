#ifndef SERVER_HPP

# define SERVER_HPP

# include <iostream>
# include <cstring>
# include <cstdlib>
# include <unistd.h>
# include <fcntl.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <poll.h>
# include <errno.h>
# include <sstream>
# include <vector>
# include <map>
# include <set>
# include <string>
# include <cstdio>
# include <cerrno>
# include <exception>
# include <algorithm>
# include <cctype>
# include "Client.hpp"

# define BACKLOG 10
# define BUF_SIZE 1024

//class Channel;

class Server {
	private:
	    int port;
	    std::string password;
	    //std::map<std::string, Channel*> channels;
	    int serverFd;
		struct pollfd pfds[BACKLOG + 1];
		int num_of_pfd;
	    std::vector<Client *> clients;
	
	public:
	    Server();
	    ~Server();
	
		void commandParser(Client &client, std::string &message);
		void handleClient(int index);
		bool handleClientPollout(int index);
		void initServer(struct sockaddr_in &hints, int port);
	    void start(int port, const char *pass);
	    void removeClient(int index);
		void commandHandler(std::string cmd, std::vector<std::string> params, Client &client);
		void checkRegistration(Client &client);
		bool nicknameCheck(std::string nickname);
		
		
	    //void acceptClient();
	    //void handleClientMessage(int client_fd, const std::string& message);
};

void parseIrc(const std::string& line, std::string& cmd, std::vector<std::string>& params, std::string& trailing);
void enqueue(std::string &outbuf, const std::string& line);

#endif
