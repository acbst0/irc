#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <netinet/in.h>

class Client
{
	private:
	    int fd;
	    std::string nick;
	    std::string username;
		std::string realname;
		std::string hostname;
	    bool is_authenticated;
	    bool is_registered;
	    bool away;
	    std::string awayMessage;
		
		public:
		Client();
	    Client(int _fd);
	    ~Client();
		
		struct sockaddr_in in_soc;
		std::string outbuf;
		std::string inbuf;

	    int getFd();
		void setFd(int _fd);
		bool getAuth();
		void setAuth(bool i);
		bool getRegis();
		void setRegis(bool i);
		std::string getNick();
		std::string getRname();
		std::string getUname();
		std::string getHname();
		void setNick(std::string nick);
		void setUname(std::string username);
		void setRname(std::string realname);
		void setHname(std::string hostname);
		bool isAway();
		void setAway(bool status);
		std::string getAwayMessage();
		void setAwayMessage(std::string message);

};

#endif