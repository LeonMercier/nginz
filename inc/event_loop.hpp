#pragma once

#include "Client.hpp"
#include "Structs.hpp"

const static int _CLIENT_TIMEOUT_S = 100;
int eventLoop(std::vector<ServerConfig>);
