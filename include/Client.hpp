#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
private:
    int _fd;
    std::string Nick;
    std::string Username;
    std::string Realname;
    bool is_authenticated;
    bool is_registered;

public:
    Client(int fd);
    ~Client();
    
    int getFd();
    const std::string& getNickname();
    void setNickname(const std::string& nick);
    bool isAuthenticated();
    void setAuthenticated(bool auth);
    void sendMessage(const std::string& message);
};

#endif