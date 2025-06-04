#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>

struct Response {
	int			status_code = 0;
	std::string status_code_str = "";

	std::string method = "";
	std::string path = "";
	std::string version = "";

	std::string body = "";
	std::string header = "";
	std::string full_response = "";
};

std::string getResponse(std::string request);
