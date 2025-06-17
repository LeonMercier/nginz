#pragma once

#include "Structs.hpp"
#include "StandardLibraries.hpp"

/**********************************************************************
 * Returns an HTML string presenting directory listing for directory,
 * @param request_path = directory given as a parameter (without root
 * at this point)
**********************************************************************/
std::string generateAutoIndex(const std::string& request_path, ServerConfig config);


std::vector<ServerConfig> configParser(char *path_to_config);
