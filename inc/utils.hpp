#pragma once
#include "StandardLibraries.hpp"

std::string getHttpDate();
bool		endsWith(const std::string& str, const std::string& suffix);
void 		removeEndSlash(std::string &str);
std::string	generateTempFilename();
std::string fileToString(std::string filename);
