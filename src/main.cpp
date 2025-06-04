#include "../inc/StandardLibraries.hpp"
#include "../inc/Structs.hpp"
#include "../inc/Webserv.hpp"
#include "../inc/event_loop.hpp"

int	main(int argc, char **argv)
{
	try {
		if (argc != 2 || std::string(argv[1]) != "configuration/webserv.conf")
		{
			 std::cout << "\nRun with \"./webserv configuration/webserv.conf\"" << std::endl;
			//eventLoop();
			return (0);
		}
		std::vector<ServerConfig> server_configs = configParser(argv[1]);
		eventLoop(server_configs);
	} catch (std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "Unknown error" << std::endl;
	}
    return (0);
}
