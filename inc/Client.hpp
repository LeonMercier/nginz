#pragma once

#include "../inc/StandardLibraries.hpp"
#include "Structs.hpp"
#include "../inc/parse_header.hpp"
#include "../inc/request_handler.hpp"

typedef struct s_resp {
	std::map<std::string, std::string>	req_header;
	Response							response;
} t_resp;
	

typedef enum e_client_state {
	IDLE,
	DISCONNECT,
	CLIENT_ERROR
} t_client_state;

class Client {
public:
	Client(std::vector<ServerConfig> configs, int epoll_fd = -1, int fd = -1);
	Client(const Client &source) = default;
	Client &operator=(const Client &source) = default;

	void recvFrom();
	void sendTo();
	t_client_state getState();
	void handlePost(
		ServerConfig config,
		std::map<std::string, std::string> &header_map,
		size_t header_end);

	void handleCompleteRequest(
		ServerConfig config,
		std::map<std::string, std::string> header_map,
		size_t header_end,
		size_t body_length,
		int status);

	void closeConnection(int epoll_fd, int client_fd);

private:
	std::vector<ServerConfig>	configs;
	int				epoll_fd;
	int				fd;
	t_client_state	state;
	std::string		recv_buf;
	// std::string		send_buf;
	// std::vector<std::string> recv_queue;
	std::vector<t_resp> send_queue;
};

typedef enum e_method {
	GET,
	POST,
	DELETE,
	ERROR
}	t_method;

