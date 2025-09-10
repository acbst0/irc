#include "../include/Server.hpp"

void Server::handleMode(const std::vector<std::string>& params, Client &client)
{
	if (params.empty())
    {
        enqueue(client.outbuf, ":server 461 " + client.getNick() + " MODE :Not enough parameters\r\n");
        return ;
    }

	std::string target = params[0];
	Channel* targetChannel = NULL;
	if (target[0] == '#' || target[0] == '&')
	{
		std::map<std::string, Channel*>::iterator channelIt = this->channels.find(target);
		if (channelIt == this->channels.end())
		{
			enqueue(client.outbuf, ":server 403 " + client.getNick() + " " + target + " :No such channel\r\n");
			return ;
		}
		targetChannel = channelIt->second;
		if (!targetChannel->isOperator(&client) && params.size() > 1)
		{
			enqueue(client.outbuf, ":server 482 " + client.getNick() + " " + target + " :You're not channel operator\r\n");
			return ;
		}
		else
		{
			if (params.size() < 2)
			{
				std::string modeList = "+";
				if (targetChannel->isInviteOnly()) modeList += "i";
				if (targetChannel->hasKey()) modeList += "k";
				if (targetChannel->isTopicRestricted()) modeList += "t";
				if (targetChannel->getUserLimit() > 0) modeList += "l";
				enqueue(client.outbuf, ":server 324 " + client.getNick() + " " + target + " " + modeList + "\r\n");
				return ;
			}

			std::string modeChanges = params[1];
			bool adding = true;
			for (size_t i = 0; i < modeChanges.size(); ++i)
			{
				char modeChar = modeChanges[i];
				if (modeChar == '+')
				{
					adding = true;
				}
				else if (modeChar == '-')
				{
					adding = false;
				}
				else
				{
					switch (modeChar)
					{
						case 'i':
							targetChannel->setInviteOnly(adding);
							break;
						case 'k':
							if (adding)
							{
								if (params.size() < 3)
								{
									enqueue(client.outbuf, ":server 461 " + client.getNick() + " MODE :Not enough parameters\r\n");
									return ;
								}
								targetChannel->setKey(params[2]);
							}
							else
							{
								targetChannel->setKey("");
							}
							break;
						case 't':
							targetChannel->setTopicRestricted(adding);
							break;
						case 'o':
							if (params.size() < 3)
							{
								enqueue(client.outbuf, ":server 461 " + client.getNick() + " MODE :Not enough parameters\r\n");
								return ;
							}
							{
								std::string targetNick = params[2];
								Client* targetClient = NULL;
								for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
								{
									if ((*it)->getNick() == targetNick)
									{
										targetClient = *it;
										break;
									}
								}
								
								if (targetClient && targetChannel->hasClient(targetClient))
								{
									if (adding)
									{
										targetChannel->addOperator(targetClient);
									}
									else
									{
										targetChannel->removeOperator(targetClient);
									}
									
									std::string userMask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
									std::string modeMsg;
									if (adding)
									    modeMsg = ":" + userMask + " MODE " + target + " +o " + targetNick + "\r\n";
									else
									    modeMsg = ":" + userMask + " MODE " + target + " -o " + targetNick + "\r\n";
									targetChannel->sendMsg(modeMsg, NULL);
								}
							}
							break;
						case 'l':
							if (adding)
							{
								if (params.size() < 3)
								{
									enqueue(client.outbuf, ":server 461 " + client.getNick() + " MODE :Not enough parameters\r\n");
									return ;
								}
								int limit = atoi(params[2].c_str());
								if (limit > 0)
									targetChannel->setUserLimit(limit);
								else
								{
									enqueue(client.outbuf, ":server 501 " + client.getNick() + " :Unknown MODE flag\r\n");
									return ;
								}
							}
							else
							{
								targetChannel->setUserLimit(0);
							}
							break;
						default:
							enqueue(client.outbuf, ":server 472 " + client.getNick() + " " + modeChar + " :is unknown mode char to me\r\n");
							return;
					}
				}
			}
		}
	}
}

