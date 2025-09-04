#include "../include/Server.hpp"

void Server::handleQuit(const std::vector<std::string>& params, Client &client)
{
    std::string quitMessage = "Quit";
    if (!params.empty())
        quitMessage = params[0];

    std::string userMask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
    std::string quitMsg = ":" + userMask + " QUIT :" + quitMessage + "\r\n";
    
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
                this->channels.erase(channelIt->first);
            }
        }
    }
    enqueue(client.outbuf, "ERROR :Closing Link: " + client.getHname() + " (" + quitMessage + ")\r\n");
}
