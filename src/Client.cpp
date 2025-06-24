
#include "../inc/Client.hpp"

Client::Client(std::vector<ServerConfig> configs, int epoll_fd, int fd) :
	configs(configs),
	epoll_fd(epoll_fd),
	fd(fd),
	request(configs)
{
	state = IDLE;
}

t_client_state Client::getState() {
	return state;
}

void Client::setState(t_client_state state) {
	this->state = state;
}

void Client::changeEpollMode(uint32_t mode) {
	struct epoll_event e_event{};
	e_event.events = mode;
	e_event.data.fd = fd;
	// note: epoll_ct_MOD, not epoll_ctl_ADD
	if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &e_event) < 0) {
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

// TODO: incomplete header -> timeout
void Client::recvFrom() {
	//std::cout << "entered recvFrom" << std::endl;
	char buf[2000] = {0};

	// TODO:: -1 really needed?
	int bytes_read = recv(fd, buf, sizeof(buf) -1, MSG_DONTWAIT);

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

	// std::cout << std::string(buf, bytes_read) << std::endl;
	// std::cout << "###" << std::endl;

	e_req_state status = request.addToRequest(std::string(buf, bytes_read));
	if (status == READY) {
		//if (request.getIsCGI()) {
		//	launchCGI();		
		//}
		// TODO: 408: timeout
		// TODO: 411 => disconnect?
		// TODO: 413 => disconnect?
		// TODO: sometimes the connection is closed without sending anything
		// back? So we cannot always toggle to EPOLLOUT?
		 
		send_queue.push_back({request.getHeaders(), request.getRes()});

		// reset the request member to an empty state
		request = Request(configs);
		
		// this will cause the event loop to trigger sendTo()
		changeEpollMode(EPOLLOUT);
	} else {
		// go back to event loop and wait to receive more
		return ;
	}
}

void Client::sendTo() {
	if (request.state == WAIT_CGI) {
		cgi.checkCgi(); // this has waitpid
	} else {
		if (send_queue.size() < 1) {
			std::cerr << "sendTo() called with empty send_queue" << std::endl;
			changeEpollMode(EPOLLIN);
			return ;
		}

		std::string to_send = send_queue.front().response.full_response;

		// std::cout << "SENDING" << to_send <<std::endl;
		int bytes_sent = send(fd, to_send.c_str(), to_send.length(), MSG_NOSIGNAL);
		if (bytes_sent < 0) {
		// TODO: currently not throwing here because maybe this is not a fatal error
			std::cerr << "send() returned -1" << std::endl;
		}

		to_send.erase(0, bytes_sent);

		// default mode is keep-alive
		std::string conn_type = "keep-alive";
		try {
			conn_type = send_queue.front().header.at("connection");
		} catch (...) {
			std::cerr << "Client::sendTo(): no Connection field in header"\
				<< std::endl;
		}

		if (to_send.empty()) {
			send_queue.erase(send_queue.begin());
			if (send_queue.size() == 0) {
				// we have sent all responses in the queue
				if (conn_type == "close" ) {
					closeConnection(epoll_fd, fd);
				} else { // nothing left to send
					changeEpollMode(EPOLLIN);
				}
			}
		}
	}
}