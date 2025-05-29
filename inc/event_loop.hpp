#pragma once

#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <map>
#include <strings.h>

#include "../inc/Client.hpp"

int eventLoop(void);
