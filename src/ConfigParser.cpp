
// IN PARSING:
// open the config_file
// skip comments (#) and first search for server block
// inside the server block search matching "values" for keywords

#include "../inc/Structs.hpp"

std::string	trimComments(const std::string &line)
{
	size_t hash_pos;

	hash_pos = line.find("#");
	if (hash_pos != std::string::npos)
		return (line.substr(0, hash_pos));
	return (line);
}

std::string	trimBeginningAndEnd(const std::string &line)
{
	size_t	start;
	size_t	end;

	start = line.find_first_not_of("\t\n\r "); // skip these whitespaces before the line
	if (start == std::string::npos)
		return ("");
	end = line.find_last_not_of("\t\n\r ");
	return (line.substr(start, end - start + 1));
}

LocationConfig	parseLocationConfig(std::ifstream &config_file, std::string &location)
{

}

ServerConfig parseIndividualServer(std::ifstream &config_file) // when config_file is passed as a reference, getline is not starting over from the top
{
	// parse individual server here and return it as ServerConfig
	// this function is called once we're already inside a server block, so "server { " is passed.
	// in a loop:
		// if "}" is encountered, break / return
	ServerConfig	server;
	std::string		line;
	std::string		key;
	
	while (std::getline(config_file, line))
	{
		line = trimComments(line);
		if (line.empty())
		{
			continue;
		}
		line = trimBeginningAndEnd(line);
		if (line.empty())
		{
			continue;
		}
		if (line.empty() || line == "{")
		{
			continue;
		}
		if (line == "}")
			break ;
		// std::cout << line << std::endl;
		std::istringstream iss;
		iss.str(line);
		iss >> key;
		if (key == "listen")
		{
			std::cout << "LISTEN FOUND!" << std::endl;
			std::string ip_port;
			iss >> ip_port;
			std::cout << "IP_PORT: " << ip_port << std::endl;
		}
		if (key == "server_name")
		{
			std::cout << "SERVER_NAME FOUND!" << std::endl;
			std::string server_name;
			iss >> server_name;
			std::cout << "SERVER_NAME: " << server_name << std::endl;
		}
		if (key == "client_max_body_size")
		{
			std::cout << "CLIENT_MAX_BODY_SIZE FOUND!" << std::endl;
			std::string client_max_size;
			iss >> client_max_size;
			std::cout << "CLIENT_MAX_SIZE: " << client_max_size << std::endl;
		}
		if (key == "error_page")
		{
			std::cout << "ERROR_PAGE FOUND!" << std::endl;
			std::string error_page;
			iss >> error_page;
			std::cout << "ERROR_PAGE: " << error_page << std::endl;
		}
		if (key == "location")
		{
			std::cout << "LOCATION FOUND!" << std::endl;
			std::string	location;
			iss >> location;
			parseLocationConfig(config_file, location);
		}
	}
	return (server);
}

std::vector<ServerConfig> parseServers(std::ifstream &config_file)
{
	// return a vector of ServerConfigs
	// search for server blocks (there might be many) and  and push_back the return value of ParseServer(parse individual server).
	// here we already skip comments and spaces from beginning and end
	std::vector<ServerConfig> servers;
	std::string	line;
	
	while (std::getline(config_file, line))
	{
		line = trimComments(line);
		if (line.empty())
		{
			continue;
		}
		line = trimBeginningAndEnd(line);
		if (line.empty())
		{
			continue;
		}
		std::cout << line << std::endl;
		if (line == "server" || line == "server {")
		{
			std::cout << "SERVER FOUND!" << std::endl;
			servers.push_back(parseIndividualServer(config_file));
		}
	}
	return (servers);

}

std::vector<std::string> trimConfigToVector(std::ifstream &config_file)
{
	std::vector<std::string> config_vector;
	std::string	line;

	while (std::getline(config_file, line))
	{
		line = trimComments(line);
		if (line.empty())
		{
			continue;
		}
		line = trimBeginningAndEnd(line);
		if (line.empty())
		{
			continue;
		}
		config_vector.push_back(line);
	}
	return (config_vector);
}

void	configParser(char *path_to_config)
{
	struct ConfigFile;
	std::vector<std::string> trimmed_config;

	std::ifstream config_file(path_to_config);
	if (!config_file)
	{
		throw std::runtime_error("Opening config file");
	}
	trimmed_config = trimConfigToVector(config_file);
	std::cout << "Trimmed vector:\n";
	for (std::vector<std::string>::iterator it = trimmed_config.begin() ; it != trimmed_config.end(); ++it)
		std::cout << *it << "|\n";
	// change config_file parameter to trimmed_config
	return ;
	std::vector<ServerConfig> servers = parseServers(config_file);
}

