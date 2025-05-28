#include <iostream>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <map>
#include <strings.h>

std::string getResponse() {
	std::string body = "<!DOCTYPE html><html><head><title>First Web Page</title></head><body>Hello Maria!</body></html>";
	//#include "marquee.cpp"
	std::string header = "HTTP/1.1 200 OK\nDate: Thu, 26 May 2025 10:00:00 GMT\nServer: Apache/2.4.41 (Unix)\nContent-Type: text/html; charset=UTF-8\nContent-Length:" + std::to_string(body.length()) + "\nSet-Cookie: session=some-session-id; Path=/; HttpOnly\nCache-Control: no-cache, private \n\n ";
	return header + body;
}

int mvp(void)
{
	int epoll_fd = epoll_create1(0);
	int socket_fd = socket(AF_INET, (SOCK_STREAM | SOCK_NONBLOCK), 0);

	// Set the address for the socket
	sockaddr_in serv_addr{};				// Struct for defining socket address
	serv_addr.sin_family = AF_INET;			// Tells the socket we're using IPv4.
	serv_addr.sin_port = htons(8080);		// Sets the port number to 8080
	serv_addr.sin_addr.s_addr = INADDR_ANY; // Listen to all interfaces

	// Bind the address to the socket
	bind(socket_fd, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr));
	// Socket is ready to listen to incoming connection requests
	listen(socket_fd, 256); // 256 is the max number of connections

	// Add the socket to the epoll
	struct epoll_event e_event{}; //{} initializes  it with zeroes, not garbage?
	e_event.data.fd = socket_fd;  // The socket itself
	e_event.events = EPOLLIN;	  // Notify me when incoming data is available
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &e_event);

	// apparenty this does NOT need to be zeroed (for ex. manpage example)
	struct epoll_event events[64]; 

	while (true)
	{
		int ready = epoll_wait(epoll_fd, events, 64, -1);
		if (ready == 0 || ready == -1)
			continue;
		for (int i = 0; i < ready; i++) // loop over all new events
		{
			if (events[i].data.fd == socket_fd) // new connection
			{
				struct sockaddr_in client;
				socklen_t size = sizeof(client);
				int client_fd = accept(socket_fd, reinterpret_cast<sockaddr *>(&client), &size);

				// epoll_ctl() takes information from e_event and puts it into
				// the data structures held in the kernel, that is why we can
				// reuse e_event. In other words; the contents of e_event are
				// only relevant for epoll_ctl()
				// Here we add new connection to the interest list
				e_event.events = EPOLLIN | EPOLLET;
				e_event.data.fd = client_fd;
				epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &e_event);
			}
			else // existing connection
			{
				char recv_buf[1000] = {0};
				std::cout << "receiving: socket: " << socket_fd << " client fd: " << events[i].data.fd << std::endl;
				recv(events[i].data.fd, recv_buf, 1000, MSG_DONTWAIT);
				std::cout << recv_buf << std::endl;
				std::string response = getResponse();
				send(events[i].data.fd, response.c_str(), response.length(), MSG_NOSIGNAL); 
			}
		}
	}
	close(e_event.data.fd);
	close(socket_fd);
	close(epoll_fd);
	return 0;
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	// if (argc != 2 || std::string(argv[1]) != "configuration/webserv.conf")
	// {
	// 	std::cout << "\nRun with \"./webserv configuration/webserv.conf\"" << std::endl;
	// 	return (1);
	// }
    mvp();
    return (0);
}
