#include "../inc/CgiHandler.hpp"

// void	CgiHandler::launchCgi(Request &request)
// {
// 	pid_t p = fork();

// 	if (p < 0){
// 		// fork has failed, we throw and quit the whole server gracefully

// 	}
// 	else if (p == 0){
		
// 		// temp_file execven outputille ja dup
// 		// save env variables for cgi
// 		// execve()
// 	}
// 	pid = p;// store pid
// }

t_cgi_state	CgiHandler::checkCgi()
{
	// TIME_OUT
	//waitpid WNOHANG
	return CGI_READY;
}

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
	if (headers.find("query") != headers.end()) // or whatever it will be saved as
	{
		query += headers.at("query");
	}
	std::string method = "REQUEST_METHOD=";
	if (headers.find("method") != headers.end()){
		method += headers.at("method");
	}
	std::string	scriptFileName = "SCRIPT_FILENAME="; // this actually has to be there(?)
	if (headers.find("path") != headers.end()){
		scriptFileName += headers.at("path");
	}
	std::string	serverPort = "SERVER_PORT=" + std::to_string(config.listen_port);
	std::string serverName = "SERVER_NAME=" + config.server_names[0]; // or should we just put IP here? we might have multiple names I'm confutse
	std::string remoteAddr = "REMOTE_ADDR=" + config.listen_ip; // "The IP address of remote host", should this really be us?
	
	envVars.push_back(contentLength);
	envVars.push_back(contentType);
	envVars.push_back("QUERY_STRING=/who.py?firstname=HAHAA&lastname=&favcolor=");
	envVars.push_back(method);
	envVars.push_back(scriptFileName);
	envVars.push_back(serverPort);
	envVars.push_back(serverName);
	envVars.push_back(remoteAddr);

	// should have pre or post incr+ in for loop?
	for (size_t i = 0; i < envVars.size(); i++){
		envp.push_back(const_cast<char*>(envVars[i].c_str()));
	}
	envp.push_back(nullptr);
}

std::string	generateTempFilename()
{
	std::string prefix = "/tmp/cgi_tmp_";
	static int counter = 0;
	return (prefix + std::to_string(counter++));
}

void	CgiHandler::launchCgi(Request &request)
{
	std::map<std::string, std::string>	headers = request.getHeaders();
	std::vector<char*> envp;
	std::vector<std::string> envVars;

	// Creating temp input and output file pointers for CGI (input for POST body, output for script's output)

	std::string	input_filename = generateTempFilename();
	std::string output_filename = generateTempFilename();

	int	input_fd = open(input_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0600);
	if (input_fd < 0){
		// throw
	}

	int	output_fd = open(output_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644); // let's verify permissions later
	if (output_fd < 0){
		close(input_fd);
		// throw
	}
	
	getCgiEnv(headers, request.getConfig(), envp, envVars);

	for (size_t i = 0; i < envp.size(); ++i)
		std::cout << "env[" << i << "]: " << envp[i] << "\n";
	// we have saved the post body into infile before
	close(input_fd); // move file position indicator to the beginning of the file stream

	pid_t p = fork();

	if (p < 0) {
		close(input_fd);
		close(output_fd);
		throw std::runtime_error("fork() failed"); // are we closing the server?
	}
	else if (p == 0) {
	
		// redirecting input and output fds to STDIN and STDOUT for execve
		dup2(input_fd, STDIN_FILENO);
		dup2(output_fd, STDOUT_FILENO);

		close(input_fd);
		close(output_fd);

		char* argv[] = {
			(char*)"./www/who.py",
			nullptr
		};
		// char *aa = "QUERY_STRING=/who.py?firstname=HAHAA&lastname=&favcolor=";
		// char *bb = 0;
		// char **asia;
		// asia[0] = aa ;
		// asia[1] = bb;
		execve(argv[0], argv, envp.data());
		// throw something ?
	}
	else {
		// Parent
		waitpid(p, NULL, 0);
		close(output_fd);
		std::fstream fafa(output_filename);
		//int fd = open(output_filename);
		std::string str;
		getline(fafa, str);
		std::cout << "FIRST LINE OF CGI OUTPUT:" << str << std::endl;
		// waitpid is not here, nor is reading from the outfile
		// we save the pid somewhere. Where?

		// Read from execves output file after waitpid and create response. Close and delete the files.
	}
}

