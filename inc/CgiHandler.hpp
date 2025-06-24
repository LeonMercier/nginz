#pragma once

#include "StandardLibraries.hpp"

class CgiHandler {
public:
	int pid;
	void	launchCgi();
	void	checkCgi();
};