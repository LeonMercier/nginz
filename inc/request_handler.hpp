#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>

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

Response getResponse(std::string request);
