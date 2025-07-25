#include "../inc/event_loop.hpp"
#include "../inc/Structs.hpp"
#include "../inc/Client.hpp"
#include "../inc/Signal.hpp"

static void createServerSocket(
	int epoll_fd,
	std::map<int, std::vector<ServerConfig>> &servers,
	std::vector<ServerConfig> configs)
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

	// Set the address for the socket
	struct addrinfo hints {};
	memset(&hints, 0, sizeof(hints)); // is this needed?
	hints.ai_family = AF_INET; // IPV4
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo *gai_result;
	if (getaddrinfo(
		configs.front().listen_ip.c_str(),
		std::to_string(configs.front().listen_port).c_str(),
		&hints,
		&gai_result) < 0)
	{
		close(socket_fd);
		throw std::runtime_error("getaddrinfo() failed"); 
	}
	
	// Bind the address to the socket
	// gai_result is a linked list of possibly multiple structs, but for now
	// we will only check the first struct
	if (bind(socket_fd, gai_result->ai_addr, gai_result->ai_addrlen) < 0)
	{
		close(socket_fd);
		freeaddrinfo(gai_result);
		throw std::runtime_error("bind() failed");
	}
	freeaddrinfo(gai_result);

	// Socket is ready to listen to incoming connection requests
	// 256 is the max number of connections
	if (listen(socket_fd, 256) < 0) {
		close(socket_fd);
		throw std::runtime_error("listen() failed");
	}

	// Add the socket to the epoll
	struct epoll_event e_event{}; //{} initializes  it with zeroes, not garbage?
	e_event.data.fd = socket_fd;  // The socket itself
	e_event.events = EPOLLIN;	  // Notify me when incoming data is available
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &e_event) < 0) {
		close(socket_fd);
		throw std::runtime_error("createServerSocket(): epoll_ctl() failed"); 
	}
	for (auto it = configs.begin(); it != configs.end(); it++) {
		servers[socket_fd].push_back(*it);
	}
}

// creates a map where the combination of ip and port is the key and all 
// configs with the same ip and port are added to that
static std::map<std::string, std::vector<ServerConfig>> sortConfigs(
	std::vector<ServerConfig> configs)
{
	std::map<std::string, std::vector<ServerConfig>> sorted;

	for (auto it = configs.begin(); it != configs.end(); it++) {
		std::string id = it->listen_ip + std::to_string(it->listen_port);
		sorted[id].push_back(*it);
	}
	return sorted;
}

static void	checkClientTimeout(std::map<int, Client> &clients)
{
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		time_t	latest_event = it->second.getLastEvent();
		time_t	current_time = std::time(nullptr);
		
		if (current_time - latest_event >= _CLIENT_TIMEOUT_S)
		{
			// 408: RFC: "The client did not produce a request within the time 
			// that the server was prepared to wait."
			t_client_state client_state = it->second.getState();
			if (client_state == DISCONNECT)
			{
				// client will get disconnected on this iteration of the 
				// event loop anyway
				continue ;
			}
			if (client_state == IDLE) 
			{
				it->second.setState(DISCONNECT);
			}
			else if (client_state == RECV || client_state == RECV_CHUNKED)
			{
				it->second.setState(TIMEOUT);
				it->second.changeEpollMode(EPOLLOUT);
			}
		}
	}
}

void	handle_signal(int sig)
{
	if (sig == SIGINT || sig == SIGTERM)
	{
		std::cout << "\nWebserver terminated by user" << std::endl;
		signal_stop = true;
	}
}

void	close_fds(std::map<int, Client> &clients, std::map<int, std::vector<ServerConfig>> &servers)
{
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); it++)
	{
		std::cout << "\nClosing client fd: " << it->second.getClientFd() << std::endl;
		close(it->second.getClientFd());
	}

	for (auto it = servers.begin(); it != servers.end(); it++)
	{
		std::cout << "Closing server socket: " << it->first << std::endl;
		close(it->first);
	}
}

int eventLoop(std::vector<ServerConfig> server_configs)
{
	std::map<std::string, std::vector<ServerConfig>> per_socket_configs =
		sortConfigs(server_configs);

	int epoll_fd = epoll_create(1);
	// first fd we create, so we can just throw
	if (epoll_fd < 0) { throw std::runtime_error("epoll_create() failed"); };

	std::map<int, std::vector<ServerConfig>> servers;
	for (auto it = per_socket_configs.begin(); it != per_socket_configs.end(); it++) {
		try
		{
			createServerSocket(epoll_fd, servers, it->second);
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
			std::map<int, Client> dummy_clients;
			close_fds(dummy_clients, servers);
			close(epoll_fd);
			throw std::runtime_error("Terminating webserv before launch\n");
		}
	}

	// apparenty this does NOT need to be zeroed (for ex. manpage example)
	struct epoll_event events[64];

	// using client_fd as index (but also storing it inside the class for now)
	std::map<int, Client> clients;

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);
	while (true)
	{
		checkClientTimeout(clients);
		int ready = epoll_wait(epoll_fd, events, 64, 1000);
		if (signal_stop)
		{
			close_fds(clients, servers);
			break ;
		}
		if (ready < 0) { throw std::runtime_error("epoll_wait() failed"); };
		if (ready == 0)
			continue;
		for (int i = 0; i < ready; i++) // loop over all new events
		{
			int curr_event_fd = events[i].data.fd;

			// fd of current event is an index of servers => new connection
			if (servers.find(curr_event_fd) != servers.end() && !signal_stop)
			{
				struct sockaddr_in client;
				socklen_t size = sizeof(client);
				int client_fd = accept(curr_event_fd,
						   reinterpret_cast<sockaddr *>(&client), &size);
				if (client_fd < 0) {
					std::cerr << "accept() failed" << std::endl;
					continue ;
				};

				auto retval = clients.insert({client_fd,
					Client(servers[curr_event_fd], epoll_fd, client_fd)});
				if (retval.second == false) {
					std::cerr <<"failed to add new client; client already exists?" << std::endl;
				}

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
					std::cerr << "eventLoop(): epoll_ctl() failed" << std::endl;
				}
				clients.at(client_fd).updateLastEvent();
			}
			else // existing connection
			{
				Client &curr_client = clients.at(curr_event_fd);
				curr_client.updateLastEvent();

				if (curr_client.getState() == DISCONNECT) {
					std::cout << "DISCONNECTING CLIENT\n";
					curr_client.closeConnection(epoll_fd, curr_client.getClientFd());
					clients.erase(curr_event_fd);
					continue;
				}
				if (events[i].events & EPOLLIN) {
					curr_client.recvFrom();
				}
				if (events[i].events & EPOLLOUT) {
					if (curr_client.getState() != TIMEOUT){
						curr_client.sendTo();
					}
					else{
						std::cout << "\nSENDING TIMEOUT HEADER TO CLIENT:" 
							<< curr_client.getClientFd() << std::endl;
						curr_client.request.getResponse(408);
						curr_client.send_queue.push_back(curr_client.request.getRes());
						for (auto it = curr_client.send_queue.begin(); it != curr_client.send_queue.end(); it++)
						{
							std::cout << it->header << std::endl;
						}
						curr_client.sendTo();
						curr_client.setState(DISCONNECT);
					}
				}
			}
		}
	}
	close(epoll_fd);
	return 0;
}


