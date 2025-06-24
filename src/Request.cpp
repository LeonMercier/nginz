#include "../inc/Request.hpp"


const static std::string header_terminator = "\r\n\r\n";

Request::Request(std::vector<ServerConfig> configs) : all_configs(configs) {}

e_req_state	Request::addToRequest(std::string part) {

	std::cout << "addToRequest()" << part <<std::endl;

	raw_request += std::string(part);

	if (headerIsComplete()) {
		size_t body_start =
			raw_request.find(header_terminator) + header_terminator.length();

		headers = parseHeader(raw_request.substr(0, body_start));

		setConfig();

		// TODO: .at() will throw if key is not found => catch => invalid request
		auto method = headers.find("method");

		// no method field
		if (method == headers.end()) {
			std::cerr << "Request::addToRequest(): no method field" << std::endl;
			handleCompleteRequest(body_start, 0, 400);
			return READY;
		}
		if (method->second == "GET") {
			// std::cerr << "Request::addToRequest(): happy" << std::endl;
			handleCompleteRequest(body_start, 0, 200);
			return READY;
		} else if (method->second == "POST") {
			return handlePost(body_start);

		} else {
			std::cerr << "Request::addToRequest(): unknown method" << std::endl;
			handleCompleteRequest(body_start, 0, 400);
			return READY;
		}
	} else {
		return RECV_MORE;
	}
}

void	Request::setConfig() {
	// According to subject, the first server in the configs for a 
	// particular host:port will be the default that is used for 
	// requests that dont have server names match

	// at this point all the configs we have have identical host+port
	
	// in case there is no match, uses the first config
	config = all_configs.front();

	if (headers.find("host") == headers.end()) {
		std::cerr << "Request::setConfig(): no host field in header" << std::endl;
		return ;
	}

	for (auto it = all_configs.begin(); it != all_configs.end(); it++) {
		// select first config where header host field matches 
		// servername
		for (auto itt = it->server_names.begin();
			itt != it->server_names.end(); itt++) {
			if (headers.at("host") == *itt) {
				config = *it;
				return ;
			}
		}
	}
}

void Request::handleCompleteRequest(
	size_t body_start,size_t body_length, int status)
{
	// this may be unnecessary since we do not support pipelining
	//std::string whole_req = raw_request.substr(0, body_start + body_length);
	//raw_request.erase(0, body_start + body_length);

	getResponse(status);
}

e_req_state Request::handlePost(size_t header_end) {
	// if there is transfer-encoding, then content-length can be ignored
	if (headers.find("transfer-encoding") != headers.end()) {
		if (headers.at("transfer-encoding") == "chunked\r") {
			std::cout << "receiving chunked transfer" << std::endl;
			// TODO: implement checks for chunked transfer xD
			return RECV_MORE;
		}
		// header has transfer-encoding but it is not set to chunked =>
		// we proceed to look for content-length
		//
		// TODO: maybe just reject requests with tranfer-encoding set to
		// something else
	}

	// content-length missing
	if (headers.find("content-length") == headers.end()) {
		handleCompleteRequest(header_end, 0, 411);
		return READY;
	}

	// has content length
	size_t content_length = 0;
	try {
		content_length = std::stoul(headers.at("content-length"));
	} catch (...) {
		std::cerr << "Client::handlePost(): failed to find Content-Length";
		std::cerr << std::endl;
		// TODO: handle invalid value in content-length
	}

	// client wants to send too big of a body
	if (config.client_max_body_size != 0
		&& content_length > config.client_max_body_size) {
		std::cerr << "Client body length larger than allowed" << std::endl;
		handleCompleteRequest(header_end, 0, 413);
		return READY;
	}

	// happy path
	if (raw_request.length() >= header_end + content_length) {
		handleCompleteRequest(header_end, content_length, 200);
		return READY;
	} else {
		return RECV_MORE;
	}
}

// TODO: this may also trigger on the end of a chunked transfer which is no
// good
bool Request::headerIsComplete() {
	if (raw_request.find("\r\n\r\n") != std::string::npos) {
		return true;
	}
	return false;
}

Response		Request::getRes() {
	return response;
}

ServerConfig	Request::getConfig() {
	return config;
}

std::map<std::string, std::string>	Request::getHeaders() {
	return headers;
}

bool			Request::getIsCgi() {
	return is_cgi;
}

// #include "../inc/request_handler.hpp"
// #include "../inc/Webserv.hpp"
// #include "../inc/parse_header.hpp"

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

int Request::getPostContentLength (std::string request) {
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

bool Request::isPostAllowed(std::string path, ServerConfig config) {
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

static void handleGet(Response &response, ServerConfig config) {
	std::cout << "handlleGet(): " << response.path << std::endl;
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

static int	validateRequest(std::map<std::string, std::string> header, Response &response, ServerConfig &config)
{
	// check if path/extension suggests cgi
	// is the method allowed in config (compare response.location to config)
	return 0;
}

void Request::getResponse(int status_code) {
	//  std::cout << "getResponse" << raw_request << std::endl;
	//  Response response;
	std::istringstream iss(raw_request);
	iss >> response.method >> response.path >> response.version;
	//std::cout << response.method << std::endl;

	// std::cout << ">>>" << "-------------------------------------------" << std::endl;
	response.location = getLocation(response.path, config);
	// std::cout << ">>>" << "Path: " << response.path << std::endl;
	// std::cout << ">>>" << "Location: " << response.location.path << std::endl;

	// if (status_code == 200) {
	// 	status_code = validateRequest(getHeaders(), response, config);
	// }

	// if request looks like CGI
	// set cgi_pid to the pid of the process
	// state = WAIT_CGI;

	

	if (status_code != 200)
		handleError(response, config, status_code);
	else if (response.method == "GET")
		handleGet(response, config);
	else if (response.method == "DELETE")
		handleDelete(response, config);
	response.full_response = response.header + response.body;
	// return response;
}
