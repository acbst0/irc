#include "../include/Server.hpp"


void Server::handleJoin(const std::vector<std::string>& params, Client &client)
{
    if (params.empty())
    {
        enqueue(client.outbuf, ":server 461 " + client.getNick() + " JOIN :Not enough parameters\r\n");
        return;
    }
    
    std::string channels = params[0];
    std::string keys = (params.size() > 1) ? params[1] : "";
    
    //0 özel durum, bütün kanallardan ayrıl
    if (channels == "0")
    {
        return;
    }
    
    std::vector<std::string> channelList;
    std::vector<std::string> keyList;
    
    // Kanal listesini parse et
    std::stringstream channelStream(channels);
    std::string channel;
    while (std::getline(channelStream, channel, ','))
    {
        if (!channel.empty())
            channelList.push_back(channel);
    }
    
    // key daha implemente edilmedi

    // if (!keys.empty())
    // {
    //     std::stringstream keyStream(keys);
    //     std::string key;
    //     while (std::getline(keyStream, key, ','))
    //     {
    //         keyList.push_back(key);
    //     }
    // }
    
    // her parametre için join komutunu çalıştır
    for (size_t i = 0; i < channelList.size(); ++i)
    {
        std::string channelName = channelList[i];
        std::string channelKey = (i < keyList.size()) ? keyList[i] : "";
        
        if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&'))
        {
            enqueue(client.outbuf, ":server 403 " + client.getNick() + " " + channelName + " :No such channel\r\n");
            continue;
        }
        
        // rfc 2812 kanal ismi maksimum 50 karakterden oluşabilir.
        if (channelName.length() > 50)
        {
            enqueue(client.outbuf, ":server 403 " + client.getNick() + " " + channelName + " :No such channel\r\n");
            continue;
        }
        
        if (channelName.find(' ') != std::string::npos || 
            channelName.find('\7') != std::string::npos || 
            channelName.find(',') != std::string::npos)
        {
            enqueue(client.outbuf, ":server 403 " + client.getNick() + " " + channelName + " :No such channel\r\n");
            continue;
        }
        
        // kanal yoksa oluştur
        Channel* targetChannel = nullptr;
        std::map<std::string, Channel*>::iterator channelIt = this->channels.find(channelName);
        
        if (channelIt == this->channels.end())
        {
            targetChannel = new Channel(channelName); // new kullandım destructorda delete edilmeli
            this->channels[channelName] = targetChannel;
        }
        else
        {
            targetChannel = channelIt->second;
        }
        
        //userı kanala eklemek
        if (!targetChannel->addClient(&client, channelKey))
        {
            if (targetChannel->hasKey() && !targetChannel->checkKey(channelKey))
            {
                enqueue(client.outbuf, ":server 475 " + client.getNick() + " " + channelName + " :Cannot join channel (+k)\r\n");
            }
            else if (targetChannel->isInviteOnly())
            {
                enqueue(client.outbuf, ":server 473 " + client.getNick() + " " + channelName + " :Cannot join channel (+i)\r\n");
            }
            else if (targetChannel->getUserLimit() > 0 && (int)targetChannel->getMemberCount() >= targetChannel->getUserLimit())
            {
                enqueue(client.outbuf, ":server 471 " + client.getNick() + " " + channelName + " :Cannot join channel (+l)\r\n");
            }
            continue;
        }
        
        // JOIN başarılı - mesajları gönder
        std::string userMask = client.getNick() + "!" + client.getUname() + "@" + client.getHname();
        std::string joinMsg = ":" + userMask + " JOIN " + channelName + "\r\n";
        
        // Kanaldaki herkese JOIN mesajı gönder
        targetChannel->sendMsg(joinMsg, nullptr);
        enqueue(client.outbuf, joinMsg);
        
        if (!targetChannel->getTopic().empty())
        {
            enqueue(client.outbuf, ":server 332 " + client.getNick() + " " + channelName + " :" + targetChannel->getTopic() + "\r\n");
        }
        else
        {
            enqueue(client.outbuf, ":server 331 " + client.getNick() + " " + channelName + " :No topic is set\r\n");
        }
        
        // kanal kullanıcı listesini göster
        std::string namesList = "";
        std::vector<Client*> members = targetChannel->getMembers();
        for (std::vector<Client*>::iterator memberIt = members.begin(); memberIt != members.end(); ++memberIt)
        {
            if (!namesList.empty())
                namesList += " ";
            
            // operatörğn başına @ ekle
            if (targetChannel->isOperator(*memberIt))
                namesList += "@";
            
            namesList += (*memberIt)->getNick();
        }
        enqueue(client.outbuf, ":server 353 " + client.getNick() + " = " + channelName + " :" + namesList + "\r\n");
        
        enqueue(client.outbuf, ":server 366 " + client.getNick() + " " + channelName + " :End of /NAMES list\r\n");
    }
}