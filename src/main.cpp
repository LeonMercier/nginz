#include <iostream>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>


int	mvp(void) {

	int epoll_fd = epoll_create1(0);
	int socket_thing = socket(AF_INET, (SOCK_STREAM | SOCK_NONBLOCK), 0);
	//???
	// setsockopt(socket_thing, int level, int optname, const void *optval, socklen_t optlen);
	//
	sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(8080);
	bind(socket_thing, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr));
	listen(socket_thing, 256);

	struct epoll_event e_event;
	while (true) {
		int ready = epoll_wait(epoll_fd, &e_event, 1, -1);

		struct sockaddr_in client;
		socklen_t size = sizeof(client);
		accept(epoll_fd, reinterpret_cast<sockaddr*>(&client), &size);
	}
	return 0;
}

int	main(int argc, char **argv)
{
	if (argc != 2 || std::string(argv[1]) != "configuration/webserv.conf")
	{
		std::cout << "\nRun with \"./webserv configuration/webserv.conf\"" << std::endl;
		return (1);
	}
	mvp();
	(void)argc;
	std::cout << "\nCan't do anything else yet. :)" << std::endl;
	return (0);
}
