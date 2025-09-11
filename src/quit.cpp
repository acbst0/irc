/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   quit.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abostano <abostano@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/11 12:48:42 by abostano          #+#    #+#             */
/*   Updated: 2025/09/11 12:51:21 by abostano         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"

void Server::handleQuit(const std::vector<std::string>& params, Client &client)
{
	std::string quitMessage = "Quit";
	if (!params.empty())
		quitMessage = params[0];

	std::string userMask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
	std::string quitMsg = ":" + userMask + " QUIT :" + quitMessage + "\r\n";
	
	std::vector<std::string> channelsToDelete;
	
	for (std::map<std::string, Channel*>::iterator channelIt = this->channels.begin(); channelIt != this->channels.end(); ++channelIt)
	{
		Channel* channel = channelIt->second;
		
		if (channel->hasClient(&client))
		{
			channel->sendMsg(quitMsg, &client);
			channel->removeClient(&client);
			if (channel->getMemberCount() == 0)
			{
				delete channel;
				channelsToDelete.push_back(channelIt->first);
			}
		}
	}
	for (std::vector<std::string>::iterator it = channelsToDelete.begin(); it != channelsToDelete.end(); ++it)
	{
		this->channels.erase(*it);
	}
	
	enqueue(client.outbuf, "ERROR :Closing Link: " + client.getHname() + " (" + quitMessage + ")\r\n");
}
