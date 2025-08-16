# webserv
A small HTTP/1.1 server written in C++17

This was a group project with [JiggyStardust](https://github.com/JiggyStardust), [Ethan Berkowitz](https://github.com/ethan-berkowitz) and myself. 

## Features
* Supported methods: GET, POST, DELETE
* Chunked transfer encoding for incoming POST bodies
* Multipart form-data for file uploads
* Run CGI scripts in Python
* Configurable redirections
* Configurable error pages
* Can show directory listing
* Configure different directories with:
    * Allowed methods
    * Redirections
    * Directory listing on/off
    * Root folder
    * Default index file

## How to run
Clone the repo and run `make`. 
Then run `./webserv`. 
See `configuration` folder for the example configuration and `www` folder for an example website. 
