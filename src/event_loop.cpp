#include "../inc/event_loop.hpp"
#include "../inc/Structs.hpp"
#include "../inc/Client.hpp"

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

	// close the socket when an execve() happens
	// TODO: fcntl() forbidden for Linux by the subject?
	// if (fcntl(socket_fd, F_SETFL, FD_CLOEXEC) < 0) {
	// 	throw std::runtime_error("fcntl() failed"); 
	// }

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
		throw std::runtime_error("getaddrinfo() failed");
	}
	
	// Bind the address to the socket
	// gai_result is a linked list of possibly multiple structs, but for now
	// we will only check the first struct
	if (bind(socket_fd, gai_result->ai_addr, gai_result->ai_addrlen) < 0)
	{
		throw std::runtime_error("bind() failed");
	}
	freeaddrinfo(gai_result);

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

void	checkClientTimeout(std::map<int, Client> &clients)
{
	for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
	{
		// std::cout << "\nClient[" << it->second.getClientFd() << "]\n";
		time_t	latest_event = it->second.getLastEvent();
		time_t	current_time = std::time(nullptr);
		
		if (current_time - latest_event >= 5)
		{
			std::cout << "\n\n\n\ntrying to setState of the Client[" << it->second.getClientFd() << "], whose latest events is at: \n"
			<< std::asctime(std::localtime(&latest_event)) << "now its: " << std::asctime(std::localtime(&current_time)) << std::endl;
			it->second.setState(TIMEOUT);
			it->second.changeEpollMode(EPOLLOUT);
		}
	}
}

int eventLoop(std::vector<ServerConfig> server_configs)
{
	std::map<std::string, std::vector<ServerConfig>> per_socket_configs =
		sortConfigs(server_configs);

	int epoll_fd = epoll_create(1);
	if (epoll_fd < 0) { throw std::runtime_error("epoll_create() failed"); };

	std::map<int, std::vector<ServerConfig>> servers;
	for (auto it = per_socket_configs.begin(); it != per_socket_configs.end(); it++) {
		createServerSocket(epoll_fd, servers, it->second);
	}

	// apparenty this does NOT need to be zeroed (for ex. manpage example)
	struct epoll_event events[64];

	// using client_fd as index (but also storing it inside the class for now)
	std::map<int, Client> clients;

	while (true)
	{
		checkClientTimeout(clients);
		// time_t time = std::time(nullptr);
		// std::cout << "Time before epoll_wait: " << std::asctime(std::localtime(&time)) << std::endl;
		int ready = epoll_wait(epoll_fd, events, 64, 1000);
		// time = std::time(nullptr);
		// std::cout << "Time after epoll_wait: " << std::asctime(std::localtime(&time)) << std::endl;
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

				auto retval = clients.insert({client_fd,
					Client(servers[curr_event_fd], epoll_fd, client_fd)});
				if (retval.second == false) {
					throw std::runtime_error("failed to add new client; \
							  client already exists?");
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
					throw std::runtime_error("eventLoop(): epoll_ctl() failed");
				}
				clients.at(client_fd).updateLastEvent();
			}
			else // existing connection
			{
				clients.at(curr_event_fd).updateLastEvent();

				// std::cout << "receiving: socket: " << socket_fd;
				//std::cout << " client fd: " << curr_event_fd << std::endl;
				if (clients.at(curr_event_fd).getState() == DISCONNECT) {
					std::cout << "CLIENT WANTS TO DISCONNECT\n";
					clients.at(curr_event_fd).closeConnection(epoll_fd, clients.at(curr_event_fd).getClientFd());
					clients.erase(curr_event_fd);

					continue;
				}

				// map::at() will throw std::out_of_range if not found
				if (events[i].events & EPOLLIN) {
					clients.at(curr_event_fd).recvFrom();
				}
				if (events[i].events & EPOLLOUT) {
					if (clients.at(curr_event_fd).getState() != TIMEOUT){
						clients.at(curr_event_fd).sendTo();
					}
					else{
						Client &cur_client = clients.at(curr_event_fd);
						std::cout << "\nSENDING TIMEOUT HEADER TO CLIENT:" << cur_client.getClientFd() << std::endl;
						// maybe front instead of back
						cur_client.request.getResponse(408);
						cur_client.send_queue.push_back(cur_client.request.getRes());
						std::cout << "After creating send_que vector\n";
						for (auto it = cur_client.send_queue.begin(); it != cur_client.send_queue.end(); it++)
						{
							std::cout << it->header << std::endl;
						}
						cur_client.sendTo();
						cur_client.setState(DISCONNECT);
					}
				}

			}
		}
	}
	// close(e_event.data.fd);
	// close(socket_fd);
	close(epoll_fd);
	return 0;
}


