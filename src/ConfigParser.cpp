
// IN PARSING:
// open the config_file -> trim comments and unnecessary spaces -> necessary config_file information is put into a string vector 
// FROM THAT STRING VECTOR:
	// search for server block(s)
	// inside the server block search matching "values" for keywords

#include "../inc/Structs.hpp"
#include "../inc/StandardLibraries.hpp"

void printServerConfigs(const std::vector<ServerConfig> &servers)
{
	for (size_t i = 0; i < servers.size(); ++i)
	{
		const ServerConfig &server = servers[i];
		std::cout << "======== SERVER " << i + 1 << " ========" << std::endl;

		// Server names
		std::cout << "Server Names: ";
		for (size_t j = 0; j < server.server_names.size(); ++j)
		{
			std::cout << server.server_names[j];
			if (j + 1 < server.server_names.size())
				std::cout << ", ";
		}
		std::cout << std::endl;

		// Listen IP/Port
		std::cout << "Listen IP: " << server.listen_ip << std::endl;
		std::cout << "Listen Port: " << server.listen_port << std::endl;

		// Error pages
		std::cout << "Error Pages:" << std::endl;
		for (std::map<int, std::string>::const_iterator it = server.error_pages.begin(); it != server.error_pages.end(); ++it)
		{
			std::cout << "  " << it->first << " => " << it->second << std::endl;
		}

		// Client max body size
		std::cout << "Client Max Body Size: " << server.client_max_body_size << std::endl;

		// Locations
		std::cout << "Locations: " << std::endl;
		for (size_t k = 0; k < server.locations.size(); ++k)
		{
			const LocationConfig &loc = server.locations[k];
			std::cout << "  --- Location " << k + 1 << " ---" << std::endl;
			std::cout << "  Path: " << loc.path << std::endl;
			std::cout << "  Root: " << loc.root << std::endl;
			std::cout << "  Index: " << loc.index << std::endl;
			std::cout << "  Methods: ";
			for (size_t m = 0; m < loc.methods.size(); ++m)
			{
				std::cout << loc.methods[m];
				if (m + 1 < loc.methods.size())
					std::cout << ", ";
			}
			std::cout << std::endl;
			std::cout << "  Autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;
			std::cout << "  Upload Store: " << loc.upload_store << std::endl;
			if (loc.return_code != 0 || !loc.return_url.empty())
			{
				std::cout << "  Return: " << loc.return_code << " " << loc.return_url << std::endl;
			}
		}
		std::cout << std::endl;
	}
}



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

LocationConfig	parseLocationConfig(std::vector<std::string> &config, std::vector<std::string>::iterator &it, std::string &path)
{
	LocationConfig	location;
	std::string	key;

	location.path = path;
	location.autoindex = false;
	location.return_code = 0;
	location.return_url = "";

	while (it != config.end())
	{
	//	std::cout << "it: " << *it << std::endl;
		std::istringstream iss;
		iss.str(*it);
		iss >> key;
		if (key == "root")
		{
			iss >> key;
			key.pop_back();
			location.root = key;
		}
		else if (key == "index")
		{
			iss >> key;
			key.pop_back();
			location.index = key;
		}
		else if (key == "methods")
		{
			while (iss >> key)
			{
				if (key.back() == ';')
					key.pop_back();
				location.methods.push_back(key);
			}
		}
		else if (key == "autoindex")
		{
			iss >> key;
			if (key == "on")
			{
				location.autoindex = true;
			}
		}
		else if (key == "upload_store")
		{
			iss >> key;
			if (key.back() == ';')
			{
				key.pop_back();
			}
			location.upload_store = key;
		}
		else if (key == "return")
		{
			int code;
			iss >> code;
			location.return_code = code;
			iss >> key;
			if (key.back() == ';')
			{
				key.pop_back();
			}
			location.return_url = key;
		}
		else if (key == "}")
		{
			break ;
		}
		it++;
	}
	return (location);

}

size_t	parseClientBodySize(std::string &client_max_size)
{
	int	coefficient = 1;

	if (client_max_size.find("K") != std::string::npos)
	{
		coefficient = 1000;
	}
	else if (client_max_size.find("M") != std::string::npos)
	{
		coefficient = 1000000;
	}
	else if (client_max_size.find("G") != std::string::npos)
	{
		coefficient = 1000000000;
	}
	return (std::stol(client_max_size) * coefficient); // add overflow handling later
}