void Server::handleTopic(const std::vector<std::string>& params, Client &client)
{
	if (params.empty())
	{
		enqueue(client.outbuf, ":server 461 " + client.getNick() + " TOPIC :Not enough parameters\r\n");
		return ;
	}
	
	std::string channelName = params[0];
	std::map<std::string, Channel*>::iterator channelIt = this->channels.find(channelName);
	
	if (channelIt == this->channels.end())
	{
		enqueue(client.outbuf, ":server 403 " + client.getNick() + " " + channelName + " :No such channel\r\n");
		return ;
	}
	
	Channel* targetChannel = channelIt->second;
	
	if (!targetChannel->hasClient(&client))
	{
		enqueue(client.outbuf, ":server 442 " + client.getNick() + " " + channelName + " :You're not on that channel\r\n");
		return ;
	}
	
	if (params.size() == 1)
	{
		std::string currentTopic = targetChannel->getTopic();
		if (currentTopic.empty())
		{
			enqueue(client.outbuf, ":server 331 " + client.getNick() + " " + channelName + " :No topic is set\r\n");
		}
		else
		{
			enqueue(client.outbuf, ":server 332 " + client.getNick() + " " + channelName + " :" + currentTopic + "\r\n");
		}
	}
	else
	{
		if (targetChannel->isTopicRestricted() && !targetChannel->isOperator(&client))
		{
			enqueue(client.outbuf, ":server 482 " + client.getNick() + " " + channelName + " :You're not channel operator\r\n");
			return ;
		}
		
		std::string newTopic = params[1];
		targetChannel->setTopic(newTopic);
		
		std::string userMask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
		std::string topicMsg = ":" + userMask + " TOPIC " + channelName + " :" + newTopic + "\r\n";
		
		targetChannel->sendMsg(topicMsg, NULL); 
	}
}

void Server::handleName(const std::vector<std::string>& params, Client &client)
{
	std::string channelList;
	if (params.empty())
		channelList = "";
	else
		channelList = params[0];
	std::vector<std::string> channelsToList;
	
	if (channelList.empty())
	{
		for (std::map<std::string, Channel*>::iterator it = this->channels.begin(); it != this->channels.end(); ++it)
			channelsToList.push_back(it->first);
	}
	else
	{
		std::stringstream ss(channelList);
		std::string channel;
		while (std::getline(ss, channel, ','))
		{
			if (!channel.empty())
				channelsToList.push_back(channel);
		}
	}
	
	for (size_t i = 0; i < channelsToList.size(); ++i)
	{
		std::string channelName = channelsToList[i];
		std::map<std::string, Channel*>::iterator channelIt = this->channels.find(channelName);
		
		if (channelIt == this->channels.end())
		{
			enqueue(client.outbuf, ":server 403 " + client.getNick() + " " + channelName + " :No such channel\r\n");
			continue ;
		}
		
		Channel* targetChannel = channelIt->second;
		
		std::string namesList = "";
		std::vector<Client*> members = targetChannel->getMembers();
		for (size_t j = 0; j < members.size(); ++j)
		{
			if (j > 0) namesList += " ";
			if (targetChannel->isOperator(members[j]))
				namesList += "@";
			namesList += members[j]->getNick();
		}
		
		enqueue(client.outbuf, ":server 353 " + client.getNick() + " = " + channelName + " :" + namesList + "\r\n");
		enqueue(client.outbuf, ":server 366 " + client.getNick() + " " + channelName + " :End of NAMES list\r\n");
	}
}

