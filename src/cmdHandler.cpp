#include "../include/Server.hpp"



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
		enqueue(client.outbuf, client.getAuth() ? ":server 001 * :Password accepted\r\n"
										: ":server 464 * :Password incorrect\r\n");
		return;
	}
	if(!client.getAuth())
		enqueue(client.outbuf, ":server 421 * : Please enter the PASS command\r\n");
	else
	{
		
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
			client.setNick(nickname);
    		enqueue(client.outbuf, ":server 001 * :Nickname set to " + nickname + "\r\n");
			if (!client.getUname().empty())
				client.setRegis(true);
		}
		else if (cmd == "USER")
		{
			if (params.size() < 4)
			{
				enqueue(client.outbuf, ":server 461 * USER :Not enough parameters\r\n");
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
			enqueue(client.outbuf, ":server 001 * :Username set to " + username + "\r\n");
			
			if (!client.getNick().empty())
				client.setRegis(true);
		}

		if (client.getRegis() == false)
		{
			if(client.getUname() == "" )
				enqueue(client.outbuf, ":server * USER :Not enough parameters\r\n");

			if(client.getNick() == "")
				enqueue(client.outbuf, ":server 431 * :No nickname given\r\n");
			return ;
		}

		else if (cmd == "JOIN")
		{
			// TODO: Implement JOIN command
			enqueue(client.outbuf, ":server 421 * JOIN :Not implemented yet\r\n");
		}
		else if (cmd == "PART")
		{
			// TODO: Implement PART command
			enqueue(client.outbuf, ":server 421 * PART :Not implemented yet\r\n");
		}
		else if (cmd == "PRIVMSG")
		{
			// TODO: Implement PRIVMSG command
			enqueue(client.outbuf, ":server 421 * PRIVMSG :Not implemented yet\r\n");
		}
		else if (cmd == "NOTICE")
		{
			// TODO: Implement NOTICE command
			enqueue(client.outbuf, ":server 421 * NOTICE :Not implemented yet\r\n");
		}
		else if (cmd == "QUIT")
		{
			// TODO: Implement QUIT command
			enqueue(client.outbuf, ":server 421 * QUIT :Not implemented yet\r\n");
		}
		else if (cmd == "MODE")
		{
			// TODO: Implement MODE command
			enqueue(client.outbuf, ":server 421 * MODE :Not implemented yet\r\n");
		}
		else if (cmd == "TOPIC")
		{
			// TODO: Implement TOPIC command
			enqueue(client.outbuf, ":server 421 * TOPIC :Not implemented yet\r\n");
		}
		else if (cmd == "NAMES")
		{
			// TODO: Implement NAMES command
			enqueue(client.outbuf, ":server 421 * NAMES :Not implemented yet\r\n");
		}
		else if (cmd == "LIST")
		{
			// TODO: Implement LIST command
			enqueue(client.outbuf, ":server 421 * LIST :Not implemented yet\r\n");
		}
		else if (cmd == "INVITE")
		{
			// TODO: Implement INVITE command
			enqueue(client.outbuf, ":server 421 * INVITE :Not implemented yet\r\n");
		}
		else if (cmd == "KICK")
		{
			// TODO: Implement KICK command
			enqueue(client.outbuf, ":server 421 * KICK :Not implemented yet\r\n");
		}
		else if (cmd == "WHO")
		{
			// TODO: Implement WHO command
			enqueue(client.outbuf, ":server 421 * WHO :Not implemented yet\r\n");
		}
		else if (cmd == "WHOIS")
		{
			// TODO: Implement WHOIS command
			enqueue(client.outbuf, ":server 421 * WHOIS :Not implemented yet\r\n");
		}
		else if (cmd == "PING")
		{
			// TODO: Implement PING command
			enqueue(client.outbuf, ":server 421 * PING :Not implemented yet\r\n");
		}
		else if (cmd == "PONG")
		{
			// TODO: Implement PONG command
			enqueue(client.outbuf, ":server 421 * PONG :Not implemented yet\r\n");
		}
		else 
			 enqueue(client.outbuf, ":server 421 * " + cmd + " :Unknown command\r\n");
	}
}