#pragma once

#include "StandardLibraries.hpp"
#include "Request.hpp"

typedef enum e_cgi_state {
	CGI_READY,
	CGI_WAITING
} t_cgi_state;

class CgiHandler {
public:
	int pid;
	void	launchCgi(Request &request); //fork(), execve()
	t_cgi_state	checkCgi(); // waitpid()
};
