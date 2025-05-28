#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <cstddef> // for size_t?

///// First defining everything as structs, later thinking which should be class etc.

struct LocationConfig
{
	std::string	path;
	std::string	root;
	std::string	index;
	std::vector<std::string> methods;
	bool		autoindex;
	std::string	upload_store; // not sure if a good name
	int			return_code;
	std::string return_url;
	// stuff for the cgi later cause no idea yet
};

struct ServerConfig
{
	std::string server_name;
	std::string listen_ip;
	int			listen_port;
	std::map<int, std::string> error_pages;
	size_t		client_max_body_size;
	//std::vector<LocationConfig>;
};

struct ConfigFile
{
	std::vector<ServerConfig> servers;
};



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

	start = line.find_first_not_of("\t\n\r"); // skip these whitespaces before the line
	if (start == std::string::npos)
		return ("");
	end = line.find_last_not_of("\t\n\r");
	return (line.substr(start, end - start + 1));
}


// IN PARSING:
// open the config_file
// skip comments (#) and first search for server block
// inside the server block search matching "values" for keywords

ServerConfig parseIndividualServer(std::ifstream &config_file) // when config_file is passed as a reference, getline is not starting over from the top
{
	// parse individual server here and return it as ServerConfig
	// this function is called once we're already inside a server block, so "server { " is passed.
	// in a loop:
		// skip all comments and spaces
		// skip newlines, tabs and carriage return (not sure if necessary)
		// if "}" is encountered, break / return
}

std::vector<ServerConfig> parseServers(std::ifstream &config_file)
{
	// return a vector of ServerConfigs
	// search for server blocks (there might be many) and  and push_back the return value of ParseServer(parse individual server).
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
	}

}

void	configParser(char *path_to_config)
{
	struct ConfigFile;

	std::ifstream config_file(path_to_config);
	if (!config_file)
	{
		throw std::runtime_error("Opening config file");
	}
	std::vector<ServerConfig> servers = parseServers(config_file);
}

