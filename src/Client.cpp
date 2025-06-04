
#include "../inc/Client.hpp"
#include "../inc/request_handler.hpp"
#include "../inc/Structs.hpp"

Client::Client(ServerConfig config, int epoll_fd, int fd) :
	config(config),
	epoll_fd(epoll_fd),
	fd(fd)  {}


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

static void closeConnection(int epoll_fd, int client_fd) {
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0) {
		throw std::runtime_error("epoll_ctl() failed");
	}

	if (close(client_fd) < 0) {
		throw std::runtime_error("failed to close() client_fd");
	}
}

static t_method getRequestMethod(std::string request) {
	std::istringstream	iss(request);
	std::string			method;

	iss >> method;
	if (method == "GET") {
		return GET;
	} else if (method == "POST") {
		return POST;
	} else {
		return ERROR;
	}
}

void Client::handleCompleteRequest(int end) {
	std::cout << "##### RECEIVED REQUEST #####" << std::endl;
	std::cout << recv_buf << std::endl;
	std::cout << "############################" << std::endl;
	
	// TODO: sometimes the connection is closed without sending anything
	// back? So we cannot always toggle to EPOLLOUT?
	 
	recv_queue.push_back(recv_buf.substr(0, end));
	recv_buf.erase(0, end);
	struct Response response = getResponse(recv_queue.front(), config);
	send_queue.push_back(response.full_response);
	recv_queue.erase(recv_queue.begin());
	
	recv_buf = "";
	// this will cause the event loop to trigger sendTo()
	changeEpollMode(epoll_fd, fd, EPOLLOUT);
}

void Client::recvFrom() {
	// sleep(1);
	//std::cout << "entered recvFrom" << std::endl;
	char buf[20] = {0};
	std::string header_end = "\r\n\r\n";

	int bytes_read = recv(fd, buf, sizeof(buf) -1, MSG_DONTWAIT);
	// std::cout << "partial receive" << std::endl;
	// std::cout << recv_buf << std::endl;

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
		closeConnection(epoll_fd, fd);
		return ;
	}

	recv_buf += std::string(buf, bytes_read);
	std::cout << "recvFrom: read " << bytes_read << " bytes" << std::endl;
	if (isCompleteHeader(recv_buf)) {
		t_method method = getRequestMethod(recv_buf);
		size_t end = recv_buf.find(header_end) + header_end.length();

		if (method == GET) {
			handleCompleteRequest(end);

		} else if (method == POST) {
			// TODO: would it be ok to try to receive everything and then
			// later check if POST is even allowed
			int content_length = getPostContentLength(recv_buf);
			if (recv_buf.length() >= end + content_length) {
				std::cout << "Buf: " << recv_buf.length() << " target: "\
					<< end << " + " << content_length << std::endl;
				handleCompleteRequest(end);
			}
			// if recv_buf is smaller than header + content length, we need to 
			// receive more
		}
	}
}

// TODO: do not close when keepalive
void Client::sendTo() {
	std::cout << "entered sendTo()" << std::endl;
	if (send_buf.empty()) {
		if (send_queue.size() > 0) {
			send_buf = send_queue.front();
		} else {
			std::cerr << "sendTo() called with empty send_queue" << std::endl;
		}
	}
	int bytes_sent = send(fd, send_buf.c_str(), send_buf.length(), MSG_NOSIGNAL);
	if (bytes_sent < 0) {
	// TODO: currently not throwing here because maybe this is not a fatal error
		std::cerr << "send() returned -1" << std::endl;
	}
	//std::cout << "sent " << bytes_sent << " bytes" << std::endl;
	send_buf.erase(0, bytes_sent);

	// we have sent all responses in the queue
	if (send_buf.empty()) {
		send_queue.erase(send_queue.begin());
		// TODO: we can close of change mode but not both (cannot change mode
		// on a closed connection)
		closeConnection(epoll_fd, fd);
		// changeEpollMode(epoll_fd, fd, EPOLLIN);
	}
}
