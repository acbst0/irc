SRCS = src/main.cpp \
		src/Server.cpp \
		src/Client.cpp \
		src/Utils.cpp \
		src/cmdHandler.cpp \
		src/join.cpp \
		src/Channel.cpp \
		src/privMsg.cpp \
		src/notice.cpp \
		src/quit.cpp \
		src/channelCommands.cpp


CXX = c++ 

RM = rm -rf

FLAGS = -Wall -Wextra -Werror -std=c++98

NAME = irc

O_FILES := $(SRCS:.cpp=.o)

all: $(O_FILES)
	$(CXX) $(SRCS) -o $(NAME)

clean:
	$(RM) $(O_FILES)

fclean : clean
	$(RM) $(NAME)

re: fclean all