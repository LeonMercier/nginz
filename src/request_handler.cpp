#include "../inc/request_handler.hpp"
#include "../inc/Webserv.hpp"
#include "../inc/parse_header.hpp"

static bool endsWith(const std::string& str, const std::string& suffix);
static std::string getHttpDate();
static void createBody(Response *response, ServerConfig config, std::string filename);
static void createBodyForError(Response *response, int status_code, std::string filename);
static void createHeader(Response *response, std::string content_type);
static void getAutoIndex(Response *response, ServerConfig config);
static void handleError(Response *response, ServerConfig config, int status_code);
static void handleGet(Response *response, ServerConfig config);
static void setStatusCode(Response *response, int status_code);

int getPostContentLength (std::string request) {
	std::istringstream iss(request);
	std::string temp;
	int result = 0;
	while (getline(iss, temp)) {
		if (temp.find("Content-Length:") != std::string::npos) {
			result = stoi(temp.substr(16));
			break;
		}
	}
	return (result);
}

static void setStatusCode(Response *response, int status_code) {
	response->status_code = status_code;
	for (auto& p : errorCodes) {
		if (status_code == p.first) {
			response->status_code_str = p.second;
		}
	}
}

static std::string getHttpDate() {
    std::time_t now = std::time(nullptr);
    std::tm *gmt = std::gmtime(&now);

    std::ostringstream date_stream;
    date_stream << std::put_time(gmt, "%a, %d %b %Y %H:%M:%S GMT");
    return date_stream.str();
}

static bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static void getAutoIndex(Response *response, ServerConfig config) {
	try {
		response->body = generateAutoIndex(response->path, config);
	} catch (...) {
		handleError(response, config, 404);
	}
	createHeader(response, "text/html; charset=UTF-8");
}

static bool isLocationInConfig(ServerConfig config, std::string path) {
	for (auto p : config.locations) {
		if (p.path == path)
			return true;
	}
	return false;
}

static void createBodyForError(Response *response, int status_code, std::string filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		for (auto& p : errorHttps) {
			if (status_code == p.first) {
				response->body = p.second;
				return;
			}
		}
	}
	std::ostringstream sbody;
	sbody << file.rdbuf();
	response->body = sbody.str();
}

static void handleError(Response *response, ServerConfig config, int status_code) {
	setStatusCode(response, status_code);
	for (auto& p : config.error_pages) {
		if (status_code == p.first) {
			createBodyForError(response, status_code, p.second);
			createHeader(response, "text/html; charset=UTF-8");
		}
	}
	for (auto& p : errorHttps) {
		if (status_code == p.first) {
			response->body = p.second;
			createHeader(response, "text/html; charset=UTF-8");
		}
	}
}

static void createHeader(Response *response, std::string content_type) {
	response->header = "HTTP/1.1 " + std::to_string(response->status_code) + " " + response->status_code_str +"\r\n"
	"Date: " + getHttpDate() + "\r\n"
	"Server: OverThirty_Webserv\r\n"
	"Content-Type: " + content_type + "\r\n"
	"Content-Length: " + std::to_string(response->body.length()) + "\r\n"
	"Cache-Control: no-cache, private\r\n"
	+ "\r\n";
}

static void createBody(Response *response, ServerConfig config, std::string filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		handleError(response, config, 404);
	}
	std::ostringstream sbody;
	sbody << file.rdbuf();
	response->body = sbody.str();
}

static void handleGet(Response *response, ServerConfig config) {
	if (response->path == "/") {
		createBody(response, config, config.locations[0].root + "/index.html");
		createHeader(response, "text/html; charset=UTF-8");
	}
	else if (isLocationInConfig(config, response->path)) {
		getAutoIndex(response, config);
	}
	else {
		createBody(response, config, config.locations[0].root + response->path);
		if (response->status_code == 200) {
			for (auto& p : extensions) {
				if (endsWith(response->path, p.first)) {
					createHeader(response, p.second);
					break;
				}
			}
		}
	}
}

// static void getRoot(Response *response, ServerConfig config) {
// 	std::string root = "";
// 	long smallest_dif = SIZE_MAX;
// 	long len = response->path.length();

// 	std::cout << ">>>" << "          Path: " << response->path << std::endl;

// 	for (auto location : config.locations) {
// 		std::cout << ">>>" << "response->path: " << location.path << std::endl;
		
// 		long dif = location.path.length() - len;
// 		std::cout << ">>>" << "           Dif: " << dif << std::endl;
// 		if (dif < 0)
// 			continue;
// 		else if (response->path.compare(0, len, location.path) == 0) {
// 			if (dif == 0) {
// 				root = location.root;
// 				break;
// 			}
// 			else if (dif < smallest_dif) {
// 				root = location.root;
// 				smallest_dif = dif;
// 			}
// 		}
// 	}
// 	response->root = root;
// }

// static void getRoot(Response *response, ServerConfig config) {
// 	std::string root = "";
// 	std::string path = response->path;
// 	size_t highest_match = 0;

// 	std::cout << ">>>" << "          Path: " << path << std::endl;

// 	for (auto location : config.locations) {
// 		std::string directory = location.path;
// 		std::cout << ">>>" << "     Directory: " << directory << std::endl;

// 		size_t i = 0;
// 		while (path[i] && directory[i]) {
// 			if (path[i] != directory[i])
// 				break;
// 			i++;
// 		}
// 		if (i > highest_match) {
// 			root = location.root;
// 			highest_match = i;
// 		}
// 	}
// 	response->root = root;
// }

Response getResponse(std::string request, ServerConfig config, int status_code) {
	//status_code = 200; //COMMENT OUT
	Response response;
	std::istringstream iss(request);
	iss >> response.method >> response.path >> response.version;

	// std::cout << ">>>" << "-------------------------------------------" << std::endl;
	// getRoot(&response, config);
	// std::cout << ">>>" << "          ROOT: " << response.root << std::endl;

	parseHeader(request);
	if (status_code != 200)
		handleError(&response, config, status_code);
	else if (response.method == "GET")
		handleGet(&response, config);
	response.full_response = response.header + response.body;
	return response;
}
