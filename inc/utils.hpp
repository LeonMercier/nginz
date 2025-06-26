#pragma once
#include "../inc/StandardLibraries.hpp"

std::string getHttpDate();
bool		endsWith(const std::string& str, const std::string& suffix);
void removeEndSlash(std::string &str);