// parse individual server here and return it as ServerConfig
// this function is called once we're already inside a server block (so "server { " is passed.)
// in a loop:
	// if "}" is encountered, break / return

ServerConfig parseIndividualServer(std::vector<std::string> &config, std::vector<std::string>::iterator it)
{
	ServerConfig	server;
	std::string		key;
	
	while (it != config.end())
	{
	//	std::cout << "it: " << *it << std::endl;
		if (*it == "{")
		{
			throw std::runtime_error("nested blocks in configuration file");
		}
		if (*it == "}")
			break ;
		std::istringstream iss;
		iss.str(*it);
		iss >> key;
		if (key == "listen")
		{
	//		std::cout << "LISTEN FOUND!" << std::endl;
			std::string ip_port;
			iss >> ip_port;
			size_t colon = ip_port.find(":");
			if (colon == std::string::npos)
			{
				throw std::runtime_error("IP address missing colon");
			}
			server.listen_ip = ip_port.substr(0, colon);
			server.listen_port = std::stoi(ip_port.substr(colon + 1));
	//		std::cout << "IP: " << server.listen_ip << "|\nPORT: " << server.listen_port << "|"<< std::endl;
		}
		else if (key == "server_name")
		{
		//	std::cout << "SERVER_NAME FOUND!" << std::endl;
			std::string name;
			while (iss >> name)
			{
				if (!name.empty() && name.back() == ';')
				{
					name.pop_back();
					server.server_names.push_back(name);
					break;
				}
				server.server_names.push_back(name);
			}
			// for (size_t i = 0; i < server.server_names.size(); ++i)
			// {
			// 	std::cout << "SERVER_NAME[" << i << "]: " << server.server_names[i] << std::endl;
			// }
		}
		else if (key == "client_max_body_size")
		{
		//	std::cout << "CLIENT_MAX_BODY_SIZE FOUND!" << std::endl;
			std::string client_max_size;
			iss >> client_max_size;
			server.client_max_body_size = parseClientBodySize(client_max_size);
		//	std::cout << "client_max_body_size: " << server.client_max_body_size << std::endl;
		}
		else if (key == "error_page")
		{
		//	std::cout << "ERROR_PAGE FOUND!" << std::endl;
			int	code;
			std::string error_page;
			iss >> code;
			iss >> error_page;
			error_page.pop_back();
			server.error_pages[code] = error_page;
		//	std::cout << "ERROR_PAGES MAP IS: \n";
    		// for (const auto& pair : server.error_pages)
			// {
       		// 	std::cout << "int: " << pair.first << "\nurl: " << pair.second << "\n";
   			// }
		}
		else if (key == "location")
		{
		//	std::cout << "LOCATION FOUND!" << std::endl;
			std::string	location;
			iss >> location;
			server.locations.push_back(parseLocationConfig(config, it, location));
		}
		it++;
	}
	return (server);
}

std::vector<ServerConfig> parseServers(std::vector<std::string> &config)
{
	// return a vector of ServerConfigs
	// search for server blocks (there might be many) and  and push_back the return value of ParseServer(parse individual server).
	// here we already skip comments and spaces from beginning and end

	std::vector<ServerConfig> servers;
	std::string	line;
	
	std::cout << line << std::endl;
	for (std::vector<std::string>::iterator it = config.begin() ; it != config.end(); ++it)
	{
		if (*it == "server" || *it == "server {") // handle this later so that it has already passed '{' before calling parseIndividualServer()
		{
	//		std::cout << "SERVER FOUND!" << std::endl;
			it++;
			servers.push_back(parseIndividualServer(config, it));
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

std::vector<ServerConfig> configParser(char *path_to_config)
{
	struct ConfigFile;
	std::vector<std::string> trimmed_config;

	std::ifstream config_file(path_to_config);
	if (!config_file)
	{
		throw std::runtime_error("Opening config file");
	}
	trimmed_config = trimConfigToVector(config_file);
	// std::cout << "Trimmed vector:\n";
	// for (std::vector<std::string>::iterator it = trimmed_config.begin() ; it != trimmed_config.end(); ++it)
	// 	std::cout << *it << "|\n";
	config_file.close();
	std::vector<ServerConfig> servers = parseServers(trimmed_config);
	printServerConfigs(servers);
	return servers;
}