void Server::handleList(const std::vector<std::string>& params, Client &client)
{
	std::string channelList;
	if (params.empty())
		channelList = "";
	else
		channelList = params[0];
	std::vector<std::string> channelsToList;
	
	if (channelList.empty())
	{
		for (std::map<std::string, Channel*>::iterator it = this->channels.begin(); it != this->channels.end(); ++it)
			channelsToList.push_back(it->first);
	}
	else
	{
		std::stringstream ss(channelList);
		std::string channel;
		while (std::getline(ss, channel, ','))
		{
			if (!channel.empty())
				channelsToList.push_back(channel);
		}
	}
	
	for (size_t i = 0; i < channelsToList.size(); ++i)
	{
		std::string channelName = channelsToList[i];
		std::map<std::string, Channel*>::iterator channelIt = this->channels.find(channelName);
		
		if (channelIt == this->channels.end())
			continue ;
		
		Channel* targetChannel = channelIt->second;
		
		std::string topic = targetChannel->getTopic();
		if (topic.empty())
			topic = "No topic is set";
		
		std::string userCount = to_string(targetChannel->getMemberCount());
		
		enqueue(client.outbuf, ":server 322 " + client.getNick() + " " + channelName + " " + userCount + " :" + topic + "\r\n");
	}
	
	enqueue(client.outbuf, ":server 323 " + client.getNick() + " :End of /LIST\r\n");
}

void Server::handleInvite(const std::vector<std::string>& params, Client &client)
{
	if (params.size() < 2)
	{
		enqueue(client.outbuf, ":server 461 " + client.getNick() + " INVITE :Not enough parameters\r\n");
		return ;
	}
	
	std::string targetNick = params[0];
	std::string channelName = params[1];
	
	Client* targetClient = NULL;
	for (size_t i = 0; i < clients.size(); ++i)
	{
		if (clients[i]->getNick() == targetNick)
		{
			targetClient = clients[i];
			break ;
		}
	}
	
	if (targetClient == NULL)
	{
		enqueue(client.outbuf, ":server 401 " + client.getNick() + " " + targetNick + " :No such nick/channel\r\n");
		return ;
	}
	
	std::map<std::string, Channel*>::iterator channelIt = this->channels.find(channelName);
	if (channelIt == this->channels.end())
	{
		enqueue(client.outbuf, ":server 403 " + client.getNick() + " " + channelName + " :No such channel\r\n");
		return ;
	}
	
	Channel* targetChannel = channelIt->second;
	
	if (!targetChannel->hasClient(&client))
	{
		enqueue(client.outbuf, ":server 442 " + client.getNick() + " " + channelName + " :You're not on that channel\r\n");
		return ;
	}
	
	if (!targetChannel->isOperator(&client))
	{
		enqueue(client.outbuf, ":server 482 " + client.getNick() + " " + channelName + " :You're not channel operator\r\n");
		return ;
	}
	
	if (targetChannel->hasClient(targetClient))
	{
		enqueue(client.outbuf, ":server 443 " + client.getNick() + " " + targetNick + " " + channelName + " :is already on channel\r\n");
		return ;
	}
	
	std::string userMask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
	std::string inviteMsg = ":" + userMask + " INVITE " + targetNick + " :" + channelName + "\r\n";
	
	enqueue(targetClient->outbuf, inviteMsg);
	enqueue(client.outbuf, ":server 341 " + client.getNick() + " " + targetNick + " " + channelName + "\r\n");
	
	targetChannel->inviteUser(targetNick);
}

