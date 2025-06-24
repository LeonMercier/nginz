#include "../inc/StandardLibraries.hpp"
#include "../inc/Structs.hpp"
#include "../inc/Webserv.hpp"
#include "../inc/event_loop.hpp"

int	main(int argc, char **argv)
{
	try {
		std::vector<ServerConfig> server_configs = configParser((char *)"configuration/webserv.conf");
		if (argc == 2)
			 std::vector<ServerConfig> server_configs = configParser(argv[1]);
		// TODO add error for too many args?
		eventLoop(server_configs);
	} catch (std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "Unknown error" << std::endl;
	}
    return (0);
}
