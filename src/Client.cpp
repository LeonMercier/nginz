
#include "../inc/Client.hpp"
#include "../inc/request_handler.hpp"

Client::Client(ServerConfig config, int fd) : config(config), fd(fd)  {}


bool isCompleteRequest(std::string request) {
	if (request.find("\r\n\r\n") != std::string::npos) {
		std::cout << "found end of request" << std::endl;
		return true;
	}
	return false;
	// return request.length() > 0;
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

// TODO: only handles GET for now
void Client::recvFrom(int epoll_fd) {
	// sleep(1);
	std::cout << "entered recvFrom" << std::endl;
	char buf[50] = {0};
	std::string header_end = "\r\n\r\n";

	int bytes_read = recv(fd, buf, sizeof(buf) -1, MSG_DONTWAIT);
	// std::cout << "partial receive" << std::endl;
	// std::cout << recv_buf << std::endl;
	recv_buf += std::string(buf, bytes_read);
	if (bytes_read > 0) {
		std::cout << "recvFrom: read " << bytes_read << " bytes" << std::endl;
		if (isCompleteRequest(recv_buf)) {
			std::cout << "##### RECEIVED REQUEST #####" << std::endl;
			std::cout << recv_buf << std::endl;
			std::cout << "############################" << std::endl;
			// send_buf = getResponse(recv_buf);
			//
			// TODO: do we decide, based on the request, whether we want to
			// send anthing back at all? So instead of calling getResponse(),
			// we would call handleRequest() which would then decide whether 
			// to toggle to EPOLLOUT...
			// 
			auto end = recv_buf.find(header_end) + header_end.length();
			recv_queue.push_back(recv_buf.substr(0, end));
			recv_buf.erase(0, end);
			struct Response response = getResponse(recv_queue.front());
			send_queue.push_back(response.full_response);
			recv_queue.erase(recv_queue.begin());
			
			recv_buf = "";
			// this will cause the event loop to trigger sendTo()
			changeEpollMode(epoll_fd, fd, EPOLLOUT);
		}
	} else if (bytes_read == 0) {
		std::cout << "recvFrom(): read 0 bytes" << std::endl;
		// TODO: why does epoll_wait keep triggering when there is nothing to
		// read?
		closeConnection(epoll_fd, fd);
	} else {
		// TODO: currently not throwing here because maybe this is not a 
		// fatal error
		std::cerr << "recvFrom() returned -1" << std::endl;
	}
}

void Client::sendTo(int epoll_fd) {
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
	std::cout << "sent " << bytes_sent << " bytes" << std::endl;
	send_buf.erase(0, bytes_sent);

	// we have sent the whole response
	if (send_buf.empty()) {
		send_queue.erase(send_queue.begin());
		// TODO: we can close of change mode but not both (cannot change mode
		// on a closed connection)
		closeConnection(epoll_fd, fd);
		// changeEpollMode(epoll_fd, fd, EPOLLIN);
	}
}
