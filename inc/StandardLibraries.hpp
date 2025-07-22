#pragma once


#include <algorithm>
#include <asm-generic/socket.h>
#include <atomic>
#include <cstddef> // for size_t?
#include <cstdio> // remove
#include <csignal>
#include <exception>
#include <fcntl.h> // fcntl() // only for MacOS?
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <netdb.h> // struct addrinfo
#include <netinet/in.h>
#include <regex>
#include <sstream> // for istrinstream
#include <stdexcept>
#include <string>
#include <string.h> //memset
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/wait.h>
// #include <sys/types.h>
#include <unistd.h> // 
#include <vector>

