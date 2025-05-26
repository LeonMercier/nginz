#include <iostream>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <map>

int	mvp(void)
{
	int epoll_fd = epoll_create1(0);
	int socket_fd = socket(AF_INET, (SOCK_STREAM | SOCK_NONBLOCK), 0);

	//Set the address for the socket
	sockaddr_in serv_addr{}; 				//Struct for defining socket address
	serv_addr.sin_family = AF_INET; 		//Tells the socket we're using IPv4.
	serv_addr.sin_port = htons(8080); 		//Sets the port number to 8080
	serv_addr.sin_addr.s_addr = INADDR_ANY;	//Listen to all interfaces

	//Bind the address to the socket
	bind(socket_fd, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr));
	//Socket is ready to listen to incoming connection requests
	listen(socket_fd, 256); 			//256 is the max number of connections

	//Add the socket to the epoll
	struct epoll_event e_event{};		//{} initializes  it with zeroes, not garbage?
	e_event.data.fd = socket_fd;		//The socket itself
	e_event.events = EPOLLIN;			//Notify me when incoming data is availible	
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &e_event);

	struct epoll_event events[1];

	std::map<int, int> connections;

	while (true) {
		int ready = epoll_wait(epoll_fd, events, 1, -1);
		if (ready == 0 || ready == -1)
			continue;

		//std::cout << "events: " << events[0].data.fd << std::endl;
		if (connections.find(events[0].data.fd) != connections.end()) {
			char buf[1000] = {0};
			recv(connections[events[0].data.fd], buf, 1000, 0);
			//#include "marquee.cpp"
			std::string body = "<!DOCTYPE html><html><head><title>First Web Page</title></head><body>Hello World!</body></html>";
			std::string header = "HTTP/1.1 200 OK\nDate: Thu, 26 May 2025 10:00:00 GMT\nServer: Apache/2.4.41 (Unix)\nContent-Type: text/html; charset=UTF-8\nContent-Length:" + std::to_string(body.length()) + "\nSet-Cookie: session=some-session-id; Path=/; HttpOnly\nX-Powered-By: PHP/7.4.3\nCache-Control: no-cache, private \n\n ";
			std::string msg = header + body;
			send(connections[events[0].data.fd], msg.c_str(), msg.length(), MSG_NOSIGNAL);

			std::cout << buf;
		}
		else {
			struct sockaddr_in client;
			socklen_t size = sizeof(client);
			int client_fd = accept(socket_fd, reinterpret_cast<sockaddr*>(&client), &size);
			if (client_fd > 0)
				std::cout << "Accepted a connection!" << std::endl;
			epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &e_event);
			connections[events[0].data.fd] = client_fd;
			//existingConnections.push_back(events[0].data.fd);
			//std::cout << "added: " << existingConnections[0] << std::endl;
		}
	}
	close(socket_fd);
	close(epoll_fd);
	return 0;
}

int	main(int argc, char **argv)
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
