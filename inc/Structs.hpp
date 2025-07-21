#pragma once

#include "StandardLibraries.hpp"

struct LocationConfig
{
	std::string	path;
	std::string	root;
	std::string	index = "";
	std::vector<std::string> methods;
	bool		autoindex = false;
	std::string	upload_store; // not sure if a good name
	int			return_code = 0;
	std::string return_url = "";
	std::string	cgi_path_py = "";
};

struct ServerConfig
{
	std::vector<std::string> server_names;
	std::string listen_ip;
	int			listen_port;
	std::map<int, std::string> error_pages;
	size_t		client_max_body_size = 1000000;
	std::vector<LocationConfig> locations;
};

struct ConfigFile
{
	std::vector<ServerConfig> servers;
};

struct Response {
	std::string body = "";
	std::string header = "";
	std::string full_response = "";

	std::string redirect_path = "";
};