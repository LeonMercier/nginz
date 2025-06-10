#include "../inc/Webserv.hpp"

// directory iterator available since c++17 so that is in Makefile now. Will see later.
std::string generateAutoIndex(const std::string& request_path, ServerConfig config)
{
    (void)config;
    const std::string root = "./www";  // We hardcoding for now, later combining with serverConfig
    std::string clean_path = request_path;

    // Ensuring it starts with a slash
    if (clean_path.empty() || clean_path[0] != '/')
        clean_path = "/" + clean_path;

    std::filesystem::path full_path = root + clean_path;

    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html>\n<head><title>Index of " << request_path << "</title></head>\n";
    html << "<body>\n<h1>" << request_path << "</h1>\n<ul>\n";

    try // is this a good approach?
    {
        for (const auto& entry : std::filesystem::directory_iterator(full_path)) //
        {
            std::string name = entry.path().filename().string();
            if (std::filesystem::is_directory(entry))
                name += "/";

            html << "<li><a href=\"" << request_path;
            if (!request_path.empty() && request_path.back() != '/')
                html << '/';
            html << name << "\">" << name << "</a></li>\n";
        }
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        html << "<p>Error reading directory: " << e.what() << "</p>\n";
    }

    html << "</ul>\n</body>\n</html>\n";
    return html.str();
}
