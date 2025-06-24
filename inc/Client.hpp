#pragma once

#include "StandardLibraries.hpp"
#include "Structs.hpp"
#include "parse_header.hpp"
//#include "request_handler.hpp"
#include "Request.hpp"
#include "CgiHandler.hpp"

typedef struct s_rsp {
	std::map<std::string, std::string>	header;
	Response							response;
} t_rsp;
	

typedef enum e_client_state {
	IDLE,
	RECV_CHUNKED,
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


private:
	std::vector<ServerConfig>	configs;
	int							epoll_fd;
	int							fd;
	t_client_state				state;
	std::vector<t_rsp>			send_queue;
	Request 					request;
};
