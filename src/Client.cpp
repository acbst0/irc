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