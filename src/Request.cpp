#include "../inc/Request.hpp"
#include "../inc/utils.hpp"

const static std::string header_terminator = "\r\n\r\n";

Request::Request(std::vector<ServerConfig> configs) : _all_configs(configs) {
	_is_cgi = false;
}

e_req_state	Request::addToRequest(std::string part) {

	// std::cout << "addToRequest()" << std::endl;
	// std::cout << part << std::endl;

	_raw_request += std::string(part);

	// headers will not be parsed multiple times when they have already been
	// received
	if (_receiving_chunked) {
		if (handleChunked(0) == RECV_MORE) {
			return RECV_MORE;
		}
		handleCompleteRequest(200);
		return READY;
	}

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
			handleCompleteRequest(400);
			return READY;
		}
		else if (method->second == "GET") {
			// std::cerr << "Request::addToRequest(): happy" << std::endl;
			handleCompleteRequest(200);
			return READY;
		}
		else if (method->second == "POST") {
			if (handlePost(body_start) == READY) {
				handleCompleteRequest(200);
				return READY;
			} else {
				return RECV_MORE;
			}
		}
		else {
			std::cerr << "Request::addToRequest(): unknown method" << std::endl;
			handleCompleteRequest(400);
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

void Request::handleCompleteRequest(int status)
{
	// this may be unnecessary since we do not support pipelining
	//std::string whole_req = raw_request.substr(0, body_start + body_length);
	//raw_request.erase(0, body_start + body_length);

	getResponse(status);
}

// returns true when the final chunk has been parsed
static bool extractChunks(std::string &raw_request,
						  std::vector<std::string> &chunks)
{
	static size_t left_to_read = 0;
	std::string chunk;

	while (raw_request.length() > 0) {
		if (left_to_read == 0) {
			try {
				raw_request.erase(0, raw_request.find_first_not_of("\r\n"));
				std::string tmp = raw_request.substr(0, raw_request.find("\r\n"));
				left_to_read = std::stoi(tmp, nullptr, 16);
				raw_request.erase(0, tmp.length() + 2);
				// std::cout << "left_to_read: " << left_to_read << std::endl;
			} catch (...) {
				std::cerr << "Request::extractChunks(): failed to parse chunk"
					"size" << std::endl;
				// TODO: propagate error to return error page
				return true;
			}
			// end of transmission is signaled by a chunk of size zero
			if (left_to_read == 0) {
				return true;
			}
		}
		// found an entire chunk
		if (raw_request.length() >= left_to_read) {
			chunk = raw_request.substr(0, left_to_read);
			raw_request.erase(0, left_to_read);
			left_to_read = 0;
			chunks.push_back(chunk);
		} else {
			// raw_request contains an incomplete chunk; need to recv more
			return false;
		}
	}
	return false;
}

e_req_state Request::handleChunked(size_t header_end) {
	 // std::cout << "handleChunked(): " << std::endl;

	std::vector<std::string> chunks;
	bool wasFinalChunk = false;
	
	// TODO: check that generated filename doesnt already
	// exist; generate new names until we get a non existent one
	if (!_receiving_chunked) {
		_tmp_filename_infile = generateTempFilename();
	}

	// ios::app => write to the end of the file
	std::ofstream file(_tmp_filename_infile, std::ios::binary | std::ios::app);

	// this is the initial call to this function
	if (!_receiving_chunked) {
		// std::cout << "handleChunked(): initial call" << _raw_request << std::endl;
		_receiving_chunked = true;
		_has_tmp_infile = true;
		_raw_request.erase(0, header_end);
		if (_raw_request.empty()) {
			// first recv() only contained a header
			return RECV_MORE;
		}
	} else {
		// std::cout << "handleChunked(): further call" << std::endl;
	}
	wasFinalChunk = extractChunks(_raw_request, chunks);
	for (auto it = chunks.begin(); it != chunks.end(); it++) {
		file << *it;
	}

	file.close();
	if (wasFinalChunk) {
		// std::cout << "handleChunked(): final chunk" << std::endl;
		// TODO: do we need to reset receiving_chunked or will the Request
		// always be destroyed after this?
		_receiving_chunked = false;
		return READY;
	} else {
		return RECV_MORE;
	}
}

e_req_state Request::handlePost(size_t body_start) {
	//method is not allowed in config
	initResponseStruct(200);
	if (methodIsNotAllowed()) {
		handleCompleteRequest(405);
		return READY;
	}

	// if there is transfer-encoding, then content-length can be ignored
	if (_headers.find("transfer-encoding") != _headers.end()) {
		if (_headers.at("transfer-encoding") == "chunked") {
			// std::cout << "receiving chunked transfer" << std::endl;
			return handleChunked(body_start);
		}
		// header has transfer-encoding but it is not set to chunked =>
		// we proceed to look for content-length
		//
		// TODO: maybe just reject requests with tranfer-encoding set to
		// something else
	}

	// content-length missing
	if (_headers.find("content-length") == _headers.end()) {
		handleCompleteRequest(411);
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
		handleCompleteRequest(413);
		return READY;
	}
	// happy path
	if (_raw_request.length() >= body_start + content_length) {
		handleCompleteRequest(200);
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

LocationConfig 	Request::getLocation()
{
	return _location;
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

std::string		Request::getPath(){
	return _path;
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
	_status_code = status_code;
	for (auto& p : errorCodes) {
		if (status_code == p.first) {
			_status_code_str = p.second;
		}
	}
}

void Request::getAutoIndex() {
	try {
		_response.body = generateAutoIndex(_path, _config);
	} catch (...) {
		handleError(404);
	}
	createHeader("text/html; charset=UTF-8");
}

void Request::createBodyForError(std::string filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		for (auto& p : errorHttps) {
			if (_status_code == p.first) {
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
		if (_status_code == p.first) {
			createBodyForError(p.second);
		}
	}
	if (_response.body == "") {
		for (auto& p : errorHttps) {
			if (_status_code == p.first) {
				_response.body = p.second;
			}
		}
	}
	createHeader("text/html; charset=UTF-8");
}

void Request::createHeader(std::string content_type) {
	_response.header = "HTTP/1.1 " + std::to_string(_status_code) + " " + _status_code_str +"\r\n"
	"Date: " + getHttpDate() + "\r\n"
	"Server: OverThirty_Webserv\r\n"
	"Content-Type: " + content_type + "\r\n"
	"Content-Length: " + std::to_string(_response.body.length()) + "\r\n"
	"Cache-Control: no-cache, private\r\n";

	if (_status_code == 301) {
		_response.header += "Location: " + _response.redirect_path + "\r\n";
	}

	_response.header += "\r\n";
}

void Request::createBody(std::string filename) {
	try {
		_response.body = fileToString(filename);
	} catch (const std::ios_base::failure& e){
		handleError(404);
	}
}

void Request::handleDelete()
{
	std::string	full_path;

	// full_path = location.root + path;
	full_path = "./www/images/directory/example.txt"; // now hardcoded, later the version above.

	std::cout << "path: " << _path << "\nroot: " << _location.root << std::endl;
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
	if (_path == "/") {
		createBody(_location.root + "/index.html");
		if (_status_code == 200)
			createHeader("text/html; charset=UTF-8");
	}
	else if (_location.autoindex == true && _is_directory == true) {
		getAutoIndex();
	}
	else {
		createBody(_location.root + _path);
		if (_status_code == 200) {
			for (auto& p : extensions) {
				if (endsWith(_path, p.first)) {
					createHeader(p.second);
					break;
				}
			}
		}
	}
}

void Request::checkLocation() {
	std::vector<LocationConfig> locations_copy = _config.locations;
	std::string temp_path = _path;

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
			_location = location;
			return ;
		}
	}
}

bool Request::methodIsNotAllowed() {
	if (std::find(_location.methods.begin(), _location.methods.end(),
			_method) == _location.methods.end()) {
		return true;
	}
	return false;
}

void Request::validateRequest() {
	// is it http version 1.1
	if (_version != "HTTP/1.1") {
		_status_code = 505;
	}

	// is the method supported by our server
	else if (_method != "GET"
		&& _method != "POST"
		&& _method != "DELETE") {
		_status_code = 501;
	}

	// is the method allowed in config
	else if (methodIsNotAllowed())
		_status_code = 405;

	// is the path a file or directory
	else if (std::filesystem::is_directory(_location.root + _path)) {
		_is_directory = true;
		// check for redirect error
		if (_path.back() != '/') {
			_response.redirect_path = _path + '/';
			_status_code = 301;
		}
	}
}

void Request::initResponseStruct(int status_code) {
	setStatusCode(status_code);
	_method = _headers.find("method")->second;
	_path = _headers.find("path")->second;
	_version = _headers.find("version")->second;
	checkLocation();
}

void Request::printRequest() {
	std::cout << "|  " << "Path: " << _path << std::endl;
	std::cout << "|  " << "Method: " << _method << std::endl;
	std::cout << "|  " << "Version: " << _version << std::endl;
	std::cout << "|  " << "Status: " << _status_code << std::endl;
	//std::cout << "|  " << "Location Path: " << location.path << std::endl;
	//std::cout << "|  " << "Redirect Path: " << response.redirect_path << std::endl;
	//std::cout << "|  " << "Is Directory: " << is_directory << std::endl;

	for (auto i: _location.methods)
		std::cout << "|  " << "Config Methods: " << i << std::endl;

	std::cout << "|  " << "-----------------------------------" << std::endl;
}

void Request::handleCgi() {
	if (_location.cgi_path_py != "/usr/bin/python3")
		handleError(400);
	else
		_is_cgi = true;
}

void Request::getResponse(int status_code) {

	initResponseStruct(status_code);
	if (_status_code == 200) {
		validateRequest();
		std::cout << "|  " << "Status After Validation: " << _status_code << std::endl;
	}

	if (_status_code != 200)
		handleError(_status_code);
	else if (endsWith(_path, ".py")){
		handleCgi();
	}
	else if (_method == "GET")
		handleGet();
	else if (_method == "DELETE")
		handleDelete();
	_response.full_response = _response.header + _response.body;
	printRequest(); // Remove
}
