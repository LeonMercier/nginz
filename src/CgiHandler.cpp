#include "../inc/CgiHandler.hpp"

void	CgiHandler::launchCgi(Request &request)
{
	// fork()
	// temp_file execven outputille ja dup
	// store pid
	// execve()
}

t_cgi_state	CgiHandler::checkCgi()
{
	// TIME_OUT
	//waitpid WNOHANG
	return CGI_READY;
}
