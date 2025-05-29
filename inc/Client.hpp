#pragma once

#include "../inc/StandardLibraries.hpp"
#include "Structs.hpp"

class Client {
public:
	Client(ServerConfig config, int fd = -1);
	Client(const Client &source) = default;
	Client &operator=(const Client &source) = default;

	void recvFrom(int epoll_fd);
	void sendTo(int epoll_fd);

private:
	int				fd;
	ServerConfig	config;
	std::string		recv_buf;
	std::string		send_buf;
	std::vector<std::string> recv_queue;
	std::vector<std::string> send_queue;
};
