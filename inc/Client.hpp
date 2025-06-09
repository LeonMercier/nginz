#pragma once

#include "../inc/StandardLibraries.hpp"
#include "Structs.hpp"

class Client {
public:
	Client(ServerConfig config, int epoll_fd = -1, int fd = -1);
	Client(const Client &source) = default;
	Client &operator=(const Client &source) = default;

	void recvFrom();
	void sendTo();
	void handleCompleteRequest(int, int);

private:
	ServerConfig	config;
	int				epoll_fd;
	int				fd;
	std::string		recv_buf;
	std::string		send_buf;
	std::vector<std::string> recv_queue;
	std::vector<std::string> send_queue;
};

typedef enum e_method {
	GET,
	POST,
	DELETE,
	ERROR
}	t_method;
