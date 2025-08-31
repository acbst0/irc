#include "../include/Server.hpp"


void Server::checkRegistration(Client &client)
{
	if (!client.getNick().empty() && !client.getUname().empty() && !client.getRegis())
	{
		client.setRegis(true);
		
		std::string fullmask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
		

		enqueue(client.outbuf, ":server 001 " + client.getNick() + " :Welcome to the Internet Relay Network " + fullmask + "\r\n");
		enqueue(client.outbuf, ":server 002 " + client.getNick() + " :Your host is server, running version 1.0\r\n");
		enqueue(client.outbuf, ":server 003 " + client.getNick() + " :This server was created\r\n");//tarih eklemeyi unutma
		enqueue(client.outbuf, ":server 004 " + client.getNick() + " server 1.0 o o\r\n");
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
{
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
        }
        return;
    }
    
    if(!client.getAuth())
    {
        enqueue(client.outbuf, ":server 464 * :Password incorrect\r\n");
        return;
    }
    
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
    }


    else if (!client.getRegis())
    {
        enqueue(client.outbuf, ":server 451 * :You have not registered\r\n");
        return ;
    }
    // Kayıtlı kullanıcılar için diğer komutlar
    else if (cmd == "JOIN")
    {
        enqueue(client.outbuf, ":server 421 * JOIN :Not implemented yet\r\n");
    }
    else if (cmd == "PART")
    {
        enqueue(client.outbuf, ":server 421 * PART :Not implemented yet\r\n");
    }
    else if (cmd == "PRIVMSG")
    {
        enqueue(client.outbuf, ":server 421 * PRIVMSG :Not implemented yet\r\n");
    }
    else if (cmd == "NOTICE")
    {
        enqueue(client.outbuf, ":server 421 * NOTICE :Not implemented yet\r\n");
    }
    else if (cmd == "QUIT")
    {
        enqueue(client.outbuf, ":server 421 * QUIT :Not implemented yet\r\n");
    }
    else if (cmd == "MODE")
    {
        enqueue(client.outbuf, ":server 421 * MODE :Not implemented yet\r\n");
    }
    else if (cmd == "TOPIC")
    {
        enqueue(client.outbuf, ":server 421 * TOPIC :Not implemented yet\r\n");
    }
    else if (cmd == "NAMES")
    {
        enqueue(client.outbuf, ":server 421 * NAMES :Not implemented yet\r\n");
    }
    else if (cmd == "LIST")
    {
        enqueue(client.outbuf, ":server 421 * LIST :Not implemented yet\r\n");
    }
    else if (cmd == "INVITE")
    {
        enqueue(client.outbuf, ":server 421 * INVITE :Not implemented yet\r\n");
    }
    else if (cmd == "KICK")
    {
        enqueue(client.outbuf, ":server 421 * KICK :Not implemented yet\r\n");
    }
    else if (cmd == "WHO")
    {
        enqueue(client.outbuf, ":server 421 * WHO :Not implemented yet\r\n");
    }
    else if (cmd == "WHOIS")
    {
        enqueue(client.outbuf, ":server 421 * WHOIS :Not implemented yet\r\n");
    }
    else if (cmd == "PING")
    {
        if (params.empty())
        {
            enqueue(client.outbuf, ":server 409 * :No origin specified\r\n");
            return;
        }
        // PING'e PONG ile cevap ver
        enqueue(client.outbuf, ":server PONG server :" + params[0] + "\r\n");
    }
    else if (cmd == "PONG")
    {
        // PONG komutunu sessizce kabul et (keepalive için)
    }
    else 
    {
        enqueue(client.outbuf, ":server 421 * " + cmd + " :Unknown command\r\n");
    }
}

// Ayrı fonksiyon olarak kayıt kontrolü