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
    int _port;
    std::string _password;
    int _server_fd;
    std::vector<Client*> _clients;
    std::map<std::string, Channel*> _channels;
    
public:
    Server(int port, const std::string& password);
    ~Server();
    
    void start();
    void acceptClient();
    void handleClientMessage(int client_fd, const std::string& message);
};


#endif


