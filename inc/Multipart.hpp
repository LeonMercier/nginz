#pragma once

#include "StandardLibraries.hpp"
#include "Structs.hpp"
#include "utils.hpp"

typedef struct s_postfile {
	std::map<std::string, std::string> fileheaders;
	std::string	tmp_filename;
} t_postfile;

class Multipart {
public:
	Multipart();

	void	init(
		std::map<std::string, std::string> &headers,
		std::string &tmp_infile,
		std::string &root);
	void	readFiles();
	void	moveFiles();
	int		actual_files = 0;

private:
	std::string				_tmp_infile;
	std::string				_boundary;
	std::vector<t_postfile>	_files;
	std::string				_path;
};
