#include "../inc/CgiHandler.hpp"

t_cgi_state	CgiHandler::checkCgi()
{
	// TIME_OUT
	//waitpid WNOHANG
	if (waitpid(_pid, NULL, 0) > 0){
		std::cout << "\n\nCGI_READY, OUTPUT:\n" << std::endl;

		std::fstream fafa(output_filename);
		std::string str;
		while (getline(fafa, str))
		{
			std::cout << str << std::endl;
		}
		std::cout << "deleting file: " << output_filename << std::endl;
		std::remove(output_filename.c_str());

		return CGI_READY;
	}
	return (CGI_WAITING);
}

// make a getter to CGI's type (Python or PHP based on extension) {}

void	getCgiEnv(std::map<std::string, std::string> &headers, const ServerConfig &config, std::vector<char*> &envp, std::vector<std::string> &envVars)
{
	std::string contentLength = "CONTENT_LENGTH=";
	if (headers.find("content-length") != headers.end()){
		contentLength += headers.at("content-length");
	}
	else {
		contentLength += "0";
	}
	std::string contentType = "CONTENT_TYPE=";
	if (headers.find("content-type") != headers.end()){
		contentType += headers.at("content-type");
	}
	else {
		contentType += "text/plain"; // or "text/html"?
	}
	std::string query = "QUERY_STRING="; // check whether it's harmful to have this if no query
	if (headers.find("query_string") != headers.end()) // or whatever it will be saved as
	{
		query += headers.at("query_string");
	}
	std::string method = "REQUEST_METHOD=";
	if (headers.find("method") != headers.end()){
		method += headers.at("method");
	}
	std::string	scriptFileName = "SCRIPT_FILENAME=";
	std::string pathInfo = "PATH_INFO="; // this actually has to be there(?)
	if (headers.find("path") != headers.end()){
		scriptFileName += headers.at("path");
		pathInfo += headers.at("path");
	}
	std::string	serverPort = "SERVER_PORT=" + std::to_string(config.listen_port);
	std::string serverName = "SERVER_NAME=" + config.server_names[0]; // or should we just put IP here? we might have multiple names I'm confutse
	std::string remoteAddr = "REMOTE_ADDR=" + config.listen_ip; // "The IP address of remote host", should this really be us?
	std::string serverProtocol = "SERVER_PROTOCOL=HTTP/1.1"; // can it be hardcoded like this?
	std::string redirectStatus = "REDIRECT_STATUS=200"; // can it be hardcoded like this?
	std::string gatewayInterface = "GATEWAY_INTERFACE=CGI/1.1"; // can it be hardcoded like this?

	// ScriptName (pain)
	// std::string scriptName = "SCRIPT_NAME=" + root (how do we get it) + headers.at("path");


	envVars.push_back(contentLength);
	envVars.push_back(contentType);
	envVars.push_back(query);
	envVars.push_back(serverProtocol);
	envVars.push_back(pathInfo);
	envVars.push_back(method);
	envVars.push_back(scriptFileName); // check if right
	envVars.push_back(serverPort);
	envVars.push_back(serverName);
	envVars.push_back(remoteAddr);

	for (size_t i = 0; i < envVars.size(); i++){
		envp.push_back(const_cast<char*>(envVars[i].c_str()));
	}
	envp.push_back(nullptr);
}

void	CgiHandler::launchCgi(Request &request)
{
	std::map<std::string, std::string> headers = request.getHeaders();
	std::vector<char*> envp;
	std::vector<std::string> envVars;

	// Creating temp input and output file pointers for CGI (input for POST body, output for script's output)

	// std::string	input_filename = generateTempFilename();
	output_filename = generateTempFilename();

	// int	input_fd = open(input_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
	// if (input_fd < 0){
	// 	// throw
	// }

	int	output_fd = open(output_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644); // let's verify permissions later
	if (output_fd < 0){
		// close(input_fd);
		// throw
	}
	
	getCgiEnv(headers, request.getConfig(), envp, envVars);
	// we have saved the post body into infile before
	// close(input_fd); // move file position indicator to the beginning of the file stream by closing

	std::cout << "\n\n\n\nCGIHANDLER\n\n\n\n";
	int pid = fork();

	if (pid < 0) {
		// close(input_fd);
		close(output_fd);
		throw std::runtime_error("fork() failed"); // are we closing the server?
	}
	else if (pid == 0) {
	
		// redirecting input and output fds to STDIN and STDOUT for execve
		// dup2(input_fd, STDIN_FILENO);
		dup2(output_fd, STDOUT_FILENO);

		// close(input_fd);
		close(output_fd);

		char* argv[] = {
			(char*)"/usr/bin/python3" // absolute path to the interpreter (we get it from the location) // (char *)request.path.to_str();
			(char*)"./www/who.py", // hardcoded now // request.root + request.path // (char *)(request.root + request.path).to_str();
			nullptr
		};
		execve(argv[0], argv, envp.data());
		// throw something ?
	}
	else {
		_pid = pid;
		close(output_fd);
	}
}

