#include "../include/Client.hpp"

Client::Client() 
{
	this->fd = 0;
	this->is_authenticated = false;
	this->is_registered = false;
	this->nick = "";
	this->username = "";
	this->away = false;
	this->awayMessage = "";
}

Client::Client(int _fd)
{
	this->fd = _fd;
	this->is_authenticated = false;
	this->is_registered = false;
	this->nick = "";
	this->username = "";
	this->away = false;
	this->awayMessage = "";
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
void Client::setAuth(bool i) {this->is_authenticated = i;}
bool Client::getRegis() { return this->is_registered; }
void Client::setRegis(bool i) {this->is_registered = i;}

std::string Client::getNick(void){return this->nick;}
std::string Client::getUname(void){return this->username;}
std::string Client::getRname(void){return this->realname;}
std::string Client::getHname(void){return this->hostname;}

void Client::setNick(std::string nick){this->nick = nick;}
void Client::setUname(std::string username){this->username = username;}
void Client::setHname(std::string hostname){this->hostname = hostname;}
void Client::setRname(std::string realname){this->realname = realname;}

bool Client::isAway() { return this->away; }
void Client::setAway(bool status) { this->away = status; }
std::string Client::getAwayMessage() { return this->awayMessage; }
void Client::setAwayMessage(std::string message) { this->awayMessage = message; }




