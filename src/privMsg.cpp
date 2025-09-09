#include "../include/Server.hpp"


void Server::handlePrivMsg(const std::vector<std::string>& params, Client &client)
{
    if (params.size() < 1)
    {
        enqueue(client.outbuf, ":server 411 " + client.getNick() + " :No recipient given (PRIVMSG)\r\n");
        return;
    }
    
    std::string target = params[0];    
    std::string message;

    //ikiden fazla parametre varsa cümle olarak mesaj gelmiştir. mesajı birleştir.
    if (params.size() >= 2)
    {
        for (size_t i = 1; i < params.size(); ++i)
        {
            if (i > 1) message += " ";
            message += params[i];
        }
    }
    
    if (message.empty())
    {
        enqueue(client.outbuf, ":server 412 " + client.getNick() + " :No text to send\r\n");
        return;
    }
    
    std::vector<std::string> targetList;
    std::stringstream targetStream(target);

    while (std::getline(targetStream, target, ','))
    {
        if (!target.empty())
            targetList.push_back(target);
    }

    //targetlar virgülle ayrılmışsa
    
    std::string uMask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
    
    for (std::vector<std::string>::iterator targetIt = targetList.begin(); 
         targetIt != targetList.end(); ++targetIt)
    {
        std::string currentTarget = *targetIt;
        std::string privmsgLine = ":" + uMask + " PRIVMSG " + currentTarget + " :" + message + "\r\n";
        
        if (currentTarget[0] == '#' || currentTarget[0] == '&')
        {
            std::map<std::string, Channel*>::iterator channelIt = this->channels.find(currentTarget);
            
            if (channelIt == this->channels.end())
            {
                enqueue(client.outbuf, ":server 404 " + client.getNick() + " " + currentTarget + " :Cannot send to channel\r\n");
                continue;
            }
            
            Channel* targetChannel = channelIt->second;
           
            if (!targetChannel->hasClient(&client))
            {
                enqueue(client.outbuf, ":server 404 " + client.getNick() + " " + currentTarget + " :Cannot send to channel\r\n");
                continue;
            }

            targetChannel->sendMsg(privmsgLine, &client);
        }

        
        else
        {
            // Özel mesaj
            Client* targetClient = NULL;
            
            for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
            {
                if ((*it)->getNick() == currentTarget)
                {
                    targetClient = *it;
                    break;
                }
            }
            
            if (targetClient == NULL)
            {
                enqueue(client.outbuf, ":server 401 " + client.getNick() + " " + currentTarget + " :No such nick/channel\r\n");
                continue;
            }

            if (targetClient->isAway())
                enqueue(client.outbuf, ":server 301 " + client.getNick() + " " + currentTarget + " :" + targetClient->getAwayMessage() + "\r\n");

            enqueue(targetClient->outbuf, privmsgLine);
        }
    }
}

