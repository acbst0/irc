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