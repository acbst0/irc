#ifndef SERVER_HPP

#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <map>
#include <string>

class Client;
class Channel;

class Server {
private:
    int Port;
    std::string Password;
    int ServerFd;
    std::vector<Client*> Clients;
    std::map<std::string, Channel*> Channels;
    
public:
    Server(int port, const std::string& password);
    ~Server();
    
    void start();
    void acceptClient();
    void handleClientMessage(int client_fd, const std::string& message);
};


#endif


