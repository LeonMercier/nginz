#include "../inc/request_handler.hpp"

static std::string getHttpDate() {
    std::time_t now = std::time(nullptr);
    std::tm *gmt = std::gmtime(&now);

    std::ostringstream date_stream;
    date_stream << std::put_time(gmt, "%a, %d %b %Y %H:%M:%S GMT");
    return date_stream.str();
}

static void createHeader(Response *response, std::string content_type) {
	response->header = "HTTP/1.1 " + response->status_code_str +"\r\n"
	"Date: " + getHttpDate() + "\r\n"
	"Server: OverThirty_Webserv\r\n"
	"Content-Type: " + content_type + "\r\n"
	"Connection: keep-alive" + "\r\n"
	"Content-Length: " + std::to_string(response->body.length()) + "\r\n"
	"Cache-Control: no-cache, private\r\n"
	+ "\r\n";
}

static void createBody(Response *response, std::string filename) {
	std::ifstream file(filename, std::ios::binary);
	std::ostringstream sbody;
	sbody << file.rdbuf();
	response->body = sbody.str();
}

static void setStatusCode(Response *response, int status_code, std::string status_code_str) {
	response->status_code = status_code;
	response->status_code_str = status_code_str;
}

bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

Response getResponse(std::string request) {
	Response response;

	std::istringstream iss(request);
	iss >> response.method >> response.path >> response.version;

	if (response.method == "GET") {
		if (response.path == "/") {
			setStatusCode(&response, 200, "200 OK");
			createBody(&response, "src/index.html");
			createHeader(&response, "text/html; charset=UTF-8");
		}
		else if (ends_with(response.path, ".png")){
			setStatusCode(&response, 200, "200 OK");
			createBody(&response, "src/potato_chip.png");
			createHeader(&response, "image/png");
		}
	}

	response.full_response = response.header + response.body;
	return response;
}
