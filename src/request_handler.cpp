#include "../inc/request_handler.hpp"
#include "../inc/Webserv.hpp"
#include "../inc/parse_header.hpp"

static bool endsWith(const std::string& str, const std::string& suffix);
static std::string getHttpDate();
static void createBody(Response &response, ServerConfig config, std::string filename);
static void createBodyForError(Response &response, int status_code, std::string filename);
static void createHeader(Response &response, std::string content_type);
static void getAutoIndex(Response &response, ServerConfig config);
static void handleError(Response &response, ServerConfig config, int status_code);
static void handleGet(Response &response, ServerConfig config);
static void handleDelete(Response &response, ServerConfig config);
static void setStatusCode(Response &response, int status_code);
static LocationConfig getLocation(std::string path, ServerConfig config);

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

bool isPostAllowed(std::string path, ServerConfig config) {
	LocationConfig location = getLocation(path, config);
	if (std::find(location.methods.begin(), location.methods.end(), "POST")
		!= location.methods.end()) {
		return true;
	}
	return false;
}

static void setStatusCode(Response &response, int status_code) {
	response.status_code = status_code;
	for (auto& p : errorCodes) {
		if (status_code == p.first) {
			response.status_code_str = p.second;
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

static void getAutoIndex(Response &response, ServerConfig config) {
	try {
		response.body = generateAutoIndex(response.path, config);
	} catch (...) {
		handleError(response, config, 404);
	}
	createHeader(response, "text/html; charset=UTF-8");
}

static void createBodyForError(Response &response, int status_code, std::string filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		for (auto& p : errorHttps) {
			if (status_code == p.first) {
				response.body = p.second;
				return;
			}
		}
	}
	std::ostringstream sbody;
	sbody << file.rdbuf();
	response.body = sbody.str();
}

static void handleError(Response &response, ServerConfig config, int status_code) {
	setStatusCode(response, status_code);
	for (auto& p : config.error_pages) {
		if (status_code == p.first) {
			createBodyForError(response, status_code, p.second);
			createHeader(response, "text/html; charset=UTF-8");
		}
	}
	for (auto& p : errorHttps) {
		if (status_code == p.first) {
			response.body = p.second;
			createHeader(response, "text/html; charset=UTF-8");
		}
	}
}

static void createHeader(Response &response, std::string content_type) {
	response.header = "HTTP/1.1 " + std::to_string(response.status_code) + " " + response.status_code_str +"\r\n"
	"Date: " + getHttpDate() + "\r\n"
	"Server: OverThirty_Webserv\r\n"
	"Content-Type: " + content_type + "\r\n"
	"Content-Length: " + std::to_string(response.body.length()) + "\r\n"
	"Cache-Control: no-cache, private\r\n"
	+ "\r\n";
}

static void createBody(Response &response, ServerConfig config, std::string filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		handleError(response, config, 404);
	}
	std::ostringstream sbody;
	sbody << file.rdbuf();
	response.body = sbody.str();
}

static void handleGet(Response &response, ServerConfig config) {
	if (response.path == "/") {
		createBody(response, config, response.location.root + "/index.html");
		createHeader(response, "text/html; charset=UTF-8");
	}
	else if (response.location.autoindex == true) { //TODO check if it is a directory at all
		getAutoIndex(response, config);
	}
	else {
		createBody(response, config, response.location.root + response.path);
		if (response.status_code == 200) {
			for (auto& p : extensions) {
				if (endsWith(response.path, p.first)) {
					createHeader(response, p.second);
					break;
				}
			}
		}
	}
}

static void sortLocations(std::vector<LocationConfig> &locations_copy) {
	int n = locations_copy.size();
	for (int i = 0; i < n - 1; i++) {
		for (int j = 0; j < n - i - 1; j++) {
			if (locations_copy[j].path.length() < locations_copy[j + 1].path.length()) {
				std::swap(locations_copy[j], locations_copy[j + 1]);
			}
		}
	}
}

static void removeEndSlash(std::string &str) {
	if (str.length() > 1 && str.back() == '/') {
		str.pop_back();
	}
}

static LocationConfig getLocation(std::string path, ServerConfig config) {
	std::vector<LocationConfig> locations_copy = config.locations;

	sortLocations(locations_copy);
	removeEndSlash(path);

	for (auto location : locations_copy) {
		removeEndSlash(location.path);
		if (location.path.length() <= path.length()
			&& path.compare(0, location.path.length(), location.path) == 0) {
			return location;
		}
	}
	//TODO Replace with exception? Needed for compilation anyway even with exception?
	LocationConfig empty_location;
	return empty_location;
}

static void handleDelete(Response &response, ServerConfig config)
{
	std::string	full_path;

	// full_path = response.location.root + response.path;
	full_path = "./www/images/directory/example.txt"; // now hardcoded, later the version above.

	std::cout << "path: " << response.path << "\nroot: " << response.root << std::endl;
	try {
		if (!std::ifstream(full_path)) {
			throw std::runtime_error("Couldn't delete unexisting file: " + full_path);
		}
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
		handleError(response, config, 404);
		return ;
	}
	try {
		std::filesystem::remove_all(full_path); // remove_all removes even directories with files
		if (std::ifstream(full_path)) {
			throw std::runtime_error("Couldn't delete the file " + full_path);
		}
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
		handleError(response, config, 500);
	}
	std::cout << "Deleted the file: " + full_path << std::endl;
}

Response getResponse(std::string request, ServerConfig config, int status_code) {
	Response response;
	std::istringstream iss(request);
	iss >> response.method >> response.path >> response.version;

	// std::cout << ">>>" << "-------------------------------------------" << std::endl;
	response.location = getLocation(response.path, config);
	// std::cout << ">>>" << "Path: " << response.path << std::endl;
	// std::cout << ">>>" << "Location: " << response.location.path << std::endl;

	parseHeader(request);
	if (status_code != 200)
		handleError(response, config, status_code);
	else if (response.method == "GET")
		handleGet(response, config);
	else if (response.method == "DELETE")
		handleDelete(response, config);
	response.full_response = response.header + response.body;
	return response;
}
