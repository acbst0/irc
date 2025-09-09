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
NAME = ircserv
OBJS_DIR = objs

O_FILES = $(SRCS:src/%.cpp=$(OBJS_DIR)/%.o)

all: $(NAME)

$(NAME): $(O_FILES)
	$(CXX) $(FLAGS) $(O_FILES) -o $(NAME)

$(OBJS_DIR):
	mkdir -p $(OBJS_DIR)

$(O_FILES): $(OBJS_DIR)/%.o: src/%.cpp | $(OBJS_DIR)
	$(CXX) $(FLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS_DIR)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re