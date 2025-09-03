#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <map>
#include "Client.hpp"

class Channel {
private:
    std::string name;
    std::string topic;
    std::string pin;  
    std::vector<Client*> members;
    std::vector<Client*> operators;
    bool invite_only;
    bool topic_restricted;
    int user_limit;

public:
    Channel(const std::string& channelName);
    ~Channel();
    
    bool addClient(Client* client, const std::string& key = "");
    void removeClient(Client* client);
    bool hasClient(Client* client);
    
    std::string getName() const;
    std::string getTopic() const;
    void setTopic(const std::string& newTopic);
    bool hasKey() const;
    bool checkKey(const std::string& providedKey) const;
    void setKey(const std::string& newKey);
    
    std::vector<Client*> getMembers() const;
    bool isOperator(Client* client);
    void addOperator(Client* client);
    size_t getMemberCount() const;
    
    void sendMsg(const std::string& message, Client* sender = NULL);
    
    bool isInviteOnly() const;
    void setInviteOnly(bool value);
    bool isTopicRestricted() const;
    void setTopicRestricted(bool value);
    int getUserLimit() const;
    void setUserLimit(int limit);
};

#endif