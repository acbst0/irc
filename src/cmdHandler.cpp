#include "../include/Server.hpp"


void Server::checkRegistration(Client &client)
{
    // PASS doğrulanmadan kayıt tamamlanmasın
    if (!client.getNick().empty() && !client.getUname().empty() && client.getAuth() && !client.getRegis())
    {
        client.setRegis(true);
        
        std::string fullmask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
        
        enqueue(client.outbuf, ":server 001 " + client.getNick() + " :Welcome to the Internet Relay Network " + fullmask + "\r\n");
        enqueue(client.outbuf, ":server 002 " + client.getNick() + " :Your host is server, running version 1.0\r\n");
        enqueue(client.outbuf, ":server 003 " + client.getNick() + " :This server was created\r\n");//tarih eklemeyi unutma
        enqueue(client.outbuf, ":server 004 " + client.getNick() + " server 1.0 o o\r\n");
        // MOTD yoksa bunu gönder (HexChat bekleyebilir)
        enqueue(client.outbuf, ":server 422 " + client.getNick() + " :MOTD File is missing\r\n");
    }
}


bool Server::nicknameCheck(std::string nickname)
{
    for (std::vector<Client *>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if ((*it)->getNick() == nickname)
        {
            return false;
        }
    }
    return true;
}

void Server::commandHandler(std::string cmd, std::vector<std::string> params, Client &client)
{//cap bak
    // IRCv3 CAP (HexChat bağlantı başında gönderir)
    if (cmd == "CAP")
    {
        std::string nickOrStar = client.getNick().empty() ? "*" : client.getNick();
        if (params.empty())
        {
            enqueue(client.outbuf, ":server 461 " + nickOrStar + " CAP :Not enough parameters\r\n");
            return;
        }
        std::string sub = params[0];
        if (sub == "LS")
        {
            // Desteklenen capability yok: boş liste
            enqueue(client.outbuf, ":server CAP " + nickOrStar + " LS :\r\n");
        }
        else if (sub == "LIST")
        {
            enqueue(client.outbuf, ":server CAP " + nickOrStar + " LIST :\r\n");
        }
        else if (sub == "REQ")
        {
            std::string req = params.size() > 1 ? params[1] : "";
            enqueue(client.outbuf, ":server CAP " + nickOrStar + " NAK :" + req + "\r\n");
        }
        else if (sub == "END")
        {
            // noop
        }
        else
        {
            enqueue(client.outbuf, ":server 421 " + nickOrStar + " CAP :Unknown command\r\n");
        }
        return;
    }

    if(cmd == "PASS")
    {
        if (params.empty())
        {
            enqueue(client.outbuf, ":server 461 * PASS :Not enough parameters\r\n");
            return;
        }
        if (client.getAuth())
        {
            enqueue(client.outbuf, ":server 462 * :You may not reregister\r\n");
            return;
        }
        
        client.setAuth(params[0] == this->password);
        
        if (!client.getAuth())
        {
            enqueue(client.outbuf, ":server 464 * :Password incorrect\r\n");
            return;
        }
        // PASS başarılı olduysa kayıt tamamlanabilir mi kontrol et
        checkRegistration(client);
        return ;
    }

    // NICK (kayıt öncesinde kabul et)
    if (cmd == "NICK")
    {
        if (params.empty())
        {
            enqueue(client.outbuf, ":server 431 * :No nickname given\r\n");
            return;
        }
        
        std::string nickname = params[0];
        if (nickname.find(' ') != std::string::npos || nickname.empty())
        {
            enqueue(client.outbuf, ":server 432 * " + nickname + " :Erroneous nickname\r\n");
            return;
        }
        
        if (nickname != client.getNick() && !nicknameCheck(nickname))
        {
            std::string target = client.getNick().empty() ? "*" : client.getNick();
            enqueue(client.outbuf, ":server 433 " + target + " " + nickname + " :Nickname is already in use\r\n");
            return;
        }
        
        client.setNick(nickname);
        checkRegistration(client);
        return;
    }
    else if (cmd == "USER")
    {
        if (params.size() < 4)
        {
            enqueue(client.outbuf, ":server 461 * USER :Not enough parameters\r\n");
            return;
        }
        
        if (client.getRegis())
        {
            enqueue(client.outbuf, ":server 462 * :You may not reregister\r\n");
            return;
        }
        
        std::string username = params[0];
        std::string hostname = params[1];
        std::string servername = params[2];    
        std::string realname = params[3];      
        
        if (username.empty() || username.find(' ') != std::string::npos)
        {
            enqueue(client.outbuf, ":server 461 * USER :Invalid username\r\n");
            return;
        }
        
        client.setUname(username);
        client.setRname(realname);
        client.setHname(hostname); 
        
        checkRegistration(client);
        return;
    }

    // Kayıt öncesi PING/PONG/QUIT'e izin ver
    if (cmd == "PING")
    {
        if (params.empty())
        {
            enqueue(client.outbuf, ":server 409 * :No origin specified\r\n");
            return;
        }
        enqueue(client.outbuf, ":server PONG server :" + params[0] + "\r\n");
        return;
    }
    else if (cmd == "PONG")
    {
        return;
    }
    else if (cmd == "QUIT")
    {
        handleQuit(params, client);
        return;
    }

    // Bilinen komutları kontrol et
    if (cmd == "JOIN" || cmd == "PRIVMSG" || cmd == "PART" || cmd == "NOTICE" || 
        cmd == "MODE" || cmd == "TOPIC" || cmd == "NAMES" || cmd == "LIST" || 
        cmd == "INVITE" || cmd == "KICK" || cmd == "WHO" || cmd == "WHOIS" || cmd == "MOTD")
    {
        // Kayıt tamamlanmadan bu komutlara izin verme
        if (!client.getRegis())
        {
            enqueue(client.outbuf, ":server 451 * :You have not registered\r\n");
            return ;
        }
    }
    else
    {
        // Bilinmeyen komut
        std::string nickOrStar = client.getNick().empty() ? "*" : client.getNick();
        enqueue(client.outbuf, ":server 421 " + nickOrStar + " " + cmd + " :Unknown command\r\n");
        return;
    }

    // MOTD isteğini karşıla (MOTD yoksa 422)
    if (cmd == "MOTD")
    {
        enqueue(client.outbuf, ":server 422 " + client.getNick() + " :MOTD File is missing\r\n");
        return;
    }

    if (cmd == "JOIN")
    {
        handleJoin(params, client);
    }
    else if (cmd == "PRIVMSG")
    {
        handlePrivMsg(params, client);
    }
    else if (cmd == "PART")
    {
        handlePart(params, client);
    }
    else if (cmd == "NOTICE")
    {
        handleNotice(params, client);
    }
    else if (cmd == "MODE")
    {
        handleMode(params, client);
    }
    else if (cmd == "TOPIC")
    {
        handleTopic(params, client);
    }
    else if (cmd == "NAMES")
    {
        handleName(params, client);
    }
    else if (cmd == "LIST")
    {
        handleList(params, client);
    }
    else if (cmd == "INVITE")
    {
        handleInvite(params, client);
    }
    else if (cmd == "KICK")
    {
        handleKick(params, client);
    }
    else if (cmd == "WHO")
    {
        handleWho(params, client);
    }
    else if (cmd == "WHOIS")
    {
        handleWhois(params, client);
    }
}
