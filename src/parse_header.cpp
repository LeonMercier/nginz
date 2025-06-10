#include "../inc/parse_header.hpp"

std::map<std::string, std::string> parseHeader(std::string request) {
	std::map<std::string, std::string> result;
	std::istringstream iss(request);
	std::string temp, subA, subB, method, path, version;
	const std::string delimiter = ":";
	int pos;
	
	iss >> method >> path >> version;
	result.insert(std::pair<std::string, std::string>("method", method));
	result.insert(std::pair<std::string, std::string>("path", path));
	result.insert(std::pair<std::string, std::string>("version", version));

	while (getline(iss, temp)) {
		if (temp.find(delimiter) != std::string::npos) {
			pos = temp.find(delimiter);
			subA = temp.substr(0, pos);
			subB = temp.substr(pos + 1);
			subB = std::regex_replace(subB, std::regex("^ +| +$"), ""); //Trim Whitespace
			
			auto return_value = result.insert(std::pair<std::string, std::string>(subA, subB));
			if (!return_value.second) {
				std::map<std::string, std::string> empty;
				return empty;
			}
		}
	}

	// for (auto& p : result) {
	// 	std::cout << ">>>" << p.first << " -- " << p.second << std::endl;
	// }

	return result;
}
