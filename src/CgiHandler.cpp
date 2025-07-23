#include "../inc/CgiHandler.hpp"
#include "../inc/Signal.hpp"
#include <stdexcept>

t_cgi_state	CgiHandler::checkCgi()
{
	int	status;

	if (_state == CGI_FAILED)
	{
		return (CGI_FAILED);
	}
	if (std::time(nullptr) - _start_time >= _CGI_TIMEOUT_S){
		std::cout << "KILL THE CHILDREN\n";
		kill(_pid, SIGKILL);
		return CGI_TIMEOUT;
	}
	if (waitpid(_pid, &status, WNOHANG) > 0){
		if (WIFEXITED(status)){
			if (WEXITSTATUS(status) != 0){
				return CGI_FAILED;
			}
		}
		std::cout << "CGI script exited succesfully" << std::endl;
		// std::cout << "\n\nCGI_READY, OUTPUT:\n" << std::endl;
		//
		// std::fstream fafa(output_filename);
		// std::string str;
		// while (getline(fafa, str))
		// {
		// 	std::cout << str << std::endl;
		// }

		return CGI_READY;
	}
	return (CGI_WAITING);
}

// make a getter to CGI's type (Python) {}

void	getCgiEnv(Request &request, const ServerConfig &config, std::vector<char*> &envp, std::vector<std::string> &envVars)
{
	std::map<std::string, std::string> headers = request.getHeaders();

	std::string contentLength = "CONTENT_LENGTH=";
	std::string contentType = "CONTENT_TYPE=";
	std::string query = "QUERY_STRING=";
	std::string method = "REQUEST_METHOD=";
	std::string	scriptFileName = "SCRIPT_FILENAME=";
	std::string pathInfo = "PATH_INFO=";
	std::string scriptName = "SCRIPT_NAME=" + request.getLocation().root;
	std::string	serverPort = "SERVER_PORT=" + std::to_string(config.listen_port);
	std::string serverName = "SERVER_NAME=" + config.server_names[0]; // or should we just put IP here? we might have multiple names I'm confutse
	std::string remoteAddr = "REMOTE_ADDR=" + config.listen_ip; // "The IP address of remote host", should this really be us?
	std::string serverProtocol = "SERVER_PROTOCOL=HTTP/1.1";
	std::string redirectStatus = "REDIRECT_STATUS=200";
	std::string gatewayInterface = "GATEWAY_INTERFACE=CGI/1.1";

	if (headers.find("content-length") != headers.end()){
		contentLength += headers.at("content-length");
	}
	else {
		contentLength += "0";
	}

	if (headers.find("content-type") != headers.end()){
		contentType += headers.at("content-type");
	}
	else {
		contentType += "text/plain"; // or "text/html"?
	}

	if (headers.find("query_string") != headers.end()){
		query += headers.at("query_string");
	}

	if (headers.find("method") != headers.end()){
		method += headers.at("method");
	}

	if (headers.find("path") != headers.end()){
		scriptFileName += headers.at("path");
		pathInfo += headers.at("path");
		scriptName += headers.at("path"); 
	}

	envVars.push_back(contentLength);
	envVars.push_back(contentType);
	envVars.push_back(query);
	envVars.push_back(method);
	envVars.push_back(scriptFileName);
	envVars.push_back(pathInfo);
	envVars.push_back(scriptName);
	envVars.push_back(serverPort);
	envVars.push_back(serverName);
	envVars.push_back(remoteAddr);
	envVars.push_back(serverProtocol);
	envVars.push_back(redirectStatus);
	envVars.push_back(gatewayInterface);

	for (size_t i = 0; i < envVars.size(); i++){
		envp.push_back(const_cast<char*>(envVars[i].c_str()));
	}
	envp.push_back(nullptr);
}

void	CgiHandler::launchCgi(Request &request)
{
	std::vector<char*> envp;
	std::vector<std::string> envVars;
		
	// Creating temp input and output file pointers for CGI (input for POST body, output for script's output)
	output_filename = generateTempFilename();

	int	output_fd = open(output_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644); // let's verify permissions later
	if (output_fd < 0){
		_state = CGI_FAILED;
		throw std::runtime_error("Failed to open file in launchCgi");
	}
	getCgiEnv(request, request.getConfig(), envp, envVars);

	std::string	executable = request.getLocation().cgi_path_py;
	std::string	script = request.getLocation().root + request.getPath();

	// std::cout << "\n\nCGI_PATH: " << executable << "\n\nrequest.path: " << request.getPath() << "\n\nSCRIPT_PATH: " << script << std::endl;

	char *argv[] = {
		(char*)executable.c_str(),
		(char*)script.c_str(),
		nullptr
	};

	// std::cout << "\n\n\n\nCGIHANDLER\n\n\n\n";
	int pid = fork();

	if (pid < 0) {
		// close(input_fd);
		close(output_fd);
		throw std::runtime_error("fork() failed");
	}
	else if (pid == 0) {
		int input_fd = open(request.getPostBodyFilename().c_str(), O_RDONLY, 0644);
		if (input_fd >= 0){
			std::cerr << "WE HAVE A FILE\n";
			dup2(input_fd, STDIN_FILENO);
			close(input_fd);
		}
		dup2(output_fd, STDOUT_FILENO);
		close(output_fd);
		execve(argv[0], argv, envp.data());
		_state = CGI_FAILED;
		throw std::runtime_error("CGI: execve failed");
	}
	else {
		_start_time = std::time(nullptr);
		_pid = pid;
		close(output_fd);
	}
}

