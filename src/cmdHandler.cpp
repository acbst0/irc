#include "../include/Server.hpp"


void Server::checkRegistration(Client &client)
{
	if (!client.getNick().empty() && !client.getUname().empty() && client.getAuth() && !client.getRegis())
	{
		client.setRegis(true);
		
		std::string fullmask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
		
		enqueue(client.outbuf, ":server 375 " + client.getNick() + " :- server Message of the Day -\r\n");
		enqueue(client.outbuf, ":server 372 " + client.getNick() + " :- carpe diem\r\n");
		enqueue(client.outbuf, ":server 376 " + client.getNick() + " :End of /MOTD command\r\n");
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
			return;
		}
		checkRegistration(client);
		return ;
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
			std::string target;
			if (client.getNick().empty())
				target = "*";
			else
				target = client.getNick();
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

	if (cmd == "JOIN" || cmd == "PRIVMSG" || cmd == "PART" || cmd == "NOTICE" || 
		cmd == "MODE" || cmd == "TOPIC" || cmd == "NAMES" || cmd == "LIST" || 
		cmd == "INVITE" || cmd == "KICK" || cmd == "WHO" || cmd == "WHOIS" || cmd == "MOTD" || cmd == "AWAY" || cmd == "BACK")
	{
		if (!client.getRegis())
		{
			enqueue(client.outbuf, ":server 451 * :You have not registered\r\n");
			return ;
		}
	}
	else
	{
		std::string nickOrStar;
		if (client.getNick().empty())
			nickOrStar = "*";
		else
			nickOrStar = client.getNick();
		enqueue(client.outbuf, ":server 421 " + nickOrStar + " " + cmd + " :Unknown command\r\n");
		return;
	}

	if (cmd == "MOTD")
	{
		enqueue(client.outbuf, ":server 375 " + client.getNick() + " :- server Message of the Day -\r\n");
		enqueue(client.outbuf, ":server 372 " + client.getNick() + " :- carpe diem\r\n");
		enqueue(client.outbuf, ":server 376 " + client.getNick() + " :End of /MOTD command\r\n");
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
	else if (cmd == "AWAY")
	{
		handleAway(params, client);
	}
	else if (cmd == "BACK")
	{
		std::vector<std::string> emptyParams;
		handleAway(emptyParams, client);
	}
	
}