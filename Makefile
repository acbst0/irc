SRCS = src/main.cpp \
		src/Server.cpp \
		src/Client.cpp

CXX = c++

RM = rm -rf

#FLAGS = -Wall -Wextra -Werror

NAME = irc

O_FILES := $(SRCS:.cpp=.o)

all: $(O_FILES)
	$(CXX) $(SRCS) -o $(NAME)

clean:
	$(RM) $(O_FILES)

fclean : clean
	$(RM) $(NAME)

