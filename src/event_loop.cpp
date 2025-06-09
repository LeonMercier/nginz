#include "../inc/event_loop.hpp"
#include "../inc/Structs.hpp"
#include "../inc/Client.hpp"

static void createServerSocket(
	int epoll_fd,
	std::map<int, ServerConfig> &servers,
	ServerConfig config)
{
	int socket_fd = socket(AF_INET, (SOCK_STREAM | SOCK_NONBLOCK), 0);
	if (socket_fd < 0) { throw std::runtime_error("socket() failed"); };

	// SO_REUSEADDR prevents bind() from failing when restarting the server 
	// quickly. SOL_SOCKET is not really an option.
	int optval = 1;
	if (setsockopt(
		socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
		throw std::runtime_error("setsockopt() failed"); 
	}

	// close the socket when an execve() happens
	// TODO: fcntl() forbidden for Linux by the subject?
	// if (fcntl(socket_fd, F_SETFL, FD_CLOEXEC) < 0) {
	// 	throw std::runtime_error("fcntl() failed"); 
	// }
	
	// Set the address for the socket
	sockaddr_in serv_addr{};				// Struct for defining socket address
	serv_addr.sin_family = AF_INET;			// Tells the socket we're using IPv4.
	serv_addr.sin_port = htons(config.listen_port);		// Sets the port number to 8080
	serv_addr.sin_addr.s_addr = INADDR_ANY; // Listen to all interfaces

	// Bind the address to the socket
	if (bind(socket_fd, reinterpret_cast<sockaddr *>(&serv_addr),
		sizeof(serv_addr)) < 0)
	{
		throw std::runtime_error("bind() failed");
	}

	// Socket is ready to listen to incoming connection requests
	// 256 is the max number of connections
	if (listen(socket_fd, 256) < 0) {
		throw std::runtime_error("listen() failed");
	}

	// Add the socket to the epoll
	struct epoll_event e_event{}; //{} initializes  it with zeroes, not garbage?
	e_event.data.fd = socket_fd;  // The socket itself
	e_event.events = EPOLLIN;	  // Notify me when incoming data is available
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &e_event) < 0) {
		throw std::runtime_error("epoll_ctl() failed"); 
	}
	servers[socket_fd] = config;
}

int eventLoop(std::vector<ServerConfig> server_configs)
{
	int epoll_fd = epoll_create1(0);
	if (epoll_fd < 0) { throw std::runtime_error("epoll_create1() failed"); };

	std::map<int, ServerConfig> servers;
	for (auto it = server_configs.begin(); it != server_configs.end(); it++) {
		createServerSocket(epoll_fd, servers, *it);
	}

	// apparenty this does NOT need to be zeroed (for ex. manpage example)
	struct epoll_event events[64];

	// using client_fd as index (but also storing it inside the class for now)
	std::map<int, Client> clients;

	while (true)
	{
		int ready = epoll_wait(epoll_fd, events, 64, -1);
		if (ready < 0) { throw std::runtime_error("epoll_wait() failed"); };
		if (ready == 0)
			continue;
		for (int i = 0; i < ready; i++) // loop over all new events
		{
			int curr_event_fd = events[i].data.fd;

			// fd of current event is an index of servers => new connection
			if (servers.find(curr_event_fd) != servers.end())
			// if (curr_event_fd == socket_fd) // new connection
			{
				struct sockaddr_in client;
				socklen_t size = sizeof(client);
				int client_fd = accept(curr_event_fd,
						   reinterpret_cast<sockaddr *>(&client), &size);
				if (client_fd < 0) {
					throw std::runtime_error("accept() failed");
				};

				 clients.insert({client_fd,
					Client(servers[curr_event_fd], epoll_fd, client_fd)});

				// epoll_ctl() takes information from e_event and puts it into
				// the data structures held in the kernel, that is why we can
				// reuse e_event. In other words; the contents of e_event are
				// only relevant for epoll_ctl()
				//
				// Here we add new connection to the interest list
				//
				// Without EPOLLET we are in "level triggered mode"
				// e_event.events = EPOLLIN | EPOLLET;
				struct epoll_event e_event{}; //{} initializes  it with zeroes, not garbage?
				e_event.events = EPOLLIN;
				e_event.data.fd = client_fd;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &e_event) < 0) {
					throw std::runtime_error("epoll_ctl() failed");
				}
			}
			else // existing connection
			{
				if (clients.find(curr_event_fd) == clients.end()) {
					std::cout << "Error: client not found" << std::endl;
					break;
				}
				// std::cout << "receiving: socket: " << socket_fd;
				//std::cout << " client fd: " << curr_event_fd << std::endl;

				if (events[i].events & EPOLLIN) {
					clients.at(curr_event_fd).recvFrom();
				}
				if (events[i].events & EPOLLOUT) {
					clients.at(curr_event_fd).sendTo();
				}

			}
		}
	}
	// close(e_event.data.fd);
	// close(socket_fd);
	close(epoll_fd);
	return 0;
}


