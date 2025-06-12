
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

	//	Server names
		std::cout << "Server Names: ";
		for (size_t j = 0; j < server.server_names.size(); ++j)
		{
			std::cout << server.server_names[j];
			if (j + 1 < server.server_names.size())
				std::cout << ", ";
		}
		std::cout << std::endl;

//		Listen IP/Port
		std::cout << "Listen IP: " << server.listen_ip << std::endl;
		std::cout << "Listen Port: " << server.listen_port << std::endl;

//		Error pages
		std::cout << "Error Pages:" << std::endl;
		for (std::map<int, std::string>::const_iterator it = server.error_pages.begin(); it != server.error_pages.end(); ++it)
		{
			std::cout << "  " << it->first << " => " << it->second << std::endl;
		}

//		Client max body size
		std::cout << "Client Max Body Size: " << server.client_max_body_size << std::endl;

	//	Locations
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

// ONCE ROOT IS KNOWN AND COMPLETE PATH AVAILABLE FOR TESTING, TRY TO OPEN THE LOCATION

std::string	validateAndParseLocationPath(std::istringstream &iss, std::string &line)
{
	std::string path;

	iss >> path;

	/*	validating the location statement line	 */
	if (line.back() != '{')
	{
		throw std::runtime_error("config file: location" + path + " block missing '{' after declaration");
	}
	if (line.find('{') != line.size() - 1)
	{
		throw std::runtime_error("config file: location" + path + " contains excessive '{'");
	}
	if (line.find(';') != std::string::npos)
	{
		throw std::runtime_error("config file: location statement for " + path + " contains forbidden character ';'");
	}
	/*	checking that '{' is separate from the location path	 */
	if (path.find('{') != std::string::npos)
	{
		throw std::runtime_error("config file: location statement for " + path + " attached to '{'");
	}
	if (path.find('/') != 0)
	{
		throw std::runtime_error("config file: location " + path + " should start with '/'");
	}

	if (path.find("//") != std::string::npos)
	{
		throw std::runtime_error("config file: location statement for " + path + " contains excessive slashes");
	}
	std::string brace;
	iss >> brace;

	if (brace != "{")
	{
		throw std::runtime_error("config file: too many location statements for " + path);
	}
	return (path);
}

void	validateAndParseRoot(std::istringstream &iss, LocationConfig &location)
{
	std::string root;

	iss >> root;
	if (root.find(';') != root.size() - 1)
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " root not ending with ';");
	}
	root.pop_back();
	if (!std::filesystem::is_directory(root))
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " root is not a directory");
	}
	if (location.path != "/" && (!std::filesystem::is_directory(root + location.path) && !std::filesystem::is_regular_file(root + location.path) ))
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " doesn't exist");
	}
	location.root = root;
	if (iss >> root)
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " root contains excessive info");
	}
}

void	validateAndParseIndex(std::istringstream &iss, LocationConfig &location)
{
	std::string index;

	iss >> index;
	if (index.find(';') != index.size() - 1)
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " index not ending with ';");
	}
	index.pop_back();
	if (iss >> index)
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " index contains excessive info");
	}
	location.index = index;
}

void	validateAndParseMethods(std::istringstream &iss, LocationConfig &location)
{
	std::string method;

	while (iss >> method)
	{
		if (method != "GET" && method != "GET;" && method != "POST" && method != "POST;" && method != "DELETE" && method != "DELETE;")
		{
			throw std::runtime_error("config file:\nlocation " + location.path + " method not supported: " + method);
		}
		if (method.back() == ';')
		{
			break ;
		}
		location.methods.push_back(method);
	}
	if (method.back() != ';')
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " method directive should end with ';'");
	}
	method.pop_back();
	location.methods.push_back(method);
	if (iss >> method)
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " methods contain excessive info after ending: " + method);
	}
}

void	validateAndParseAutoindex(std::istringstream &iss, LocationConfig &location)
{
	std::string autoindex;

	iss >> autoindex;
	if (autoindex.find(';') != autoindex.size() - 1)
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " autoindex not ending with ';'");
	}
	autoindex.pop_back();
	if (iss >> autoindex)
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " autoindex contains excessive info after ;");
	}
	if (autoindex == "on")
	{
		location.autoindex = true;
	}
	else if (autoindex == "off")
	{
		location.autoindex = false;
	}
	else
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " autoindex setting '" + autoindex + "' not supported");
	}
}

