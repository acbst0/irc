SRCS = src/main.cpp \
		src/Server.cpp

CXX = c++

#FLAGS = -Wall -Wextra -Werror

NAME = irc

all:
	$(CXX) $(SRCS) -o $(NAME)

