#include "../inc/Request.hpp"
#include "../inc/utils.hpp"

const static std::string header_terminator = "\r\n\r\n";

Request::Request(std::vector<ServerConfig> configs) : _all_configs(configs) {
	_is_cgi = false;
}

e_req_state	Request::addToRequest(std::string part) {

	std::cout << "addToRequest()" << part <<std::endl;

	_raw_request += std::string(part);

	if (headerIsComplete()) {
		size_t body_start =
			_raw_request.find(header_terminator) + header_terminator.length();

		_headers = parseHeader(_raw_request.substr(0, body_start));

		setConfig();

		// TODO: .at() will throw if key is not found => catch => invalid request
		auto method = _headers.find("method");

		// no method field
		if (method == _headers.end()) {
			std::cerr << "Request::addToRequest(): no method field" << std::endl;
			handleCompleteRequest(body_start, 0, 400);
			return READY;
		}
		else if (method->second == "GET") {
			// std::cerr << "Request::addToRequest(): happy" << std::endl;
			handleCompleteRequest(body_start, 0, 200);
			return READY;
		}
		else if (method->second == "POST") {
			return handlePost(body_start);
		}
		else {
			std::cerr << "Request::addToRequest(): unknown method" << std::endl;
			handleCompleteRequest(body_start, 0, 400);
			return READY;
		}
	}
	else {
		return RECV_MORE;
	}
}

void	Request::setConfig() {
	// According to subject, the first server in the configs for a 
	// particular host:port will be the default that is used for 
	// requests that dont have server names match

	// at this point all the configs we have have identical host+port
	
	// in case there is no match, uses the first config
	_config = _all_configs.front();

	if (_headers.find("host") == _headers.end()) {
		std::cerr << "Request::setConfig(): no host field in header" << std::endl;
		return ;
	}

	for (auto it = _all_configs.begin(); it != _all_configs.end(); it++) {
		// select first config where header host field matches 
		// servername
		for (auto itt = it->server_names.begin();
			itt != it->server_names.end(); itt++) {
			if (_headers.at("host") == *itt) {
				_config = *it;
				return ;
			}
		}
	}
}

void Request::handleCompleteRequest(size_t body_start,size_t body_length, int status)
{
	// this may be unnecessary since we do not support pipelining
	//std::string whole_req = raw_request.substr(0, body_start + body_length);
	//raw_request.erase(0, body_start + body_length);

	getResponse(status);
}

e_req_state Request::handlePost(size_t header_end) {
	// if there is transfer-encoding, then content-length can be ignored
	if (_headers.find("transfer-encoding") != _headers.end()) {
		if (_headers.at("transfer-encoding") == "chunked\r") {
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
	if (_headers.find("content-length") == _headers.end()) {
		handleCompleteRequest(header_end, 0, 411);
		return READY;
	}

	// has content length
	size_t content_length = 0;
	try {
		content_length = std::stoul(_headers.at("content-length"));
	} catch (...) {
		std::cerr << "Client::handlePost(): failed to find Content-Length";
		std::cerr << std::endl;
		// TODO: handle invalid value in content-length
	}

	// client wants to send too big of a body
	if (_config.client_max_body_size != 0
		&& content_length > _config.client_max_body_size) {
		std::cerr << "Client body length larger than allowed" << std::endl;
		handleCompleteRequest(header_end, 0, 413);
		return READY;
	}

	// happy path
	if (_raw_request.length() >= header_end + content_length) {
		handleCompleteRequest(header_end, content_length, 200);
		return READY;
	} else {
		return RECV_MORE;
	}
}

// TODO: this may also trigger on the end of a chunked transfer which is no
// good
bool Request::headerIsComplete() {
	if (_raw_request.find("\r\n\r\n") != std::string::npos) {
		return true;
	}
	return false;
}

Response		Request::getRes() {
	return _response;
}

ServerConfig	Request::getConfig() {
	return _config;
}

std::map<std::string, std::string>	Request::getHeaders() {
	return _headers;
}

bool			Request::getIsCgi() {
	return _is_cgi;
}

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

// bool Request::isPostAllowed(std::string path, ServerConfig config) {
// 	LocationConfig location = getLocation(path, config);
// 	if (std::find(location.methods.begin(), location.methods.end(), "POST")
// 		!= location.methods.end()) {
// 		return true;
// 	}
// 	return false;
// }

void Request::setStatusCode(int status_code) {
	_response.status_code = status_code;
	for (auto& p : errorCodes) {
		if (status_code == p.first) {
			_response.status_code_str = p.second;
		}
	}
}

void Request::getAutoIndex() {
	try {
		_response.body = generateAutoIndex(_response.path, _config);
	} catch (...) {
		handleError(404);
	}
	createHeader("text/html; charset=UTF-8");
}

void Request::createBodyForError(std::string filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		for (auto& p : errorHttps) {
			if (_response.status_code == p.first) {
				_response.body = p.second;
				return;
			}
		}
	}
	std::ostringstream sbody;
	sbody << file.rdbuf();
	_response.body = sbody.str();
}

void Request::handleError(int status_code) {
	setStatusCode(status_code);
	for (auto& p : _config.error_pages) {
		if (_response.status_code == p.first) {
			createBodyForError(p.second);
		}
	}
	if (_response.body == "") {
		for (auto& p : errorHttps) {
			if (_response.status_code == p.first) {
				_response.body = p.second;
			}
		}
	}
	createHeader("text/html; charset=UTF-8");
}

void Request::createHeader(std::string content_type) {
	_response.header = "HTTP/1.1 " + std::to_string(_response.status_code) + " " + _response.status_code_str +"\r\n"
	"Date: " + getHttpDate() + "\r\n"
	"Server: OverThirty_Webserv\r\n"
	"Content-Type: " + content_type + "\r\n"
	"Content-Length: " + std::to_string(_response.body.length()) + "\r\n"
	"Cache-Control: no-cache, private\r\n";

	if (_response.status_code == 301) {
		_response.header += "Location: " + _response.redirect_path + "\r\n";
	}

	_response.header += "\r\n";
}

void Request::createBody(std::string filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open())
		return handleError(404);
	std::ostringstream sbody;
	sbody << file.rdbuf();
	_response.body = sbody.str();
}

