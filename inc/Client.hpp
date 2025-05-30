#pragma once

#include <string>
#include <iostream>
// #include <sys/epoll.h>
#include <sys/socket.h>
// #include <netinet/in.h>
// #include <unistd.h>

class Client {
public:
	Client(int fd = -1);

	void recvFrom();
<<<<<<< HEAD
	void sendTo(std::string response);
=======
	void sendTo();
>>>>>>> cd06d02 (move send() and recv() to Client class)

private:
	int			fd;
	std::string recv_buf;
	std::string send_buf;
};
