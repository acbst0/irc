#include "../include/Client.hpp"

Client::Client() 
{
	this->fd = 0;
}

Client::Client(int _fd)
{
	this->fd = _fd;
}

Client::~Client() {}

int Client::getFd()
{
	return this->fd;
}

void Client::setFd(int _fd)
{
	this->fd = _fd;
}

bool Client::getAuth() { return this->is_authenticated; }

void Client::setAuth(bool i) 
{
	this->is_authenticated = i;
}

bool Client::getRegis() { return this->is_registered; }

void Client::setRegis(bool i)
{
	this->is_registered = i;
}

void Client::setNick(std::string str)
{
	this->nick = str;
}

std::string Client::getNick()
{
	return this->nick;
}

void Client::setUsername(std::string str)
{
	this->username = str;
}

std::string Client::getUsername()
{
	return this->username;
}

void Client::setRealname(std::string str)
{
	this->realname = str;
}