void	validateAndParseUpload(std::istringstream &iss, LocationConfig &location)
{
	std::string upload;

	iss >> upload;
	if (upload.find(';') != upload.size() - 1)
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " upload_store not ending with ';'");
	}
	upload.pop_back();
	if (iss >> upload)
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " upload_store contains excessive info after ;");
	}
	location.upload_store = upload;
// SHOULD THERE BE ADDITIONAL CHECKS? TRY TO OPEN THE PATH (upload_store)?
}

void	validateAndParseRedirect(std::istringstream &iss, LocationConfig &location)
{
	std::string code;

	iss >> code;
	if (!std::regex_match(code, std::regex(R"(^\d{3}$)")))
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " return code contains unsupported values");
	}
	location.return_code = std::stoi(code);

	std::string url;
	
	iss >> url;
	if (url.find(';') != url.size() - 1)
	{
		throw std::runtime_error("config file:\nlocation " + location.path + " return statement not ending with ';'");
	}
	url.pop_back();
	location.return_url = url;
}

LocationConfig	parseLocationConfig(std::vector<std::string> &config, std::vector<std::string>::iterator &it, std::istringstream &iss)
{
	LocationConfig	location;
	std::string	key;
	std::string path;

	location.path = validateAndParseLocationPath(iss, *it);
	it++;
	while (it != config.end())
	{
		std::istringstream iss;
		iss.str(*it);
		iss >> key;
		// std::cout << "it: " << *it << ", key: " << key << std::endl;
		if (key == "root")
		{
			validateAndParseRoot(iss, location);
		}
		else if (key == "index")
		{
			validateAndParseIndex(iss, location);
		}
		else if (key == "methods")
		{
			validateAndParseMethods(iss, location);
		}
		else if (key == "autoindex")
		{
			validateAndParseAutoindex(iss, location);
		}
		else if (key == "upload_store")
		{
			validateAndParseUpload(iss, location);
		}
		else if (key == "return")
		{
			validateAndParseRedirect(iss, location);
		}
		else if (key == "}")
		{
			break ;
		}
		else
		{
			throw std::runtime_error("config file:\n\tserver's location block:\n\t\tunrecognised: " + key); // coulud be more detailed message.
		}
		it++;
	}
	return (location);

}

void	validateAndParseClientBodySize(std::istringstream &iss, ServerConfig &server)
{
	std::string client_body;

	iss >> client_body;

	if (client_body.back() != ';')
	{
		throw std::runtime_error("config file: client_max_body_size directive missing semicolon");
	}
	if (client_body.find(';') != client_body.size() - 1)
	{
		throw std::runtime_error("config file: client_max_body_size directive contains excessive semicolon");
	}
	client_body.pop_back();

	std::regex valid_body_size(R"(^\d{1,10}$|^\d{1,7}[kK]$|^\d{1,4}[mM]$)");

	if (!std::regex_match(client_body, valid_body_size))
	{
		throw std::runtime_error("config file: client_max_body_size doesn't match requirements");
	}
	if (iss >> client_body)
	{
		throw std::runtime_error("config file: client_max_body_size contains excessive info");
	}
	int	coefficient = 1;

	if (client_body.find("K") != std::string::npos)
	{
		coefficient = 1000;
	}
	else if (client_body.find("M") != std::string::npos)
	{
		coefficient = 1000 * 1000;
	}
	else if (client_body.find("G") != std::string::npos)
	{
		coefficient = 1000 * 1000 * 1000;
	}
	server.client_max_body_size = std::stol(client_body) * coefficient;
}

void	validateAndParseErrorPage(std::istringstream &iss, ServerConfig &server)
{
	std::string error_code;
	std::string error_html;

	(void)server;
	iss >> error_code;
	if (!std::regex_match(error_code, std::regex(R"(^\d{3}$)")))
	{
		throw std::runtime_error ("config file: invalid error_page code: " + error_code);
	}
	int code = std::stoi(error_code);
	iss >> error_html;
	if (error_html.empty() || error_html.back() != ';')
	{
		throw std::runtime_error("config file: error_page " + error_code + " file path missing");
	}
	if (error_html.find(';') != error_html.size() - 1)
	{
		throw std::runtime_error("config file: error_page " + error_code + " not ending with (one) semicolon");
	}
	error_html.pop_back();
	std::ifstream test("./" + error_html);
	if (!test)
	{
		throw std::runtime_error("config file: error_page " + error_code + " not a valid path");
	}
	test.close();
	server.error_pages[code] = error_html;
}

