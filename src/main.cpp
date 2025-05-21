#include <iostream>

int	main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cout << "\nLaunch with ./webserv configuration/desired_config_file.conf" << std::endl;
		return (1);
	}
	(void)argc;
	std::cout << "\nCan't do anything yet. :)" << std::endl;
	return (0);
}