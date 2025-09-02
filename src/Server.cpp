#include "../include/Server.hpp"

Server::Server()
{
	this->serverFd = 0;
	this->num_of_pfd = 0;
}



Server::~Server()
{
    
}

void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) flags = 0;

	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) 
		throw(std::runtime_error("Failed while setting socket non-blocking."));
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
	for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		if ((*it)->getFd() == pfds[index].fd)
		{
			delete *it;
			clients.erase(it);
			break;
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
	
	// Trailing parametresi varsa params'a ekle
	if (!trailing.empty()) {
		params.push_back(trailing);
	}
	
	commandHandler(cmd, params, client);
}

void Server::handleClient(int i)
{
	char buffer[BUF_SIZE];//buffer yönetimine bak
	int bytes = recv(this->pfds[i].fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)//bunun sayesinde halletti
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
		std::string message(buffer);
		std::cout << "Received: " << buffer << std::endl;
		
		// search for client and pass it to command parser
		for (size_t j = 0; j < clients.size(); j++)
		{
			if (clients[j]->getFd() == pfds[i].fd)
			{
				commandParser(*clients[j], message);
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
				return true; // removed
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
	initServer(hints, port);

	while (true)
	{
		if (poll(this->pfds, this->num_of_pfd, -1) < 0)
			throw std::exception();

		// yeni connection olup olmadigini kontrol et.
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
				this->pfds[this->num_of_pfd].events = POLLIN | POLLOUT;//pollout durumuna da baktı
				this->num_of_pfd++;
				this->clients.push_back(cl);
				
				std::string welcome = "Hello World!\n";
				send(cl->getFd(), welcome.c_str(), welcome.size(), 0);

				char ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &cl->in_soc.sin_addr, ip, sizeof(ip));
				std::cout << "New Connection : " << ip << std::endl;
			}
		}
		
		// surekli pollfdnin icindeki clientler veri gonderiyor mu onu kontrol et
		for (int i = 1; i < this->num_of_pfd; i++)
		{
			if (this->pfds[i].revents & POLLIN)// girdi durumunda clientleri ayarlıyor
				handleClient(i);
			if (this->pfds[i].revents & POLLOUT)// çıktı durumunda clientleri ayarlıyor
			{
				// kullanıcıya not; true döndürdüğü zaman clientın kaldırıldığı anlaşılmalı, buna göre i düzenlenmeli
				if (handleClientPollout(i))
					i--;
			}
		}
	}
	close(this->serverFd);
}