void validateAndParseServerName(std::string &line, std::istringstream &iss, ServerConfig &server)
{
	if (server.server_names.size() != 0)
	{
		throw std::runtime_error("config file: conflicting/multiple server_name directives");
	}
	if (line.back() != ';')
	{
		throw std::runtime_error("config file: server_name directive missing semicolon");
	}
	if (line.find(';') != line.size() - 1)
	{
		throw std::runtime_error("config file: server_name directive contains excessive semicolon");
	}
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
}

void	validateAndParseListen(std::istringstream &iss, ServerConfig &server)
{
	std::string ip_port;
	iss >> ip_port;

	std::regex valid_ip(R"(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\:\d{1,5}\;)"); // 1-3digits.1-3digits.1-3.digits.1-3digits:1-5digits;
	if (!std::regex_match(ip_port, valid_ip))
	{
		throw std::runtime_error ("config file: listen IP and port in wrong format.");
	}
	ip_port.pop_back();
	size_t colon = ip_port.find(":");

	if (colon == std::string::npos) // shouldn't happend since we already validated
	{
		throw std::runtime_error("config file: IP address missing colon");
	}
	server.listen_ip = ip_port.substr(0, colon);
	server.listen_port = std::stoi(ip_port.substr(colon + 1));
}

ServerConfig parseIndividualServer(std::vector<std::string> &config, std::vector<std::string>::iterator &it)
{
	ServerConfig	server;
	std::string		key;
	
	while (it != config.end())
	{
	//	//std::cout << "it: " << *it << std::endl;
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
			validateAndParseListen(iss, server);
		}
		else if (key == "server_name")
		{
			validateAndParseServerName(*it, iss, server);
		}
		else if (key == "client_max_body_size")
		{
			validateAndParseClientBodySize(iss, server);
		}
		else if (key == "error_page")
		{
			validateAndParseErrorPage(iss, server);
		}
		else if (key == "location")
		{
			server.locations.push_back(parseLocationConfig(config, it, iss));
		}
		else
		{
			throw std::runtime_error("config file:\n\tserver block:\n\t\tunrecognised: " + *it);
		}
		it++;
	}
	return (server);
}

void	validateServerStatement( std::vector<std::string>::iterator &it)
{
	if (it->find_first_not_of("server") < 6)
	{
		throw std::runtime_error("config file:\nunexpected characters before server block statement");
	}
	std::string rest = it->substr(6,6);
	size_t	brace_pos = rest.find_first_not_of(" \t");
	if (rest[brace_pos] != '{')
	{	
		throw std::runtime_error("config file:\nserver block statement should be followed by '{'");
	}
	if (rest.find_first_not_of(" \t", brace_pos + 1) != std::string::npos)
	{
		throw std::runtime_error("config file:\nunexpected characters after '{' in server block");
	}
}

std::vector<ServerConfig> parseServers(std::vector<std::string> &config)
{
	std::vector<ServerConfig> servers;
	std::string	line;
	
	for (std::vector<std::string>::iterator it = config.begin() ; it != config.end(); ++it)
	{
	//	std::cout << "it: " << *it << std::endl;
		if (it->find("server") != std::string::npos)
		{
			validateServerStatement(it);
			it++;
			servers.push_back(parseIndividualServer(config, it));
		}
		else
		{
			throw std::runtime_error("config file: malformed");
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

void validateBraces(std::vector<std::string> &config)
{
	int opening_brace = 0;
	int closing_brace = 0;

	for (std::vector<std::string>::iterator it = config.begin() ; it != config.end(); ++it)
	{
		for (char ch: (*it))
		{
			if (ch == '{')
			{
				opening_brace++;
			}
			else if (ch == '}')
			{
				closing_brace++;
				if (opening_brace < closing_brace)
				{
					throw std::runtime_error("config file:\nunmatching closing brace '}'");
				}
			}
		}
	}
	if (opening_brace < closing_brace)
	{
		throw std::runtime_error("config file:\nunmatching closing brace '}'");
	}
	if (opening_brace > closing_brace)
	{
		throw std::runtime_error("config file:\nunmatching opening brace '{'");
	}
}

std::vector<ServerConfig> configParser(char *path_to_config)
{
	struct ConfigFile;
	std::vector<std::string> trimmed_config;

	if (std::filesystem::path(path_to_config).extension() != ".conf")
	{
		throw std::runtime_error("config file: extension not matching .conf");
	}
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
	validateBraces(trimmed_config);
	std::vector<ServerConfig> servers = parseServers(trimmed_config);
	if (servers.empty())
	{
		throw std::runtime_error("config file: no valid server information configured");
	}
	std::cout << "PRINTING SERVERS\n";
	printServerConfigs(servers);
	return servers;
}
