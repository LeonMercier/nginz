#include "../inc/utils.hpp"

std::string getHttpDate() {
    std::time_t now = std::time(nullptr);
    std::tm *gmt = std::gmtime(&now);

    std::ostringstream date_stream;
    date_stream << std::put_time(gmt, "%a, %d %b %Y %H:%M:%S GMT");
    return date_stream.str();
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void removeEndSlash(std::string &str) {
	if (str.length() > 1 && str.back() == '/') {
		str.pop_back();
	}
}

static bool fileExists(std::string filename) {
	std::ifstream file(filename.c_str());
	return file.good();
}

std::string	generateTempFilename()
{
	std::string prefix = "/tmp/webserv_file_";
	static int counter = 0;
	
	while (fileExists(prefix + std::to_string(counter))) {
		counter++;
	}
	return (prefix + std::to_string(counter++));
}

std::string fileToString(std::string filename) {
	std::ifstream file(filename, std::ios::binary);
	if (!file.is_open()) {
		throw std::ios_base::failure("Failed to open file: " + filename);
    }
	std::ostringstream sbody;
	sbody << file.rdbuf();
	file.close();
	return sbody.str();
}
