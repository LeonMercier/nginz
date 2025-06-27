#pragma once

#include "StandardLibraries.hpp"
#include "Request.hpp"
#include "utils.hpp"

typedef enum e_cgi_state {
	CGI_READY,
	CGI_WAITING
} t_cgi_state;

class CgiHandler {
	private:
		int _pid;
	public:
		std::string output_filename;
		void	launchCgi(Request &request); //fork(), execve()
		t_cgi_state	checkCgi(); // waitpid()
};
