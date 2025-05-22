#include <iostream>

int	main(int argc, char **argv)
{
	if (argc != 2 || std::string(argv[1]) != "configuration/webserv.conf")
	{
		std::cout << "\nRun with \"./webserv configuration/webserv.conf\"" << std::endl;
		return (1);
	}
	(void)argc;
	std::cout << "\nCan't do anything else yet. :)" << std::endl;
	return (0);
}