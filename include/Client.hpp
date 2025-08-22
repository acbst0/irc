#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>

class Client {
private:
    int fd;
    std::string nick;
    std::string username;
    std::string realname;
    bool is_authenticated;
    bool is_registered;

public:
    Client(int _fd);
    ~Client();
    
    int getFd();
    const std::string& getNickname();
    void setNickname(const std::string& _nick);
    bool isAuthenticated();
    void setAuthenticated(bool _auth);
    void sendMessage(const std::string& _message);
};

#endif