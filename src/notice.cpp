/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   notice.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fkuyumcu <fkuyumcu@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/11 13:22:03 by fkuyumcu          #+#    #+#             */
/*   Updated: 2025/09/11 13:22:04 by fkuyumcu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

void Server::handleNotice(const std::vector<std::string>& params, Client &client)
{
	if (params.size() < 2)
		return ;
	
	std::string target = params[0];
	std::string message = params[1];
	
	if (target[0] == '#' || target[0] == '&')
	{
		std::map<std::string, Channel*>::iterator channelIt = this->channels.find(target);
		
		if (channelIt == this->channels.end())
			return ;
		
		Channel* targetChannel = channelIt->second;
		if (!targetChannel->hasClient(&client))
			return ;
		
		std::string userMask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
		std::string noticeMsg = ":" + userMask + " NOTICE " + target + " :" + message + "\r\n";
		targetChannel->sendMsg(noticeMsg, &client);
	}
	else
	{
		Client* targetClient = NULL;
		for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
		{
			if ((*it)->getNick() == target)
			{
				targetClient = *it;
				break ;
			}
		}
		
		if (targetClient == NULL)
			return ;

		std::string userMask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
		std::string noticeMsg = ":" + userMask + " NOTICE " + target + " :" + message + "\r\n";
		enqueue(targetClient->outbuf, noticeMsg);
	}
}
