
#include "../inc/StandardLibraries.hpp"
#include "../inc/Structs.hpp"
#include "../inc/Webserv.hpp"

int	main(int argc, char **argv)
{
	// (void)argc;
	// (void)argv;
	if (argc != 2 || std::string(argv[1]) != "configuration/webserv.conf")
	{
		// std::cout << "\nRun with \"./webserv configuration/webserv.conf\"" << std::endl;
		eventLoop();
		return (0);
	}
	configParser(argv[1]);
    return (0);
}
