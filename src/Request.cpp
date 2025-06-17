#include "../inc/Request.hpp"

const static std::string header_terminator = "\r\n\r\n";

Request::Request(std::vector<ServerConfig> configs) : all_configs(configs) {}

e_req_state	Request::addToRequest(std::string part) {

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
	std::string whole_req = raw_request.substr(0, body_start + body_length);
	raw_request.erase(0, body_start + body_length);

	response = getResponse(whole_req, config, status);
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
