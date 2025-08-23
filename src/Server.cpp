#include "../include/Server.hpp"

Server::Server()
{
	this->serverFd = 0;
	this->num_of_pfd = 0;
}



Server::~Server()
{
    
}

void setNonBlocking(int fd) {
    // Mevcut bayrakları al
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) flags = 0;
    // O_NONBLOCK bitini ekleyerek non-blocking yap
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::start(const char *port, const char *pass)
{
	// PASS IS USELESS RIGHT NOW
	(void *)pass;

	struct addrinfo hints;
	struct addrinfo *res;
	
	std::cout << "CHECK -> HERE" << std::endl;
	std::memset(&hints, 0 , sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, port, &hints, &res) != 0)
	{
		std::cerr << "ADRINFO ERROR" << std::endl;
		throw std::exception();
	}

	this->serverFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (this->serverFd < 0)
	{
		std::cerr << "SOCKET ERROR" << std::endl;
		throw std::exception();
	}

	int yes = 1;
	setsockopt(this->serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	if (bind(this->serverFd, res->ai_addr, res->ai_addrlen) < 0)
	{
		std::cerr << "BIND ERROR" << std::endl;
		throw std::exception();
	}

	freeaddrinfo(res);

	if (listen(this->serverFd, BACKLOG) < 0)
	{
		std::cerr << "LISTEN ERROR" << std::endl;
		throw std::exception();
	}

	setNonBlocking(this->serverFd);

	std::cout << "IRC Server Has Been Running!" << std::endl;

	this->pfds[0].fd = this->serverFd;
	this->pfds[0].events = POLLIN;
	this->num_of_pfd++;
	while (true)
	{
		if (poll(this->pfds, this->num_of_pfd, -1) < 0)
			throw std::exception();

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
				this->pfds[this->num_of_pfd].events = POLLIN;
				this->num_of_pfd++;
				this->clients.push_back(cl);
				
				std::string welcome = "Hello World!\n";
				send(cl->getFd(), welcome.c_str(), welcome.size(), 0);

				char ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &cl->in_soc.sin_addr, ip, sizeof(ip));
				std::cout << "New Connection : " << ip << std::endl;
			}
		}
	}

	close(this->serverFd);
}