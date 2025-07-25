#pragma once

#include "Client.hpp"
#include "Structs.hpp"
#include "Signal.hpp"

const static int _CLIENT_TIMEOUT_S = 10;

int		eventLoop(std::vector<ServerConfig>);
void	handle_signal(int sig);
