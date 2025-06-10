#include "Structs.hpp"
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>

struct Response {
	int			status_code = 200;
	std::string status_code_str = "OK";

	std::string method = "";
	std::string path = "";
	std::string version = "";

	std::string body = "";
	std::string header = "";
	std::string full_response = "";

	bool connection_is_close = false;
};

int getPostContentLength (std::string request);
Response getResponse(std::string request, ServerConfig config, int status_code);

const std::map<std::string, std::string> extensions {
	{".gif", "image/gif"},
	{".html", "text/html"},
	{".jpeg", "image/jpeg"},
	{".jpg", "image/jpeg"},
	{".png", "image/png"}
};
