#ifndef LIBS_HPP
#define LIBS_HPP


//defines

//#define MAXCONN 100
// TODO fkuyumcu : Bu MAXCONN kullanımı gereksiz. Zaten server.hpp de tanımlı

#define RESET   "\033[0m"
#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"


//includes


#if defined(__linux__)
#include <cstdlib>
#include <cstring>
#include <cstdio>
#endif
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <_types/_nl_item.h>
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <iostream>
#include <list>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

#include <vector>
#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cctype>
#include <algorithm>

#endif