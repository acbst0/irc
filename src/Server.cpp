#include "../include/Server.hpp"

Server::Server()
{
	this->serverFd = 0;
	this->num_of_pfd = 0;
	this->running = true;
	
	std::memset(this->pfds, 0, sizeof(this->pfds));
}

Server::~Server()
{
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
    {
        delete it->second;
    }
    channels.clear();
    
    for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if ((*it)->getFd() > 0)
            close((*it)->getFd());
        delete *it;
    }
    clients.clear();
    
    if (serverFd > 0)
        close(serverFd);
}

void setNonBlocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) 
		throw(std::runtime_error("Failed while setting socket non-blocking."));
}

void Server::stop()
{
    std::cout << "Stopping..." << std::endl;
    this->running = false;
}


void Server::initServer(struct sockaddr_in &hints, int port)
{	
	std::memset(&hints, 0 , sizeof(hints));
	hints.sin_family = AF_INET;
	hints.sin_port = htons(port);
	hints.sin_addr.s_addr = INADDR_ANY;
	
	
	this->serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->serverFd == -1)
	{
		std::cerr << "SOCKET ERROR" << std::endl;
		throw std::exception();
	}
	
	int yes = 1;
	
	if(setsockopt(this->serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
		throw(std::runtime_error("Setsockopt error."));
	
	if (bind(this->serverFd, (struct sockaddr *)&hints, sizeof(hints)) < 0)
		throw(std::runtime_error("Error while binding socket."));
		
	if (listen(this->serverFd, BACKLOG) < 0)
		throw(std::runtime_error("Error while listening socket."));
	setNonBlocking(this->serverFd);
	
	std::cout << "IRC Server Has Been Running!" << std::endl;
	
	this->pfds[0].fd = this->serverFd;
	this->pfds[0].events = POLLIN;
	this->num_of_pfd++;
	

}

void Server::removeClient(int index)
{
	Client* clientToRemove = NULL;
	
	for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		if ((*it)->getFd() == pfds[index].fd)
		{
			clientToRemove = *it;
			break;
		}
	}
	
	if (clientToRemove)
	{
		for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); )
		{
			Channel* channel = it->second;
			if (channel->hasClient(clientToRemove))
			{
				channel->removeClient(clientToRemove);
				
				if (channel->getMemberCount() == 0)
				{
					delete channel;
					std::map<std::string, Channel*>::iterator toErase = it;
					++it;
					channels.erase(toErase);
					continue;
				}
			}
			++it;
		}
		
		for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
		{
			if (*it == clientToRemove)
			{
				delete *it;
				clients.erase(it);
				break;
			}
		}
	}
	
	for (int i = index; i < num_of_pfd - 1; i++)
	{
		pfds[i] = pfds[i + 1];
	}
	num_of_pfd--;
}



void Server::commandParser(Client &client, std::string &message)
{
	std::cout << "Processing command from client " << client.getFd() << ": " << message << std::endl;
	
	std::string cmd, trailing;
	std::vector<std::string> params;
	parseIrc(message, cmd, params, trailing);
	
	if (!trailing.empty())
	{
		params.push_back(trailing);
	}
	
	commandHandler(cmd, params, client);
}

void Server::handleClient(int i)
{
	char buffer[BUF_SIZE];
	int bytes = recv(this->pfds[i].fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			return;
		}
		else
		{
			std::cout << "Client disconnected with error: " << strerror(errno) << std::endl;
			close(this->pfds[i].fd);
			removeClient(i);
			return;
		}
	}
	else if (bytes == 0)
	{
		std::cout << "Client disconnected" << std::endl;
		close(this->pfds[i].fd);
		removeClient(i);
		return;
	}
	else
	{
		buffer[bytes] = '\0';
		
		for (size_t j = 0; j < clients.size(); j++)
		{
			if (clients[j]->getFd() == pfds[i].fd)
			{
				clients[j]->inbuf.append(buffer, bytes);
				
				std::string& inputBuffer = clients[j]->inbuf;
				size_t pos = 0;
				
				while ((pos = inputBuffer.find("\r\n")) != std::string::npos || 
					   (pos = inputBuffer.find("\n")) != std::string::npos)
				{
					std::string line = inputBuffer.substr(0, pos);
					if (inputBuffer[pos] == '\r')
					    inputBuffer.erase(0, pos + 2);
					else
					    inputBuffer.erase(0, pos + 1);

					if (!line.empty())
					{
						std::cout << "Processing complete command from client " << clients[j]->getFd() << ": " << line << std::endl;
						commandParser(*clients[j], line);
					}
				}
				break;
			}
		}
	}

}

bool Server::handleClientPollout(int i)
{
	for (size_t j = 0; j < clients.size(); j++)
	{
		if (clients[j]->getFd() == pfds[i].fd && !clients[j]->outbuf.empty())
		{
			int sent = send(pfds[i].fd, clients[j]->outbuf.c_str(), clients[j]->outbuf.size(), 0);
			if (sent > 0)
			{
				clients[j]->outbuf.erase(0, sent);
			}
			else if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
			{
				std::cout << "Send error: " << strerror(errno) << std::endl;
				close(pfds[i].fd);
				removeClient(i);
				return true;
			}
			break;
		}
	}
	return false;
}


void Server::start(int port, const char *pass)
{
	this->password = pass;

	struct sockaddr_in hints;
	std::memset(&hints, 0, sizeof(hints));
	initServer(hints, port);

	while (this->running)
	{
		if (poll(this->pfds, this->num_of_pfd, -1) < 0)
		{
			if (!this->running)
				break;
			throw std::exception();
		}

		if (this->pfds[0].revents & POLLIN)
		{
			Client *cl = new Client;
			socklen_t len = sizeof(cl->in_soc);

			int client_fd = accept(this->serverFd, (struct sockaddr*)&(cl->in_soc), &len);
			cl->setFd(client_fd);
			if (client_fd >= 0)
			{
				setNonBlocking(cl->getFd());

				this->pfds[this->num_of_pfd].fd = cl->getFd();
				this->pfds[this->num_of_pfd].events = POLLIN | POLLOUT;
				this->num_of_pfd++;
				this->clients.push_back(cl);
				
				std::string welcome = "Hello World!\n";
				send(cl->getFd(), welcome.c_str(), welcome.size(), 0);

				char ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &cl->in_soc.sin_addr, ip, sizeof(ip));
				std::cout << "New Connection : " << ip << std::endl;
			}
		}
		
		for (int i = 1; i < this->num_of_pfd; i++)
		{
			if (this->pfds[i].revents & POLLIN)
				handleClient(i);
			if (this->pfds[i].revents & POLLOUT)
			{
				if (handleClientPollout(i))
					i--;
			}
		}
	}
	close(this->serverFd);
}


