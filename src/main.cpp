#include "../include/Server.hpp"
#include "../include/libs.hpp"
#include "../include/Client.hpp"
//#include "../include/Channel.hpp"


bool validate(const std::string &portS)
{
    if (portS.empty())
        return false;
    
    for (unsigned char c : portS)
    {
        if (!std::isdigit(c))
            return false;
    }
    
    errno = 0;
    char *endptr = nullptr;
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

    if (!validate(argv[1]))
    {
		std::cerr << "Invalid port. Provide a numeric port within range 1024-65535." << std::endl;
        return 1;
    }
   
    
    try {
        Server server;

        server.start(std::atoi(argv[1]), argv[2]);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
