#include "../inc/parse_header.hpp"

void	encodeQuery(std::string &query)
{
	std::size_t pos = 0;
	pos = query.find_first_of("+");
// Separate handling for spaced as they might be encoded with '+' instead of '%...'
// right now only implemented for one space to test
	if (pos != std::string::npos)
	{
		std::cout << "IN HEADER PARSING, WE HAVE FOUND ENCODED SPACE" << std::endl;
		query.replace(pos, 1, " ");
	}
// for % implementation TOMORROW

}

std::map<std::string, std::string> parseHeader(std::string request) {
	std::map<std::string, std::string> result;
	std::istringstream iss(request);
	std::string temp, subA, subB, method, path, version;
	const std::string delimiter = ":";
	int pos;
	
	iss >> method >> path >> version;

	if (path.find('?') != std::string::npos) {
		std::string form_path = path.substr(0, path.find('?'));
		std::string query_string = path.substr(path.find('?') + 1);

		encodeQuery(query_string);

		result.insert(std::pair<std::string, std::string>("path", form_path));
		result.insert(std::pair<std::string, std::string>("query_string", query_string));
	}
	else
		result.insert(std::pair<std::string, std::string>("path", path));

	result.insert(std::pair<std::string, std::string>("method", method));
	result.insert(std::pair<std::string, std::string>("version", version));

	while (getline(iss, temp)) {
		if (temp.find(delimiter) != std::string::npos) {
			pos = temp.find(delimiter);

			subA = temp.substr(0, pos);
			std::transform(subA.begin(), subA.end(), subA.begin(),[](unsigned char c) {
				return std::tolower(c);
			});

			subB = temp.substr(pos + 1);
			subB = std::regex_replace(subB, std::regex("^ +| +$"), ""); //Trim Whitespace

			auto remove_value = remove(subB.begin(), subB.end(), '\r');
			subB.erase(remove_value, subB.end());

			auto return_value = result.insert(std::pair<std::string, std::string>(subA, subB));
			if (!return_value.second) {
				std::map<std::string, std::string> empty;
				return empty;
			}
		}
	}
	for (auto& p : result) {
		std::cout << ">>>" << p.first << " -- " << p.second << std::endl;
	}

	return result;
}