void Request::handleDelete()
{
	std::string	full_path;

	// full_path = response.location.root + response.path;
	full_path = "./www/images/directory/example.txt"; // now hardcoded, later the version above.

	std::cout << "path: " << _response.path << "\nroot: " << _response.location.root << std::endl;
	try {
		if (!std::ifstream(full_path)) {
			throw std::runtime_error("Couldn't delete unexisting file: " + full_path);
		}
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
		handleError(404);
		return ;
	}
	try {
		std::filesystem::remove_all(full_path); // remove_all removes even directories with files
		if (std::ifstream(full_path)) {
			throw std::runtime_error("Couldn't delete the file " + full_path);
		}
	} catch (std::exception &e) {
		std::cout << e.what() << std::endl;
		handleError(500);
	}
	std::cout << "Deleted the file: " + full_path << std::endl;
}

void Request::handleGet() {
	std::cout << "handleGet(): " << _response.path << std::endl;
	if (_response.path == "/") {
		createBody(_response.location.root + "/index.html");
		if (_response.status_code == 200)
			createHeader("text/html; charset=UTF-8");
	}
	else if (_response.location.autoindex == true && _response.is_directory == true) {
		getAutoIndex();
	}
	else {
		createBody(_response.location.root + _response.path);
		if (_response.status_code == 200) {
			for (auto& p : extensions) {
				if (endsWith(_response.path, p.first)) {
					createHeader(p.second);
					break;
				}
			}
		}
	}
}

void Request::getLocation() {
	std::vector<LocationConfig> locations_copy = _config.locations;
	std::string temp_path = _response.path;

	int n = locations_copy.size(); // sort locations to find the longest match
	for (int i = 0; i < n - 1; i++) {
		for (int j = 0; j < n - i - 1; j++) {
			if (locations_copy[j].path.length() < locations_copy[j + 1].path.length()) {
				std::swap(locations_copy[j], locations_copy[j + 1]);
			}
		}
	}
	removeEndSlash(temp_path);
	for (auto location : locations_copy) {
		removeEndSlash(location.path);
		if (location.path.length() <= temp_path.length()
			&& temp_path.compare(0, location.path.length(), location.path) == 0) {
			_response.location = location;
			return ;
		}
	}
}

void Request::validateRequest() {
	std::string root_and_path = _response.location.root + _response.path;

	// check if path/extension suggests cgi
	// is the method allowed in config (compare response.location to config)

	//check for not allowed methods
	if (_response.method != "GET"
		&& _response.method != "POST"
		&& _response.method != "DELETE") {
		_response.status_code = 405;
	}

	// is the path a file or directory and does the directory have a trailing slash
	else if (std::filesystem::is_directory(root_and_path)) {
		_response.is_directory = true;
		if (_response.path.back() != '/') {
			_response.redirect_path = _response.path + '/';
			_response.status_code = 301;
			return ;
		}
	}
	
}

void Request::getResponse(int status_code) {
	_response.method = _headers.find("method")->second;
	_response.path = _headers.find("path")->second;
	_response.version = _headers.find("version")->second;
	
	getLocation();

	std::cout << "|  " << "Path: " << _response.path << std::endl;
	
	setStatusCode(status_code);
	if (_response.status_code == 200)
	validateRequest();
	std::cout << "|  " << "Validate Result: " << _response.status_code << std::endl;
	
	if (_headers.at("path") == "/who.py"){
		_is_cgi = true;
	}
	
	if (_response.status_code != 200)
		handleError(_response.status_code);
	else if (_response.method == "GET")
		handleGet();
	else if (_response.method == "DELETE")
		handleDelete();
	_response.full_response = _response.header + _response.body;

	std::cout << "|  " << "-----------------------------------" << std::endl;
}
