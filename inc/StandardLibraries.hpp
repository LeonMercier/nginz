#pragma once


#include <algorithm>
#include <asm-generic/socket.h>
#include <cstddef> // for size_t?
#include <exception>
#include <fcntl.h> // fcntl() // only for MacOS?
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sstream> // for istrinstream
#include <stdexcept>
#include <string>
//#include <strings.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h> // sleep()
#include <vector>
