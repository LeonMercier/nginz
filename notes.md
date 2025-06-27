
-----------
21/5/25
-----------

There can and will me multiple configuration files for different server setups, so the naming might still change. However config files end with .conf extension.

The configuration file needs to be parsed. Regex might be a helpful/fast method for this.

We launch the program: ./executable "configuration_file", so for example ./webser webserv.conf


Not sure if we should follow c++11 (older), c++20 (newer) or some else standar, have to study the differences a bit.

-----------
23/5/2025
-----------

If we want to add cookies, config file needs session defined (on). For now, not included.


-----------
12/6/2025
-----------
* In HTTP 1.1 Connection: Keep-Alive is the default and Close is used if requested
* Response is sent also in case of POST and DELETE (200 series codes)

-----------
26/6/2025
-----------
* More than one cgi extension type is bonus! we can handle ONLY python for example
