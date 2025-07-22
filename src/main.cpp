#include "../inc/StandardLibraries.hpp"
#include "../inc/Structs.hpp"
#include "../inc/Webserv.hpp"
#include "../inc/event_loop.hpp"

int	main(int argc, char **argv)
{
	try {
		std::vector<ServerConfig> server_configs;
		if (argc == 2) {
			 server_configs = configParser(argv[1]);
		} else {
			std::cout << "main(): using default config file" << std::endl;
			server_configs = configParser((char *)"configuration/webserv.conf");
		}
		// signal(SIGINT, handle_signal);
		// signal(SIGTERM, handle_signal);
		eventLoop(server_configs);
		// TODO add error for too many args?
	} catch (std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cout << "Type:    " << typeid(e).name() << "\n";
	} catch (...) {
		std::cerr << "Unknown error" << std::endl;
	}
    return (0);
}
