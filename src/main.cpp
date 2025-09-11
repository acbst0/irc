/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: fkuyumcu <fkuyumcu@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/11 13:22:01 by fkuyumcu          #+#    #+#             */
/*   Updated: 2025/09/11 13:22:02 by fkuyumcu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Server.hpp"
#include "../include/libs.hpp"
#include "../include/Client.hpp"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

Server* g_server = NULL;

void signalHandler(int signal)
{
	(void)signal;
	if (g_server)
		g_server->stop();
}

bool validate(const std::string &portS)
{
	if (portS.empty())
		return false;
	
	for (size_t i = 0; i < portS.size(); ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(portS[i])))
			return false;
	}
	
	errno = 0;
	char *endptr = NULL;
	long val = std::strtol(portS.c_str(), &endptr, 10);
	
	if (errno == ERANGE)
		return false;
	if (endptr == portS.c_str() || *endptr != '\0')
		return false;
	if (val < 1024 || val > 65535)
		return false;

	return true;
}


int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: ./irc <port> <password>" << std::endl;
		return 1;
	}
	std::string password = argv[2];

	if (password.empty())
	{
		std::cerr << "Password cannot be empty." << std::endl;
		return 1;
	}

	if (!validate(argv[1]))
	{
		std::cerr << "Invalid port. Provide a numeric port within range 1024-65535." << std::endl;
		return 1;
	}
   
	signal(SIGINT, signalHandler); 
	signal(SIGTERM, signalHandler);
	signal(SIGQUIT, signalHandler);
	
	try
	{
		Server server;
		g_server = &server;

		server.start(std::atoi(argv[1]), argv[2]);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		g_server = NULL;
		return 1;
	}
	
	g_server = NULL;
	return 0;
}
