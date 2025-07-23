#pragma once

#include "Webserv.hpp"
#include "parse_header.hpp"
#include "Request.hpp"
#include "CgiHandler.hpp"
	

typedef enum e_client_state {
	IDLE,
	RECV,
	SEND,
	RECV_CHUNKED,
	WAIT_CGI,
	TIMEOUT,
	DISCONNECT,
	CLIENT_ERROR
} t_client_state;

class Client {
public:

	CgiHandler cgi;

	// calls Request constructor with configs
	Client(std::vector<ServerConfig> configs, int epoll_fd = -1, int fd = -1);
	Client(const Client &source) = default;
	Client &operator=(const Client &source) = default;

	t_client_state	getState();
	void			setState(t_client_state state);
	void			changeEpollMode(uint32_t mode);
	void			closeConnection(int epoll_fd, int client_fd);
	void			recvFrom();
	void			sendTo();
	void			updateLastEvent();
	time_t			getLastEvent();
	int				getClientFd();

	std::vector<Response>		send_queue;
	Request 					request;


private:
	std::vector<ServerConfig>	configs;
	int							epoll_fd;
	int							fd;
	t_client_state				state;
	std::string					_to_send;
	
	time_t						_last_event;
};
