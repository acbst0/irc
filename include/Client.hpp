#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <netinet/in.h>

class Client {
	private:
	    int fd;
	    std::string nick;
	    std::string username;
	    bool is_authenticated;
	    bool is_registered;
		
		public:
		Client();
	    Client(int _fd);
	    ~Client();
		
		struct sockaddr_in in_soc;
		std::string outbuf;

	    int getFd();
		void setFd(int _fd);
		bool getAuth();
		void setAuth(bool i);
		bool getRegis();
		void setRegis(bool i);
		std::string getNick();
		std::string getUname();
		void setNick(std::string nick);
		void setUname(std::string username);

	    //void sendMessage(const std::string& _message);
};

#endif