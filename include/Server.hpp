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

# define BACKLOG 10
# define BUF_SIZE 1024

//class Client;
//class Channel;

class Server {
	private:
	    //int port;
	    //std::string password;
	    int serverFd;
	    //std::vector<Client*> clients;
	    //std::map<std::string, Channel*> channels;
	
	public:
	    Server();
	    ~Server();
	
	    void start(const char *port, const char *pass);
	    //void acceptClient();
	    //void handleClientMessage(int client_fd, const std::string& message);
};

#endif
