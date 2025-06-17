#pragma once

#include "Structs.hpp"
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>

struct Response {
	int			status_code = 200;
	std::string status_code_str = "OK";

	std::string method = "";
	std::string path = "";
	std::string version = "";

	std::string root = "";

	std::string body = "";
	std::string header = "";
	std::string full_response = "";
};

Response getResponse(std::string request, ServerConfig config, int status_code);
int getPostContentLength (std::string request);

const std::map<std::string, std::string> extensions {
	{".gif", "image/gif"},
	{".html", "text/html"},
	{".jpeg", "image/jpeg"},
	{".jpg", "image/jpeg"},
	{".txt", "text/html"},
	{".png", "image/png"}
};

const std::map<int, std::string> errorCodes {
	{400, "Bad Request"},				//Malformed request syntax
	{403, "Forbidden"}, 				//Understood the request but refused to fulfill it
	{404, "Not Found"}, 				//Request was not found either temporarily or permantly
	{405, "Method Not Allowed"}, 		//Method is known but not supported
	{408, "Client Timeout"}, 			//Did not receive the request in the time allowed to wait
	{411, "Length Required"}, 			//Missing content length for a body in a request
	{413, "Content Too Large"}, 		//Request content is too large. Terminate request or close connection
	{414, "URI Too Long"}, 				//It's just tooooooo long
	{500, "Internal Server Error"}, 	//Generic response when the server itself has an error
	{501, "Not Implemented"}, 			//Server does not support functionality. Includes method?
	{503, "Service Unavailable"}, 		//Server down for too much traffic or maintenance
	{505, "HTTP Version Not Supported"}	//Exactly what it sounds like
};

const std::map<int, std::string> errorHttps {
    {400, "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>400 Bad Request</title>\r\n</head>\r\n<body>\r\n<h1>400 Bad Request</h1>\r\n<p>Request body could not be read properly.</p>\r\n</body>\r\n</html>\r\n"},
    {403, "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>403 Forbidden</title>\r\n</head>\r\n<body>\r\n<h1>403 Forbidden</h1>\r\n<p>You don't have permission to access the resource.</p>\r\n</body>\r\n</html>\r\n"},
    {404, "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>404 Not Found</title>\r\n</head>\r\n<body>\r\n<h1>404 Page Not Found</h1>\r\n<p>Sorry, the resource you requested could not be found.</p>\r\n</body>\r\n</html>\r\n"},
    {405, "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>405 Method Not Allowed</title>\r\n</head>\r\n<body>\r\n<h1>405 Method Not Allowed</h1>\r\n<p>The requested method is not suported.</p>\r\n</body>\r\n</html>\r\n"},
    {408, "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>408 Request Timeout</title>\r\n</head>\r\n<body>\r\n<h1>408 Request Timeout</h1>\r\n<p>Failed to process request in time. Please try again.</p>\r\n</body>\r\n</html>\r\n"},
    {411, "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>411 Length Required</title>\r\n</head>\r\n<body>\r\n<h1>411 Length Required</h1>\r\n<p>Requests must have a content length header.</p>\r\n</body>\r\n</html>\r\n"},
    {413, "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>413 Payload Too Large</title>\r\n</head>\r\n<body>\r\n<h1>413 Payload Too Large</h1>\r\n<p>The request is larger than the server is willing or able to process.</p>\r\n</body>\r\n</html>\r\n"},
    {414, "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>414 URI Too Long</title>\r\n</head>\r\n<body>\r\n<h1>414 URI Too Long</h1>\r\n<p>The URI provided was too long for the server to process.</p>\r\n</body>\r\n</html>\r\n"},
    {500, "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>500 Internal Server Error</title>\r\n</head>\r\n<body>\r\n<h1>500 Internal Server Error</h1>\r\n<p>The server was unable to complete your request. Please try again later.</p>\r\n</body>\r\n</html>\r\n"},
    {501, "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>501 Not Implemented</title>\r\n</head>\r\n<body>\r\n<h1>501 Not Implemented</h1>\r\n<p>The server either does not recognize the request method, or it lacks the ability to fulfil the request.</p>\r\n</body>\r\n</html>\r\n"},
    {503, "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>503 Service Unavailable</title>\r\n</head>\r\n<body>\r\n<h1>503 Service Unavailable</h1>\r\n<p>The server cannot handle the request because it is overloaded or down for maintenance.</p>\r\n</body>\r\n</html>\r\n"},
    {505, "<!DOCTYPE html>\r\n<html lang=\"en\">\r\n<head>\r\n<title>505 HTTP Version Not Supported</title>\r\n</head>\r\n<body>\r\n<h1>505 HTTP Version Not Supported</h1>\r\n<p>The server does not support the HTTP version used in the request.</p>\r\n</body>\r\n</html>\r\n"}
};

