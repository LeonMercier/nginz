#include "../inc/Multipart.hpp"

Multipart::Multipart() {}

void	Multipart::init(
	std::map<std::string, std::string> &headers,
	std::string &tmp_infile,
	std::string &root)
{
	_tmp_infile = tmp_infile;
	_path = root + headers.at("path");
	if (_path.back() != '/') {
		_path += '/';
	}
	std::string content_type_line = headers.at("content-type");
	std::string content_type = content_type_line.substr(
		0, content_type_line.find(";"));
	std::string boundary = content_type_line.substr(content_type.length() + 2);
	_boundary = boundary.substr(boundary.find("=") + 1);

	readFiles();
	moveFiles();
}

void Multipart::readFiles() {
	std::ifstream infile(_tmp_infile, std::ios::binary);
	if (infile.fail()) {
		throw std::runtime_error("Multipart::readFiles(): failed to open file");
	}

	while (!infile.eof()) {
		// Read file headers
		std::map<std::string, std::string> file_headers;
		while (true) {
			std::array<char, 256> arr;
			infile.getline(&arr[0], 256, '\r');
			infile.ignore(1);
			std::string line(arr.begin(), arr.end());
			line.erase(line.find('\0'));
			if (line == "") {
				break;
			}
			if (line == "--" + _boundary) {
				continue ;
			}
			if (line == "\n") {
				continue ;
			}
			std::string key = line.substr(0, line.find(":"));
			key.erase(0, key.find_first_not_of('\n'));
			line.erase(0, line.find(":") + 2);
			file_headers[key] = line;
		}

		// Read file contents

		// For benchmarking:
		// time_t start = std::time(nullptr);
		char ch;
		std::string body;
		while (infile.get(ch)) {
			body += ch;
			if (body.length() >= _boundary.length()) {
				if (body.substr(body.length() - _boundary.length()) == _boundary) {
					break ;
				}
			}
			if (infile.eof()) {
				break;
			}
		}
		// time_t end = std::time(nullptr);
		// std::cout << "time taken: " << end - start << std::endl;

		// Write file contents to a new tmp file
		std::string outfile_name = generateTempFilename();
		std::ofstream outfile(outfile_name, std::ios::binary);
		outfile << body;
		outfile.close();
		_files.push_back({file_headers, outfile_name});
		if (infile.peek() == -1) {
			break;
		}
		infile.ignore(1);
	}
}

// Extract filename for file headers
static std::string getFilename(std::map<std::string, std::string> &hdr) {
	for (auto it : hdr) {
		if (it.first == "Content-Disposition") {
			std::stringstream ss(it.second);
			char delim = ';';
			std::string part;
			while (std::getline(ss, part, delim)) {
				part.erase(0, part.find_first_not_of(' '));
				if (part.substr(0, 8) == "filename" ) {
					part.erase(0, 10);
					part.erase(part.find_last_not_of('"') + 1);
					return part;
				}
			}
		}
	}
	return "";
}

// Move tmpfiles to their actual filenames
// If the tmpfile represents a form field, and not an actual file, then it
// does not a have a filename field and it just gets deleted and ignored. 
void	Multipart::moveFiles() {
	for (auto it : _files) {
		std::string filename = getFilename(it.fileheaders);
		if (filename == "" ) {
			std::filesystem::remove(it.tmp_filename);
			continue ;
		}
		// First, try to rename the tmpfile to the actual filename. That will 
		// if the two paths are on separate filesystems. In case of fail, we 
		// copy the file to the new location instead (that can fail too but 
		// will be caught elsewhere).
		try {
			std::filesystem::rename(it.tmp_filename, _path + filename);
		} catch (std::filesystem::filesystem_error &e) {
			std::cout << "Multipart::moveFiles(): rename failed, copying file" << std::endl;
			std::filesystem::copy(it.tmp_filename, _path + filename);
			std::filesystem::remove(it.tmp_filename);
		}
	}
}
