#include "../inc/CgiHandler.hpp"

void	CgiHandler::launchCgi(Request &request)
{
	pid_t p = fork();

	if (p < 0){
		// fork has failed, we throw and quit the whole server gracefully

	}
	else if (p == 0){
		
		// temp_file execven outputille ja dup
		// save env variables for cgi
		// execve()
	}
	pid = p;// store pid
}

t_cgi_state	CgiHandler::checkCgi()
{
	// TIME_OUT
	//waitpid WNOHANG
	return CGI_READY;
}

char	**getCgiEnv(std::map<std::string, std::string> &headers, ServerConfig &config)
{
	char	**env;

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
		contentType += "text/plain"; // "text/html"?
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
	if (headers.find("path") != header.end()){
		scriptFileName += headers.at("path");
	}
	std::string	serverPort = "SERVER_PORT=" + std::to_string(config.listen_port);
	std::string serverName = "SERVER_NAME=" + config.server_names; // or should we just put IP here? we might have multiple names I'm confutse
	std::string remoteAddr = "REMOTE_ADDR=" + config.listen_ip; // "The IP address of remote host", should this really be us?
}

void CgiHandler::launchCgi(Request &request)
{
	std::map<std::string, std::string>	headers = request.getHeaders();

	// Creating temp input and output files for CGI (input for POST body, output for script's output)

	std::FILE* infile = std::tmpfile(); // this is actually done before, this just to demonstrate
	// Creating a tempfile with unique auto-generated filename:
	
	//std::rewind for rewinding input back to start
	if (!infile.is_open())
	{
		throw std::runtime_error("LaunchCgi: input file doesn't exist");
	}
	std::FILE* outfile = std::tmpfile();
	if (!outfile.is_open())
	{
		infile.close();
		throw std::runtime_error("LaunchCgi: open output file failure");
	}

	// we have saved the post body into infile before
	std::rewind(infile); // move file position indicator to the beginning of the file stream

	// SETUP ENVIRONMENT VARIABLES SOMEWHERE ELSE, THESE ARE JUST PLACEHOLDERS:

		char* envp[] = {
			(char*)"REQUEST_METHOD=" + 
			(char*)"SCRIPT_NAME=/cgi-bin/hello.py",
			(char*)"QUERY_STRING=",
			(char*)"CONTENT_LENGTH=0" + content_length,
			(char*)"CONTENT_TYPE=text/html",
			(char*)"GATEWAY_INTERFACE=CGI/1.1",
			(char*)"SERVER_PROTOCOL=HTTP/1.1",
			(char*)"SERVER_SOFTWARE=webserv/0.1",
			nullptr
		};

	pid_t p = fork();

	if (p < 0) {
		infile.close();
		outfile.close();
		throw std::runtime_error("fork() failed"); // are we closing the server?
	}
	else if (p == 0) {
	
		// redirecting input and output fds to STDIN and STDOUT for execve
		dup2(infile, STDIN_FILENO);
		dup2(infile, STDOUT_FILENO);

		infile.close();
		outfile.close();

		char* argv[] = {
			(char*)"./cgi-bin/script.py?name=Saara",
			nullptr
		};

		execve(argv[0], argv, envp);
		// throw something
	}
	else {
		// Parent

		// waitpid is not here, nor is reading from the outfile

		// Read from execves output file after waitpid and create response. 
		// Close (=delete) temp files there after waitpid or TIMEOUT
	}
}
