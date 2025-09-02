#include "../include/Server.hpp"


void Server::handlePrivMsg(const std::vector<std::string>& params, Client &client)
{
    if (params.size() < 2)
    {
        enqueue(client.outbuf, ":server 411 " + client.getNick() + " :No recipient given PRIVMSG\r\n");
        return;
    }
    
    std::string target = params[0];
    std::string message = params[1];
    
    if (message.empty())
    {
        enqueue(client.outbuf, ":server 412 " + client.getNick() + " :No text to send\r\n");
        return;
    }
    
    std::string uMask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
    std::string privmsgLine = ":" + uMask + " PRIVMSG " + target + " :" + message + "\r\n";
    
    if (target[0] == '#' || target[0] == '&')
    {
        std::map<std::string, Channel*>::iterator channelIt = this->channels.find(target);// map içerisinde elmente göre arama yaptırılıyor
        Channel* targetChannel = channelIt->second;
       
        if (!targetChannel->hasClient(&client))
        {
            enqueue(client.outbuf, ":server 404 " + client.getNick() + " " + target + " :Cannot send to channel\r\n");
            return;
        }

        if (channelIt == this->channels.end())
        {
            enqueue(client.outbuf, ":server 403 " + client.getNick() + " " + target + " :No such channel\r\n");
            return;
        }
        targetChannel->sendMsg(privmsgLine, &client);
    }

    else
    {
        Client* targetClient = nullptr;
        
        
        
        for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
        {
            if ((*it)->getNick() == target)
            {
                targetClient = *it;
                break ;
            }
        }
        
        if (targetClient == nullptr)
        {
            enqueue(client.outbuf, ":server 401 " + client.getNick() + " " + target + " :No such nick/channel\r\n");
            return;
        }

        enqueue(targetClient->outbuf, privmsgLine);
    }
}

