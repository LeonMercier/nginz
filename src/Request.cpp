#include "../inc/Request.hpp"
#include "../inc/utils.hpp"

const static std::string header_terminator = "\r\n\r\n";

Request::Request(std::vector<ServerConfig> configs) : _all_configs(configs) {
	_is_cgi = false;
}

void	Request::addToRequest(std::string part) {
	try {
		_raw_request += std::string(part);
	} catch (...) {
		handleCompleteRequest(413);
		return ;
	}

	if (_receiving_chunked) {
		try {
			handleChunked();
		} catch (std::exception &e) {
			handleCompleteRequest(500);
			return ;
		}
		if (_state == READY) {
			handleCompleteRequest(200);
		}
		return ;
	}

	if (_state == RECV_HEADER) {
		if (!headerIsComplete()) {
			if (_raw_request.length() > 8192) {
				handleCompleteRequest(413);
			}
			return ;
		}
		_state = RECV_BODY;
		size_t body_start =
			_raw_request.find(header_terminator) + header_terminator.length();
		try {
			_headers = parseHeader(_raw_request.substr(0, body_start));
		} catch (...) {
			handleCompleteRequest(500);
			return;
		}
		_raw_request.erase(0, body_start);

		setConfig();

		auto method = _headers.find("method");

		// no method field
		if (method == _headers.end() || method->second == "") {
			std::cerr << "Request::addToRequest(): no method field" << std::endl;
			handleCompleteRequest(400);
			return ;
		}
		else if (method->second == "GET" || method->second == "DELETE") {
			handleCompleteRequest(200);
			return ;
		}
		else if (method->second != "POST") {
			std::cerr << "Request::addToRequest(): unknown method" << std::endl;
			handleCompleteRequest(405);
			return ;
		}
	}

	if (_state == RECV_BODY || _state == RECV_MORE_BODY) {
		auto method = _headers.find("method");
		if (method != _headers.end() && method->second == "POST"){
			handlePost();
		}
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
		// select first config where header host field matches servername
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
	_state = READY;
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
			} catch (std::exception &e) {
				std::cerr << "Request::extractChunks(): failed to parse chunk"
					"size" << std::endl;
				// rethrowing the same exception
				throw;
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

void Request::handleChunked() {
	std::vector<std::string> chunks;
	bool wasFinalChunk = false;
	
	if (!_receiving_chunked) {
		_post_body_filename = generateTempFilename();
	}

	// ios::app => write to the end of the file
	std::ofstream file(_post_body_filename, std::ios::binary | std::ios::app);

	// this is the initial call to this function
	if (!_receiving_chunked) {
		_receiving_chunked = true;
		if (_raw_request.empty()) {
			return ;
		}
	}
	wasFinalChunk = extractChunks(_raw_request, chunks);
	for (auto it = chunks.begin(); it != chunks.end(); it++) {
		file << *it;
	}

	file.close();
	if (wasFinalChunk) {
		_receiving_chunked = false;
		_state = READY;
	}
}

void	Request::initialPost() {
	_state = RECV_MORE_BODY;
	//method is not allowed in config
	initResponseStruct(200);
	if (methodIsNotAllowed()) {
		handleCompleteRequest(405);
		return ;
	}

	// if there is transfer-encoding, then content-length can be ignored
	if (_headers.find("transfer-encoding") != _headers.end()) {
		if (_headers.at("transfer-encoding") == "chunked") {
			try {
				handleChunked();
			} catch (std::exception &e) {
				handleCompleteRequest(500);
				return ;
			}
			return ;
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
		return ;
	}

	// has content length
	_content_length = 0;
	long temp = 0;
	try {
		temp = std::stol(_headers.at("content-length"));
	} catch (...) {
		std::cerr << "Client::handlePost(): failed to parse Content-Length";
		std::cerr << std::endl;
		handleCompleteRequest(400);
		return ;
	}
	if (temp < 0) {
		handleCompleteRequest(400);
		return ;
	}
	else {
		_content_length = (size_t)temp;
	}
	// check if client wants to send too big of a body
	if (_config.client_max_body_size != 0
		&& _content_length > _config.client_max_body_size) {
		std::cerr << "Client body length larger than allowed" << std::endl;
		handleCompleteRequest(413);
		return ;
	}
	_post_body_filename = generateTempFilename();
}

void Request::handlePost() {
	if (_state == RECV_BODY) {
		initialPost();
	}
	if (_state == READY) {
		return ;
	}
	std::ofstream file(_post_body_filename, std::ios::app | std::ios::binary);
	if (_raw_request.length() + _body_bytes_read <= _content_length) {
		file << _raw_request;
	} else {
		file << _raw_request.substr(0, _content_length - _body_bytes_read);
	}
	_body_bytes_read += _raw_request.length();
	_raw_request = "";
	if (_body_bytes_read >= _content_length) {
		file.close();
		handleCompleteRequest(200);
	} else {
		file.close();
	}
}

bool Request::headerIsComplete() {
	if (_raw_request.find("\r\n\r\n") != std::string::npos)
		return true;
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

void	Request::setIsCgi(bool new_state) {
	_is_cgi = new_state;
}

bool			Request::getConnectionTypeIsClose() {
	return _connection_type_is_close;
}

std::string		Request::getPostBodyFilename(){
	return _post_body_filename;
}

t_method			Request::getMethod(){
	if (_method == "GET")
		return GET;
	if (_method == "POST")
		return POST;
	if (_method == "DELETE")
		return DELETE;
	return ERR_METHOD;
}

e_req_state	Request::getState() {
	return _state;
}

void Request::setStatusCode(int status_code) {
	_status_code = status_code;
	for (auto& p : errorCodes) {
		if (status_code == p.first) {
			_status_code_str = p.second;
		}
	}
}

std::string Request::generateAutoIndex(const std::string& request_path, std::string &root)
{
    std::string clean_path = request_path;

    // Ensuring it starts with a slash
    if (clean_path.empty() || clean_path[0] != '/')
        clean_path = "/" + clean_path;

    std::filesystem::path full_path = root + clean_path;

    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html>\n<head><title>Index of " << request_path << "</title></head>\n";
    html << "<body>\n<h1>" << request_path << "</h1>\n<ul>\n";

    try
    {
        for (const auto& entry : std::filesystem::directory_iterator(full_path)) //
        {
            std::string name = entry.path().filename().string();
            if (std::filesystem::is_directory(entry))
                name += "/";

            html << "<li><a href=\"" << request_path;
            if (!request_path.empty() && request_path.back() != '/')
                html << '/';
            html << name << "\">" << name << "</a></li>\n";
        }
    } catch (const std::filesystem::filesystem_error& e) {
        html << "<p>Error reading directory: " << e.what() << "</p>\n";
    }

    html << "</ul>\n</body>\n</html>\n";
    return html.str();
}

void Request::getAutoIndex() {
	try {
		_response.body = generateAutoIndex(_path, _location.root);
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
		if (_status_code == p.first)
			createBodyForError(p.second);
	}
	if (_response.body == "") {
		for (auto& p : errorHttps) {
			if (_status_code == p.first)
				_response.body = p.second;
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

	if (_status_code == 201)
		_response.header += "Location: " + _location.root + _path + "\r\n";
	else if (_status_code == 301)
		_response.header += "Location: " + _response.redirect_path + "\r\n";
	else if (_status_code == 408) {
		_response.header += "Connection: close\r\n";
		_connection_type_is_close = true;
	}
	else
		_response.header += "Connection: keep-alive\r\n";

	_response.header += "\r\n";
}

void Request::createBody(std::string filename) {
	try {
		_response.body = fileToString(filename);
	} catch (...){
		handleError(404);
	}
}

void Request::handleDelete()
{
	std::string	full_path = _location.root + _path;
	try {
		if (!std::ifstream(full_path))
			throw std::runtime_error("Couldn't delete unexisting file: " + full_path);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		handleError(404);
		return ;
	}
	try {
		std::filesystem::remove_all(full_path); // remove_all removes even directories with files
		if (std::ifstream(full_path))
			throw std::runtime_error("Couldn't delete the file " + full_path);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		handleError(500);
	}
}


void Request::handleGet() {
	if (_path == "/" || (_location.autoindex == false && _is_directory == true)) {
		if (_location.index != "") {
			std::string temp = _location.root + "/" + _location.index;
			if (!std::filesystem::exists(temp)) {
				handleError(404);
				return ;
			}
			else
				createBody(temp);
		}
		else
			createBody(_location.root + "/index.html");
		createHeader("text/html; charset=UTF-8");
	}
	else if (_location.autoindex == true && _is_directory == true)
		getAutoIndex();
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

	// sort locations by largest to smallest
	int n = locations_copy.size(); 
	for (int i = 0; i < n - 1; i++) {
		for (int j = 0; j < n - i - 1; j++) {
			if (locations_copy[j].path.length() < locations_copy[j + 1].path.length()) {
				std::swap(locations_copy[j], locations_copy[j + 1]);
			}
		}
	}
	removeEndSlash(temp_path);
	// find the location that most completely matches the path
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

bool Request::fileExtensionIsSupported(std::string &path) {
	for (auto& p : extensions) {
		if (endsWith(path, p.first)) {
			return true;
		}
	}
	return false;
}

void Request::validateRequest() {

	// is any part of the first request line missing
	if (_method == "" || _path == "" || _version == "") {
		handleError(400);
	}
	// does the path contain the forbidden ".."
	else if (_path.find("/../") != std::string::npos || endsWith(_path, "/..")) {
		handleError(403);
	}
	// does the path size exceed the maximum uri size
	else if (_path.size() > _MAX_URI_SIZE) {
		handleError(414);
	}
	// is it http version 1.1
	else if (_version != "HTTP/1.1") {
		handleError(505);
	}
	// is the method supported by our server
	else if (_method != "GET"
		&& _method != "POST"
		&& _method != "DELETE") {
		handleError(501);
	}
	// is the method allowed in config
	else if (methodIsNotAllowed())
		handleError(405);
	// does the config contain return code 301 for redirection
	else if (_location.return_code == 301) {
		_response.redirect_path = _location.return_url;
		handleError(301);
	}
	// is the path a directory
	else if (std::filesystem::is_directory(_location.root + _path)) {
		_is_directory = true;
		// check for redirect error
		if (_path.back() != '/' && _method != "POST") {
			_response.redirect_path = _path + '/';
			handleError(301);
		}
		//check for extension type not supported for the location index
		else if (_location.index != "" && !fileExtensionIsSupported(_location.index)) {
			handleError(415);
		}
	}
	// does the file exist
	else if (!std::filesystem::exists(_location.root + _path)) {
		handleError(404);
	}
	//check for extension type not supported
	else if (!fileExtensionIsSupported(_path)) {
		handleError(415);
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
	std::cout << "|  " << "Index: " << _location.index << std::endl;
	std::cout << "|  " << "Root: " << _location.root << std::endl;
	//std::cout << "|  " << "Location Path: " << location.path << std::endl;
	//std::cout << "|  " << "Redirect Path: " << response.redirect_path << std::endl;
	//std::cout << "|  " << "Is Directory: " << is_directory << std::endl;

	for (auto i: _location.methods)
		std::cout << "|  " << "Config Methods: " << i << std::endl;

	std::cout << "|  " << "-----------------------------------" << std::endl;
}

void Request::handleCgi() {
	if (_location.cgi_path_py == "")
		handleError(400);
	else
		_is_cgi = true;
}

void	Request::separateMultipart() {
	// Multipart::init() can throw exceptions related to file handling
	try {
		_multipart.init(_headers, _post_body_filename, _location.root);
	} catch (std::exception &e) {
		std::cerr << "Multipart error: " << e.what() << std::endl;
		handleError(500);
	}
}

void	Request::respondPost() {
	// the POST body is in the file _post_body_filename
	auto content_type_iter = _headers.find("content-type");
	if (content_type_iter != _headers.end()) {
		if (content_type_iter->second.find(
			"multipart/form-data") != std::string::npos)
		{
			separateMultipart();
			handleError(201);
		} else {
			handleError(415);
		}
	}
	else {
		handleError(415);
	}
}

void Request::getResponse(int status_code) {

	if (status_code != 200) {
		handleError(status_code);
		_response.full_response = _response.header + _response.body;
		return ;
	}
	initResponseStruct(status_code);
	validateRequest();
	if (_status_code != 200) {
		_response.full_response = _response.header + _response.body;
		return ;
	}
	else if (endsWith(_path, ".py"))
		handleCgi();
	else if (_method == "GET")
		handleGet();
	else if (_method == "POST")
		respondPost();
	else if (_method == "DELETE")
		handleDelete();
	_response.full_response = _response.header + _response.body;
	// printRequest(); // Remove
}
