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
	if (cmd == "PASS")
	{
		if (params.empty())
		{
			enqueue(client.outbuf, ":server 461 * PASS :Not enough parameters\r\n");
			return;
		}
		client.setAuth(params[0] == this->password);
		enqueue(client.outbuf, client.getAuth() ? ":server NOTICE * :Password accepted\r\n"
										: ":server 464 * :Password required/incorrect\r\n");
		return;
	}
	else if (cmd == "NICK")
	{
	    if (params.empty()) {
	        enqueue(client.outbuf, ":server 431 * :No nickname given\r\n"); // ERR_NONICKNAMEGIVEN
	        return;
	    }
	    std::string nick = params[0];
	
	    // Çakışma kontrolü (örnek: basit map<string,Client*> ile)
	    if (this->clientsByNick.count(nick)) {
	        enqueue(client.outbuf, ":server 433 * " + nick + " :Nickname is already in use\r\n"); // ERR_NICKNAMEINUSE
	        return;
	    }
	
	    client.setNick(nick);
	    this->clientsByNick[nick] = &client;
	    enqueue(client.outbuf, ":server NOTICE * :Nickname set to " + nick + "\r\n");
	    return;
	}
	else if (cmd == "USER")
	{
	    if (params.size() < 4) {
	        enqueue(client.outbuf, ":server 461 * USER :Not enough parameters\r\n"); // ERR_NEEDMOREPARAMS
	        return;
	    }
	
	    // USER <username> <hostname> <servername> :<realname>
	    std::string username = params[0];
	    std::string realname = trailing.empty() ? params.back() : trailing;
	
	    client.setUsername(username);
	    client.setRealname(realname);
	
	    enqueue(client.outbuf, ":server NOTICE * :User registered as " + username + "\r\n");
	    return;
	}
	else if (cmd == "JOIN")
	{

	}
	else if (cmd == "QUIT")
	{
	    std::string quitMsg;
	    if (!trailing.empty())
	        quitMsg = trailing;
	    else if (!params.empty())
	        quitMsg = params[0];
	    else
	        quitMsg = "Client Quit";

	    // Başka kullanıcılara bildir (örnek: sadece kendi nickini gösterelim)
	    std::string fullMsg = ":" + client.getNick() + " QUIT :" + quitMsg + "\r\n";
	    for (std::vector<Client *>::iterator it = this->clients.begin(); it != this->clients.end(); ++it)
		{
			Client *other = *it;
        	if (other && other->getFd() != client.getFd())
        	{
        	    enqueue(other->outbuf, fullMsg);
        	}
	    }

	    // Bu client'a da bilgi gönder (opsiyonel)
	    enqueue(client.outbuf, ":server NOTICE * :Goodbye!\r\n");
		close(client.getFd());
	    return;
	}
}

void Server::handleClient(int i)
{
	char buffer[BUF_SIZE];//buffer yönetimine bak
	int bytes = recv(this->pfds[i].fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes <= 0)
	{
		if (bytes == 0)
			std::cout << "Client disconnected normally" << std::endl;
		else
			std::cout << "Client disconnected with error: " << strerror(errno) << std::endl;
		close(this->pfds[i].fd);
		removeClient(i);
		return;
	}
	else
	{
		buffer[bytes] = '\0';
		std::string message(buffer);
		std::cout << "Received: " << buffer << std::endl;
		
		for (size_t j = 0; j < clients.size(); j++)
		{
			if (clients[j]->getFd() == pfds[i].fd)
			{
				commandParser(*clients[j], message);
				break;
			}
		}
	}

	// Send any pending output
	for (size_t j = 0; j < clients.size(); j++)
	{
		if (clients[j]->getFd() == pfds[i].fd && !clients[j]->outbuf.empty())
		{
			int sent = send(pfds[i].fd, clients[j]->outbuf.c_str(), clients[j]->outbuf.size(), 0);
			if (sent > 0)
			{
				clients[j]->outbuf.erase(0, sent);
			}
			break;
		}
	}
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
		
		// surekli pollfdnin icindeki clientler veri gonderiyor mu onu kontrol et
		for (int i = 1; i < this->num_of_pfd; i++)
		{
			if (this->pfds[i].revents & POLLIN)
				handleClient(i);
			if (this->pfds[i].revents & POLLOUT)
			{
				// Send pending output
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
							i--; // Adjust index since we removed an element
							break;
						}
						break;
					}
				}
			}
		}
	}
	close(this->serverFd);
}