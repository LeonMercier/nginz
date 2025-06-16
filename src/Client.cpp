
#include "../inc/Client.hpp"

Client::Client(std::vector<ServerConfig> configs, int epoll_fd, int fd) :
	configs(configs),
	epoll_fd(epoll_fd),
	fd(fd)  {
	state = IDLE;
}

t_client_state Client::getState() {
	return state;
}
void Client::setState(t_client_state state) {
	this->state = state;
}


bool isCompleteHeader(std::string request) {
	if (request.find("\r\n\r\n") != std::string::npos) {
		//std::cout << "found end of request" << std::endl;
		return true;
	}
	return false;
}

static void changeEpollMode(int epoll_fd, int client_fd, uint32_t mode) {
	struct epoll_event e_event{};
	e_event.events = mode;
	e_event.data.fd = client_fd;
	// note: epoll_ct_MOD, not epoll_ctl_ADD
	if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &e_event) < 0) {
		throw std::runtime_error("epoll_ctl() failed");
	}
}

// Client object is erased in event_loop() after the state is checked
void Client::closeConnection(int epoll_fd, int client_fd) {
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0) {
		throw std::runtime_error("epoll_ctl() failed");
	}

	if (close(client_fd) < 0) {
		throw std::runtime_error("failed to close() client_fd");
	}
	state = DISCONNECT;
}

// TODO: 408: timeout
// TODO: 411 => disconnect?
// TODO: 413 => disconnect?
void Client::handleCompleteRequest(
	ServerConfig config, 
	size_t header_end,
	size_t body_length,
	int status)
{
	// std::cout << "##### RECEIVED REQUEST #####" << std::endl;
	// std::cout << recv_buf.substr(0, header_end + body_length)<< std::endl;
	// std::cout << "############################" << std::endl;
	
	// TODO: sometimes the connection is closed without sending anything
	// back? So we cannot always toggle to EPOLLOUT?
	 
	std::string whole_req = cur_request.raw_request.substr(
		0, header_end + body_length);
	cur_request.raw_request.erase(0, header_end + body_length);
	cur_request.response = getResponse(whole_req, config, status);
	send_queue.push_back(cur_request);
	
	// this will cause the event loop to trigger sendTo()
	changeEpollMode(epoll_fd, fd, EPOLLOUT);
}

void Client::handlePost(
	ServerConfig config,
	size_t header_end)
{
	// if there is transfer-encoding, then content-length can be ignored
	if (cur_request.header.find("transfer-encoding") != cur_request.header.end()) {
		if (cur_request.header.at("transfer-encoding") == "chunked\r") {
			std::cout << "receiving chunked transfer" << std::endl;
			// TODO: we may already have received everything here
			setState(RECV_CHUNKED);
		}
	} else if (cur_request.header.find("Content-Length") == cur_request.header.end()) {
		handleCompleteRequest(config, header_end, 0, 411);
	} else {
		size_t content_length = 0;
		try {
			content_length = std::stoul(cur_request.header.at("Content-Length"));
		} catch (...) {
			std::cerr << "Client::handlePost(): failed to find Content-Length";
			std::cerr << std::endl;
		}
		if (config.client_max_body_size != 0
			&& content_length > config.client_max_body_size) {
			std::cerr << "Client body length larger than allowed" << std::endl;
			handleCompleteRequest(config, header_end, 0, 413);
			// TODO: do we want to continue receiving?
		} else if (cur_request.raw_request.length() >= header_end + content_length) {
			std::cout << "Buf: " << cur_request.raw_request.length();
			std::cout << " target: " << header_end << " + " << content_length;
			std::cout << std::endl;
			handleCompleteRequest(config, header_end, content_length, 0);
		}
		// if recv_buf is smaller than header + content length, we need to 
		// receive more => we hall out of this function without doing anything
	}
	
}

