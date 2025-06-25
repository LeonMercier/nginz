#!/usr/bin/python3

##############################################################################
# This script is for testing CGI with GET method that is launched from a HTML
# form with multiple fields. 
# Run for example like this: 
# QUERY_STRING="?firstname=Leon&lastname=Potato&favcolor=blue" ./who.py
##############################################################################

import os

raw_query = os.environ['QUERY_STRING']
query = raw_query.split('?')
# disregard everything before question mark
fields = query[1].split('&')
map = dict()
for field in fields:
    split_field = field.split('=')
    map[split_field[0]] = split_field[1]

body = '<!DOCTYPE html>\n'
body += '<html lang="en">\n'
body += '<head>\n'
body += '<meta charset="UTF-8" />\n'
body += '<title>Babys first CGI</title>\n'
body += '</head>\n'
body += '<body>\n'
body += 'Hello ' + map["firstname"] + ' ' + map["lastname"] + '\n'
body += 'Your favourite color is: ' + map["favcolor"] + '\n'
body += '</body>\n'
body += '</html>\n'

# print adds a newline at the end
print('HTTP/1.1 200 OK\r')
#print('Date: Wed, 25 Jun 2025 11:01:20 GMT\r')
print('Server: OverThirty_Webserv\r')
print('Content-Type: text/html; charset=UTF-8\r')
print('Content-Length: ' + str(len(body)) + '\r')
print('Cache-Control: no-cache, private\r')
print('\r')
print(body)
