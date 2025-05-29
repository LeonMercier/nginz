#include "../inc/event_loop.hpp"

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	// if (argc != 2 || std::string(argv[1]) != "configuration/webserv.conf")
	// {
	// 	std::cout << "\nRun with \"./webserv configuration/webserv.conf\"" << std::endl;
	// 	return (1);
	// }
    eventLoop();
    return (0);
}