// A client can have multiple configs, but request can only match one config
// TODO: incomplete header -> timeout
void Client::recvFrom() {
	//std::cout << "entered recvFrom" << std::endl;
	char buf[2000] = {0};
	std::string header_terminator = "\r\n\r\n";

	int bytes_read = recv(fd, buf, sizeof(buf) -1, MSG_DONTWAIT);
	// std::cout << "partial receive" << std::endl;

	if (bytes_read < 0) {
		// TODO: currently not throwing here because maybe this is not a 
		// fatal error
		std::cerr << "recvFrom() returned -1" << std::endl;
		return ;
	}
	// this is possibly correct, i.e. EPOLLHUP is not needed because there is
	// an event anyway when the client closes the connection
	if (bytes_read == 0) {
		std::cout << "recvFrom(): read 0 bytes" << std::endl;
		closeConnection(epoll_fd,fd);
		return ;
	}

	std::cout << std::string(buf, bytes_read) << std::endl;
	std::cout << "###" << std::endl;

	cur_request.raw_request += std::string(buf, bytes_read);

	std::cout << "recvFrom: read " << bytes_read << " bytes" << std::endl;
	if (isCompleteHeader(cur_request.raw_request)) {
		// t_method method = getRequestMethod(recv_buf);
		size_t header_end =
			cur_request.raw_request.find(header_terminator) + header_terminator.length();

		cur_request.header =
			parseHeader(cur_request.raw_request.substr(0, header_end));
		if (cur_request.header.find("Host") == cur_request.header.end()) {
			std::cerr << "Client::recvFrom(): no host field" << std::endl;
			// TODO : end of chunked transfer looks like end of header,
			// therefore no host field
		}

		// According to subject, the first server in the config for a 
		// particular host:port will be the default that is used for 
		// requests that dont have server names match

		// in case there is no match, uses the first config
		ServerConfig config = configs.front();
		// at this point all the configs we have have identical host+port
		if (cur_request.header.find("Host") != cur_request.header.end()) {
			for (auto it = configs.begin(); it != configs.end(); it++) {
				// select first config where header host field matches 
				// servername
				// TODO: have this in a try in case map::at() fails
				// TODO: pick first match instead of last
				for (auto itt = it->server_names.begin();
					itt != it->server_names.end(); itt++) {
					if (cur_request.header.at("Host") == *itt) {
						config = *it;
						break ;
					}
				}
			}
		}

		// TODO: .at() will throw if key is not found => catch => invalid request
		if (cur_request.header.at("method") == "GET") {
			handleCompleteRequest(config, header_end, 0, 0);

		} else if (cur_request.header.at("method") == "POST") {
			handlePost(config, header_end);
		}
	}
}


void Client::sendTo() {
	std::cout << "entered sendTo()" << std::endl;
/* 	if (send_buf.empty()) {
		if (send_queue.size() > 0) {
			send_buf = send_queue.front().response.full_response;
		} else {
			std::cerr << "sendTo() called with empty send_queue" << std::endl;
		}
	} */
	if (send_queue.size() < 1) {
		std::cerr << "sendTo() called with empty send_queue" << std::endl;
		return ;
	}
	std::string &to_send = send_queue.front().response.full_response;
	int bytes_sent = send(fd, to_send.c_str(), to_send.length(), MSG_NOSIGNAL);
	if (bytes_sent < 0) {
	// TODO: currently not throwing here because maybe this is not a fatal error
		std::cerr << "send() returned -1" << std::endl;
	}
	//std::cout << "sent " << bytes_sent << " bytes" << std::endl;
	to_send.erase(0, bytes_sent);

	std::string conn_type = "keep-alive";
	try { //TODO: lowercase connection?
		conn_type = send_queue.front().header.at("Connection");
	} catch (...) {
		std::cerr << "Client::sendTo(): no Connection field in header" << std::endl;
	}

	if (to_send.empty()) {
		send_queue.erase(send_queue.begin());
		if (send_queue.size() == 0) {
			// we have sent all responses in the queue
			if (conn_type == "close" ) {
				closeConnection(epoll_fd, fd);
			} else { // nothing left to send
				changeEpollMode(epoll_fd, fd, EPOLLIN);
			}
		}
	}
}
