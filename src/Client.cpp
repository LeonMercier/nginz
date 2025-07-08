
#include "../inc/Client.hpp"
#include "../inc/utils.hpp"

Client::Client(std::vector<ServerConfig> configs, int epoll_fd, int fd) :
	request(configs),
	configs(configs),
	epoll_fd(epoll_fd),
	fd(fd)
{
	state = IDLE;
}

t_client_state Client::getState() {
	return state;
}

int	Client::getClientFd(){
	return fd;
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
		throw std::runtime_error("changeEpollMode: epoll_ctl() failed");
	}
}

time_t Client::getLastEvent(){
	return _last_event;
}

// Client object is erased in event_loop() after the state is checked
void Client::closeConnection(int epoll_fd, int client_fd) {
	std::cout << "CLIENT_FD: " << client_fd << std::endl;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) < 0) {
		if (errno == EBADF)
		{
			std::cout << "SHITTY FD\n";
		}
		else if (errno == EINVAL)
		{
			std::cout << "SHITTY EPOLLFD\n";
		}
		throw std::runtime_error("closeConnection: epoll_ctl() failed");
	}

	if (close(client_fd) < 0) {
		throw std::runtime_error("failed to close() client_fd");
	}
	std::cout << "Clientfd " << client_fd << " has been closed\n" ;
	state = DISCONNECT;
}

void	Client::updateLastEvent()
{
	_last_event = std::time(nullptr);
	// std::cout << "Client[" << fd << "] latest event: " << std::asctime(std::localtime(&_last_event)) << std::endl;
}

// TODO: incomplete header -> timeout
void Client::recvFrom() {
	//std::cout << "entered recvFrom" << std::endl;
	if (state == IDLE)
	{
		state = RECV;
	}
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
		// std::cout << "recvFrom(): read 0 bytes" << std::endl;
		state = DISCONNECT;
		return ;
	}

	// std::cout << std::string(buf, bytes_read) << std::endl;
	// std::cout << "###" << std::endl;

	request.addToRequest(std::string(buf, bytes_read));
	if (request.getState() == READY) {
		if (request.getIsCgi()) {
			state = WAIT_CGI;
			cgi.launchCgi(request);
			changeEpollMode(EPOLLOUT);
			return ;
		}
		// TODO: 408: timeout
		// TODO: 411 => disconnect?
		// TODO: 413 => disconnect?
		// TODO: sometimes the connection is closed without sending anything
		// back? So we cannot always toggle to EPOLLOUT?
		 
		send_queue.push_back(request.getRes());

		// reset the request member to an empty state
		request = Request(configs);
		
		// this will cause the event loop to trigger sendTo()
		changeEpollMode(EPOLLOUT);
		state = SEND;
	} else {
		// go back to event loop and wait to receive more
		return ;
	}
}

void Client::sendTo() {
	if (_to_send.length() == 0 && state == WAIT_CGI) {
		// std::cout << "DOING CGI" << std::endl;
		t_cgi_state cgi_result = cgi.checkCgi(); // this has waitpid
		if (cgi_result == CGI_READY) {
			if (request.getMethod() == POST){
				std::remove(request.getPostBodyFilename().c_str());
			}
			try {
				Response tmp;
				tmp.full_response = fileToString(cgi.output_filename);
				std::cout << "deleting file: " << cgi.output_filename << std::endl;
				std::remove(cgi.output_filename.c_str());
				send_queue.push_back(tmp);
			} catch (const std::ios_base::failure& e){
				// TODO how to handle error from client?
			}
				state = SEND;
			return ;
		}
		else if (cgi_result == CGI_TIMEOUT || cgi_result == CGI_FAILED)
		{
			std::cout << "Client::sendTo(): cgi timeout or fail" << std::endl;
			try {
				std::remove(cgi.output_filename.c_str());
			} catch (...){}
			try {
				request.getResponse(504);
				send_queue.push_back(request.getRes());
			} catch (const std::ios_base::failure& e){
				// TODO how to handle error from client?
			}
			state = SEND;
			return ;
		}
		else
		{
			// keep waiting for CGI
			return;
		}
	} else {
		if (send_queue.size() < 1) {
			std::cerr << "sendTo() called with empty send_queue" << std::endl;
			changeEpollMode(EPOLLIN);
			state = IDLE;
			return ;
		}

		if (_to_send.empty()) {
			_to_send = send_queue.front().full_response;
		}

		// std::cout << "SENDING" << to_send <<std::endl;
		int bytes_sent = send(fd, _to_send.c_str(), _to_send.length(), MSG_NOSIGNAL);
		if (bytes_sent < 0) {
		// TODO: currently not throwing here because maybe this is not a fatal error
			std::cerr << "send() returned -1" << std::endl;
		}

		_to_send.erase(0, bytes_sent);

		if (_to_send.empty()) {
			// request.setIsCgi(false);
			send_queue.erase(send_queue.begin());
			if (send_queue.size() == 0) {
				// we have sent all responses in the queue
				if (request.getConnectionTypeIsClose() == true ) {
					state = DISCONNECT;
				} else { // nothing left to send
					changeEpollMode(EPOLLIN);
					request = Request(configs);
					state = IDLE;
				}
			}
		}
	}
}
