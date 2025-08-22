#include "../include/Server.hpp"
//#include "../include/Client.hpp"
//#include "../include/Channel.hpp"

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }
    
    //int port = std::atoi(argv[1]);
    //std::string password = argv[2];
    
    try {
        Server server;
        server.start(argv[1], argv[2]);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
