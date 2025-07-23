#include "../inc/parse_header.hpp"

void	hexadecimalToAscii(std::string &hex)
{
	std::stringstream ss;
	char c = stoi(hex, nullptr, 16);
	ss << c;
	hex = ss.str();
}

void	encodeQuery(std::string &query)
{
	std::cout << "\n\nQuery before encoding: " << query << std::endl;

	std::size_t pos = 0;

	// Separate handling for spaced as in query they are encoded with '+' instead of '%...'
	pos = query.find_first_of("+");
	while (pos != std::string::npos)
	{
		query.replace(pos, 1, " ");
		pos = query.find_first_of("+");
	}

	pos = 0;
	while ((pos = query.find_first_of("%", pos)) != std::string::npos)
	{
		if (pos + 2 >= query.length())
		{
			break ;
		}
		std::string temp = query.substr(pos + 1, 2);
		if (temp != "26"){
			hexadecimalToAscii(temp);
			query.replace(pos, 3, temp);
		}
		else
		{
			pos += 3;
		}
	}
	std::cout << "\n\nQuery after encoding: " << query << std::endl;
}

void	encodePath(std::string &path)
{
	std::size_t pos = 0;
	while ((pos = path.find_first_of("%", pos)) != std::string::npos)
	{
		if (pos + 2 >= path.length())
		{
			break ;
		}
		std::string temp = path.substr(pos + 1, 2);
		hexadecimalToAscii(temp);
		path.replace(pos, 3, temp);
	}
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
		
		encodePath(form_path);
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
