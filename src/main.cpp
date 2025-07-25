#include "../inc/StandardLibraries.hpp"
#include "../inc/Structs.hpp"
#include "../inc/Webserv.hpp"
#include "../inc/event_loop.hpp"

int	main(int argc, char **argv)
{
	if (argc > 2)
	{
		std::cout << "launch with './webserv path/to/configutarionfile.conf'" << std::endl;
		return (0);
	}
	try {
		std::vector<ServerConfig> server_configs;
		if (argc == 2) {
			 server_configs = configParser(argv[1]);
		} else {
			std::cout << "main(): using default config file" << std::endl;
			server_configs = configParser((char *)"configuration/webserv.conf");
		}
		eventLoop(server_configs);
	} catch (std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Type:    " << typeid(e).name() << "\n";
		return (1);
	} catch (...) {
		std::cerr << "Unknown error" << std::endl;
		return (1);
	}
    return (0);
}