void Server::handleKick(const std::vector<std::string>& params, Client &client)
{
	if (params.size() < 2)
	{
		enqueue(client.outbuf, ":server 461 " + client.getNick() + " KICK :Not enough parameters\r\n");
		return ;
	}
	
	std::string channelName = params[0];
	std::string targetNick = params[1];
	std::string kickMessage;
	if (params.size() > 2)
	    kickMessage = params[2];
	else
	    kickMessage = "Kicked";
	
	std::map<std::string, Channel*>::iterator channelIt = this->channels.find(channelName);
	if (channelIt == this->channels.end())
	{
		enqueue(client.outbuf, ":server 403 " + client.getNick() + " " + channelName + " :No such channel\r\n");
		return ;
	}
	
	Channel* targetChannel = channelIt->second;
	
	if (!targetChannel->hasClient(&client))
	{
		enqueue(client.outbuf, ":server 442 " + client.getNick() + " " + channelName + " :You're not on that channel\r\n");
		return ;
	}
	
	if (!targetChannel->isOperator(&client))
	{
		enqueue(client.outbuf, ":server 482 " + client.getNick() + " " + channelName + " :You're not channel operator\r\n");
		return ;
	}
	
	Client* targetClient = NULL;
	for (size_t i = 0; i < clients.size(); ++i)
	{
		if (clients[i]->getNick() == targetNick)
		{
			targetClient = clients[i];
			break ;
		}
	}
	
	if (targetClient == NULL)
	{
		enqueue(client.outbuf, ":server 401 " + client.getNick() + " " + targetNick + " :No such nick/channel\r\n");
		return ;
	}
	
	if (!targetChannel->hasClient(targetClient))
	{
		enqueue(client.outbuf, ":server 441 " + client.getNick() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
		return ;
	}
	
	std::string userMask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
	std::string kickMsg = ":" + userMask + " KICK " + channelName + " " + targetNick + " :" + kickMessage + "\r\n";
	targetChannel->sendMsg(kickMsg, NULL); 
	targetChannel->removeClient(targetClient);
	if (targetChannel->getMemberCount() == 0)
	{
		delete targetChannel;
		this->channels.erase(channelIt->first);
	}
}

void Server::handleWho(const std::vector<std::string>& params, Client &client)
{
	std::string mask;
	if (params.empty())
		mask = "";
	else
		mask = params[0];
	
	for (size_t i = 0; i < clients.size(); ++i)
	{
		Client* currentClient = clients[i];
		if (mask.empty() || currentClient->getNick() == mask)
		{
			std::string userInfo = currentClient->getNick() + " " + currentClient->getUname() + " " + currentClient->getHname() + " * :" + currentClient->getRname();
			enqueue(client.outbuf, ":server 352 " + client.getNick() + " " + userInfo + "\r\n");
		}
	}
	
	enqueue(client.outbuf, ":server 315 " + client.getNick() + " :End of /WHO list\r\n");
}

void Server::handleWhois(const std::vector<std::string>& params, Client &client)
{
	if (params.empty())
	{
		enqueue(client.outbuf, ":server 461 " + client.getNick() + " WHOIS :Not enough parameters\r\n");
		return ;
	}
	
	std::string targetNick = params[0];
	Client* targetClient = NULL;
	for (size_t i = 0; i < clients.size(); ++i)
	{
		if (clients[i]->getNick() == targetNick)
		{
			targetClient = clients[i];
			break ;
		}
	}
	
	if (targetClient == NULL)
	{
		enqueue(client.outbuf, ":server 401 " + client.getNick() + " " + targetNick + " :No such nick/channel\r\n");
		return ;
	}
	
	std::string userInfo = targetClient->getNick() + " " + targetClient->getUname() + " " + targetClient->getHname() + " * :" + targetClient->getRname();
	enqueue(client.outbuf, ":server 311 " + client.getNick() + " " + userInfo + "\r\n");
	
	std::string channelsList = "";
	for (std::map<std::string, Channel*>::iterator it = this->channels.begin(); it != this->channels.end(); ++it)
	{
		if (it->second->hasClient(targetClient))
		{
			if (!channelsList.empty()) channelsList += " ";
			if (it->second->isOperator(targetClient)) channelsList += "@";
			channelsList += it->first;
		}
	}
	if (!channelsList.empty())
	{
		enqueue(client.outbuf, ":server 319 " + client.getNick() + " " + targetNick + " :" + channelsList + "\r\n");
	}
	
	enqueue(client.outbuf, ":server 312 " + client.getNick() + " " + targetNick + " :server :IRC Server\r\n");
	enqueue(client.outbuf, ":server 317 " + client.getNick() + " " + targetNick + " 0 :seconds idle\r\n");
	enqueue(client.outbuf, ":server 318 " + client.getNick() + " " + targetNick + " :End of /WHOIS list\r\n");
}
