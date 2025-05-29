#include "../inc/Client.hpp"
#include "../inc/request_handler.hpp"

static std::string getResponse() {
	std::string body = "<!DOCTYPE html><html><head><title>First Web Page</title></head><body>Hello Maria!</body></html>";
	//#include "marquee.cpp"
	std::string header = "HTTP/1.1 200 OK\nDate: Thu, 26 May 2025 10:00:00 GMT\nServer: Apache/2.4.41 (Unix)\nContent-Type: text/html; charset=UTF-8\nContent-Length:" + std::to_string(body.length()) + "\nSet-Cookie: session=some-session-id; Path=/; HttpOnly\nCache-Control: no-cache, private \n\n ";
	return header + body;
}

Client::Client(int fd) : fd(fd)  {}

bool isCompleteRequest(std::string request) {
	return request.length() > 0;
}

void Client::recvFrom() {
	char buf[1000] = {0};

	int bytes_read = recv(fd, buf, sizeof(buf) -1, MSG_DONTWAIT);
	recv_buf += std::string(buf, bytes_read);
	std::cout << recv_buf << std::endl;
	if (isCompleteRequest(recv_buf)) {
		std::string response = getResponse(recv_buf);
		sendTo(response);
	}
}

void Client::sendTo(std::string response) {
//	std::string response = getResponse();
	send(fd, response.c_str(), response.length(), MSG_NOSIGNAL); 
}
