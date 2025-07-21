#include "../inc/Request.hpp"
#include "../inc/utils.hpp"

const static std::string header_terminator = "\r\n\r\n";

Request::Request(std::vector<ServerConfig> configs) : _all_configs(configs) {
	_is_cgi = false;
}

void	Request::addToRequest(std::string part) {

	// std::cout << "addToRequest()" << std::endl;
	// std::cout << part << std::endl;

	_raw_request += std::string(part);

	if (_receiving_chunked) {
		handleChunked();
		if (_state == READY) {
			handleCompleteRequest(200);
		}
		return ;
	}

	if (_state == RECV_HEADER) {
		if (!headerIsComplete()) {
			// TODO: prevent client from sending an infinitely long header
			return ;
		}
		_state = RECV_BODY;
		size_t body_start =
			_raw_request.find(header_terminator) + header_terminator.length();

		_headers = parseHeader(_raw_request.substr(0, body_start));
		_raw_request.erase(0, body_start);
		// for (auto it = _headers.begin(); it != _headers.end(); it++) {
		// 	std::cout << it->first << " = " << it->second << std::endl;
		// }

		setConfig();

		// TODO: .at() will throw if key is not found => catch => invalid request
		auto method = _headers.find("method");

		// no method field
		if (method->second == "") {
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

void Request::handleChunked() {
	 // std::cout << "handleChunked(): " << std::endl;

	std::vector<std::string> chunks;
	bool wasFinalChunk = false;
	
	// TODO: check that generated filename doesnt already
	// exist; generate new names until we get a non existent one
	if (!_receiving_chunked) {
		_post_body_filename = generateTempFilename();
	}

	// ios::app => write to the end of the file
	std::ofstream file(_post_body_filename, std::ios::binary | std::ios::app);

	// this is the initial call to this function
	if (!_receiving_chunked) {
		// std::cout << "handleChunked(): initial call" << _raw_request << std::endl;
		_receiving_chunked = true;
		if (_raw_request.empty()) {
			// first recv() only contained a header
			return ;
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
			// std::cout << "receiving chunked transfer" << std::endl;
			handleChunked();
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
	try {
		_content_length = std::stoul(_headers.at("content-length"));
	} catch (...) {
		std::cerr << "Client::handlePost(): failed to parse Content-Length";
		std::cerr << std::endl;
		// TODO: handle invalid value in content-length
	}
	if (_content_length == 0)
	{
		std::cout << "CONTENT-LENGTH = 0\n";
	}
	// client wants to send too big of a body
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
	_body_bytes_read += _raw_request.length();
	file << _raw_request;
	_raw_request = "";
	if (_body_bytes_read >= _content_length) {
		std::cout << "handlePost(): complete POST" << std::endl;
		// std::cout << _raw_request << std::endl;
		handleCompleteRequest(200);
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
	{
		return GET;
	}
	if (_method == "POST")
	{
		return POST;
	}
	if (_method == "DELETE")
	{
		return DELETE;
	}
	return ERR_METHOD;
}

e_req_state	Request::getState() {
	return _state;
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
	else if (_status_code == 408) {
		// std::cout << "CREATING RESPONSE HEADER FOR 408\n\n";
		_response.header += "Connection: close\r\n";
		_connection_type_is_close = true;
	}
	else {
		_response.header += "Connection: keep-alive\r\n";
	}

	_response.header += "\r\n";
	// std::cout << "response_header:\n" << _response.header << std::endl;
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

	full_path = _location.root + _path;
	// std::cout << "path: " << full_path << std::endl;
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
	if (_path == "/" || (_location.autoindex == false && _is_directory == true)) {
		if (_location.index != "") {
			std::string temp = _location.root + "/" + _location.index;
			std::cout << "|  " << "found a directory and not auto-index" << std::endl;
			std::cout << "|  " << "COMBINED THING: " << temp << std::endl;
			if (!std::filesystem::exists(temp)) {
				std::cout << "|  " << "Does not exist" << std::endl;
				handleError(404);
			}
			else {
				std::cout << "|  " << "It exists so do it" << std::endl;
				createBody(temp);
			}
		}
		else
			createBody(_location.root + "/index.html");
		if (_status_code == 200)
			createHeader("text/html; charset=UTF-8");
	}
	else if (_location.autoindex == true && _is_directory == true) {
		getAutoIndex();
	}
	else {
		if (!std::filesystem::exists(_location.root + _path)) {
			std::cout << "|  " << "Does not exist 2" << std::endl;
			handleError(404);
			return ;
		}
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

bool Request::fileExtensionIsSupported(std::string &path) {
	//std::cout << "|  " << "path in file ext check = " << path << std::endl;
	for (auto& p : extensions) {
		//std::cout << "|  " << "p.first = " << p.first << std::endl;
		if (endsWith(path, p.first)) {
			return true;
		}
	}
	return false;
}

void Request::validateRequest() {

	// is any part of the first request line missing
	if (_method == "" || _path == "" || _version == "") {
		_status_code = 400;
	}
	else if (_path.find("/../") != std::string::npos || endsWith(_path, "/..")) {
		_status_code = 403;
	}
	// is it http version 1.1
	else if (_version != "HTTP/1.1") {
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
	//is the location return code 301 for redirection
	else if (_location.return_code == 301) {
		_response.redirect_path = _location.return_url;
		_status_code = 301;
	}
	// is the path a directory
	else if (std::filesystem::is_directory(_location.root + _path)) {
		std::cout << "|  " << "is directory check in validation" << std::endl;
		_is_directory = true;
		// check for redirect error
		if (_path.back() != '/') {
			_response.redirect_path = _path + '/';
			_status_code = 301;
		}
		//check for extension type not supported for the location index
		else if (_location.index != "" && !fileExtensionIsSupported(_location.index)) {
			std::cout << "|  " << "Error for ext not supported  with index" << std::endl;
			_status_code = 415;
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
	// Now we have to read _post_body_filename and separate the individual 
	// files from it. 
	// We can use the PostFile class and _post_file_uploads vector for it 
	// (they are empty at this point)
	// or we can do something else
	std::string content_type_line = _headers.at("content-type");
	std::string content_type = content_type_line.substr(
		0, content_type_line.find(";"));
	std::string boundary = content_type_line.substr(content_type.length() + 2);
	boundary = boundary.substr(boundary.find("=") + 1);
	std::cout << "CONT-TYPE: " << content_type << std::endl;
	std::cout << "BOUNDARY: " << boundary << std::endl;
}

void	Request::respondPost() {
	// std::cout << "Request::respondPost()" << std::endl;
	// the POST body is in the file _post_body_filename
	auto content_type_iter = _headers.find("content-type");
	if (content_type_iter != _headers.end()) {
		if (content_type_iter->second.find(
			"multipart/form-data") != std::string::npos)
		{
			separateMultipart();
		} else {
			// TODO: not sure what is suposed to happen when its not a 
			// multipart AND not a CGI
		}
	}
	// TODO:create the actual response
}

void Request::getResponse(int status_code) {
	// std::cout << "GETRESPONSE" << std::endl;

	if (status_code == 408) {
		_status_code = status_code;
		handleError(status_code);
		_response.full_response = _response.header + _response.body;
		return ;
	}
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
	else if (endsWith(_path, ".bla")){
		handleCgi();
	}
	else if (_method == "GET")
		handleGet();
	else if (_method == "POST")
		respondPost(); // sorry handlePost() function name already exists
	else if (_method == "DELETE")
		handleDelete();
	_response.full_response = _response.header + _response.body;
	printRequest(); // Remove
}
