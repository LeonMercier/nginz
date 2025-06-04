#include "../inc/request_handler.hpp"
#include "../inc/Webserv.hpp"

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

static void setStatusCode(Response *response, int status_code, std::string status_code_str) {
	response->status_code = status_code;
	response->status_code_str = status_code_str;
}

static std::string getHttpDate() {
    std::time_t now = std::time(nullptr);
    std::tm *gmt = std::gmtime(&now);

    std::ostringstream date_stream;
    date_stream << std::put_time(gmt, "%a, %d %b %Y %H:%M:%S GMT");
    return date_stream.str();
}

static void createHeader(Response *response, std::string content_type) {
	response->header = "HTTP/1.1 " + std::to_string(response->status_code) + " " + response->status_code_str +"\r\n"
	"Date: " + getHttpDate() + "\r\n"
	"Server: OverThirty_Webserv\r\n"
	"Content-Type: " + content_type + "\r\n"
	"Content-Length: " + std::to_string(response->body.length()) + "\r\n"
	"Cache-Control: no-cache, private\r\n"
	+ "\r\n";
}

static void createBody(Response *response, std::string filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		std::ifstream file("errors/404.html", std::ios::binary);
		//Throw exception with custom exception class if cannot open error
		setStatusCode(response, 404, "File not found");
	}
	std::ostringstream sbody;
	sbody << file.rdbuf();
	response->body = sbody.str();
}

bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

Response getResponse(std::string request, ServerConfig config) {
	Response response;

	std::istringstream iss(request);
	std::string connection;
	std::string d;
	iss >> response.method >> response.path >> response.version >> d >> d >> d >> connection;
	if (connection == "close")
		response.connection_is_close = true;

	//std::cout << request << std::endl;

	std::string root = "./www"; //temporary instead of config

	if (response.method == "GET") {
		if (response.path == "/") {
			setStatusCode(&response, 200, "OK");
			createBody(&response, root + "/index.html");
			createHeader(&response, "text/html; charset=UTF-8");
			response.full_response = response.header + response.body;
			return response;
		}
		else {
			createBody(&response, root + response.path);
		}
		if (ends_with(response.path, ".png")){
			
			createHeader(&response, "image/png");
		}
		else if (ends_with(response.path, ".jpg")){
			createHeader(&response, "image/jpeg");
		}
	}
	// response.body = generateAutoIndex("images/");
	// response.header = "HTTP/1.1 200 OK\r\n"
	// "Content-Type: text/html; charset=UTF-8\r\n"
	// "Content-Length: " + std::to_string(response.body.length()) + "\r\n"
	// "Connection: close\r\n"
	// + "\r\n";
	response.full_response = response.header + response.body;
	return response;
}
