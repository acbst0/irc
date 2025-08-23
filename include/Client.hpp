#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <netinet/in.h>

class Client {
	private:
	    int fd;
	    //std::string nick;
	    //std::string username;
	    //std::string realname;
	    //bool is_authenticated;
	    //bool is_registered;
		
	public:
		Client();
	    Client(int _fd);
	    ~Client();
		
		struct sockaddr_in in_soc;

	    int getFd();
		void setFd(int _fd);
	    //const std::string& getNickname();
	    //void setNickname(const std::string& _nick);
	    //bool isAuthenticated();
	    //void setAuthenticated(bool _auth);
	    //void sendMessage(const std::string& _message);
};

#endif