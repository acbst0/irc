/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fkuyumcu <fkuyumcu@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/11 13:22:30 by fkuyumcu          #+#    #+#             */
/*   Updated: 2025/09/11 13:22:46 by fkuyumcu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP

# define SERVER_HPP

# include <iostream>
# include <cstring>
# include <cstdlib>
# include <unistd.h>
# include <fcntl.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <poll.h>
# include <errno.h>
# include <sstream>
# include <vector>
# include <map>
# include <set>
# include <string>
# include <cstdio>
# include <cerrno>
# include <exception>
# include <algorithm>
# include <cctype>
# include "Client.hpp"
# include "Channel.hpp"

# define BACKLOG 128
# define BUF_SIZE 1024


class Server
{
	private:
		int port;
		std::string password;
		std::map<std::string, Channel*> channels;
		int serverFd;
		struct pollfd pfds[BACKLOG + 1];
		int num_of_pfd;
		std::vector<Client *> clients;
		bool running;
	
	public:
		Server();
		~Server();
	
		void commandParser(Client &client, std::string &message);
		void handleClient(int index);
		bool handleClientPollout(int index);
		void initServer(struct sockaddr_in &hints, int port);
		void start(int port, const char *pass);
		void stop();
		void removeClient(int index);
		void commandHandler(std::string cmd, std::vector<std::string> params, Client &client);
		void checkRegistration(Client &client);
		bool nicknameCheck(std::string nickname);
		void handleJoin(const std::vector<std::string>& params, Client &client);
		void handlePart(const std::vector<std::string>& params, Client &client);
		void handlePrivMsg(const std::vector<std::string>& params, Client &client);
		void handleNotice(const std::vector<std::string>& params, Client &client);
		void handleQuit(const std::vector<std::string>& params, Client &client);

		void handleMode(const std::vector<std::string>& params, Client &client);
		void handleTopic(const std::vector<std::string>& params, Client &client);
		void handleName(const std::vector<std::string>& params, Client &client);
		void handleList(const std::vector<std::string>& params, Client &client);
		void handleInvite(const std::vector<std::string>& params, Client &client);
		void handleKick(const std::vector<std::string>& params, Client &client);
		void handleWho(const std::vector<std::string>& params, Client &client);
		void handleWhois(const std::vector<std::string>& params, Client &client);
		void handleAway(const std::vector<std::string>& params, Client &client);
		
};

void parseIrc(const std::string& line, std::string& cmd, std::vector<std::string>& params, std::string& trailing);
void enqueue(std::string &outbuf, const std::string& line);
std::string to_string(int number);

#endif
