#pragma once

#include "request_handler.hpp"
#include "parse_header.hpp"
#include "StandardLibraries.hpp"
#include "Structs.hpp"

typedef enum {
	RECV_MORE,
	READY
} e_req_state;

class Request {
public:
	Request(std::vector<ServerConfig> configs);

	// This gets called from Client when we receive new data. It then returns:
	//
	// READY: we have received a complete request and the response struct is 
	// complete and accessible by getRes(). The Client will then send the
	// response and destroy the current instance of Request.
	//
	// RECV_MORE: we still need to receive more data before forming a response.
	// The Client will attempt to receive more data end the call this function 
	// again.
	e_req_state							addToRequest(std::string part);

	// sets config to the one from all_configs that matches
	// the request host field
	void								setConfig();

	// GET calls this directly
	void handleCompleteRequest(size_t body_start, size_t body_length, int status);

	// check if body is received and then call handleCompleteRequest()
	e_req_state handlePost(size_t body_start);

	bool headerIsComplete();

	Response							getRes();
	ServerConfig						getConfig();
	std::map<std::string, std::string>	getHeaders();

private:
	std::vector<ServerConfig>			all_configs;
	struct ServerConfig					config;
	std::string							raw_request;
	struct Response						response;
	std::map<std::string, std::string>	headers;
};
