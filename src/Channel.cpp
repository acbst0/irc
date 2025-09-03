#include "../include/Channel.hpp"
#include "../include/libs.hpp"

Channel::Channel(const std::string& channelName) 
    : name(channelName), topic(""), pin(""), invite_only(false), topic_restricted(true), user_limit(0)
{
}

Channel::~Channel()
{
}

bool Channel::addClient(Client* client, const std::string& providedKey)
{
    if (hasClient(client))
        return true;
    
    if (hasKey() && !checkKey(providedKey))
        return false;
    
    if (invite_only)
        return false; // TODO: invite list eklenecek
    
    // user limit
    if (user_limit > 0 && (int)members.size() >= user_limit)
        return false;
    
    // Kullanıcıyı ekle
    members.push_back(client);
    
    if (members.size() == 1)
        operators.push_back(client);
    
    return true;
}

void Channel::removeClient(Client* client)
{
    for (std::vector<Client*>::iterator it = members.begin(); it != members.end(); ++it)
    {
        if (*it == client)
        {
            members.erase(it);
            break;
        }
    }
    
    for (std::vector<Client*>::iterator it = operators.begin(); it != operators.end(); ++it)
    {
        if (*it == client)
        {
            operators.erase(it);
            break;
        }
    }
}

bool Channel::hasClient(Client* client)
{
    for (std::vector<Client*>::iterator it = members.begin(); it != members.end(); ++it)
    {
        if (*it == client)
            return true;
    }
    return false;
}

std::string Channel::getName() const
{
    return name;
}

std::string Channel::getTopic() const
{
    return topic;
}

void Channel::setTopic(const std::string& newTopic)
{
    topic = newTopic;
}

bool Channel::hasKey() const
{
    return !pin.empty();
}

bool Channel::checkKey(const std::string& providedKey) const
{
    return pin == providedKey;
}

void Channel::setKey(const std::string& newKey)
{
    pin = newKey;
}

std::vector<Client*> Channel::getMembers() const
{
    return members;
}

bool Channel::isOperator(Client* client)
{
    for (std::vector<Client*>::iterator it = operators.begin(); it != operators.end(); ++it)
    {
        if (*it == client)
            return true;
    }
    return false;
}

void Channel::addOperator(Client* client)
{
    if (!isOperator(client))
        operators.push_back(client);
}

size_t Channel::getMemberCount() const
{
    return members.size();
}

void Channel::sendMsg(const std::string& message, Client* sender)
{
    for (std::vector<Client*>::iterator it = members.begin(); it != members.end(); ++it)//kanaldaki herkese mesajı gönderiyor
    {
        if (*it != sender)
        {
            (*it)->outbuf += message;//her bir üyenin output bufferına gönderiyor
        }
    }
}

bool Channel::isInviteOnly() const
{
    return invite_only;
}

void Channel::setInviteOnly(bool value)
{
    invite_only = value;
}

bool Channel::isTopicRestricted() const
{
    return topic_restricted;
}

void Channel::setTopicRestricted(bool value)
{
    topic_restricted = value;
}

int Channel::getUserLimit() const
{
    return user_limit;
}

void Channel::setUserLimit(int limit)
{
    user_limit = limit;
}