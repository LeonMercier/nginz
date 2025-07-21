"""

How to run the test!
	Go to the root of the webserv project
	Run Normal:
		pytest -v
	Run with prints from python and webserv
		pytest -v -s

"""

import pytest
import requests
import http.client
import socket
import time



def test_200_get_root():
	response = requests.get("http://127.0.0.1:8080/")
	assert response.status_code == 200

def test_200_get_cgi():
	response = requests.get("http://127.0.0.1:8080/cgi/who.py?firstname=first&lastname=last&favcolor=color")
	assert response.status_code == 200

def test_301_redirect_when_missing_trailing_slash():
	response = requests.get("http://127.0.0.1:8080/images", allow_redirects=False)
	assert response.status_code == 301
	response = requests.get("http://127.0.0.1:8080/images")
	assert response.status_code == 200
	assert response.url == "http://127.0.0.1:8080/images/"

def test_301_redirect_from_config_return():
	response = requests.get("http://127.0.0.1:8080/puzzle/", allow_redirects=False)
	assert response.status_code == 301
	response = requests.get("http://127.0.0.1:8080/puzzle/")
	assert response.status_code == 200
	assert response.url == "https://xkcd.com/blue_eyes.html"

def test_400_bad_request():
	s = socket.socket()
	s.connect(('127.0.0.1', 8080))

	req = "\r\n\r\n"

	s.send(req.encode())
	response = s.recv(1024)
	response_text = response.decode('utf-8', errors='ignore')
	full_status_line = response_text.split('\r\n')[0]
	print("Status Line:", full_status_line)
	split_status_line = full_status_line.split()
	status_code = split_status_line[1]
	print("Status:", split_status_line[1])
	assert status_code == '400'

def test_403_forbidden_with_two_dots_1():
	s = socket.socket()
	s.connect(('127.0.0.1', 8080))

	req = "GET /../helloworld.txt HTTP/1.1\r\n\r\n"

	s.send(req.encode())
	response = s.recv(1024)
	response_text = response.decode('utf-8', errors='ignore')
	full_status_line = response_text.split('\r\n')[0]
	print("Status Line:", full_status_line)
	split_status_line = full_status_line.split()
	status_code = split_status_line[1]
	print("Status:", split_status_line[1])
	assert status_code == '403'

def test_403_forbidden_with_two_dots_2():
	s = socket.socket()
	s.connect(('127.0.0.1', 8080))

	req = "GET /.. HTTP/1.1\r\n\r\n"

	s.send(req.encode())
	response = s.recv(1024)
	response_text = response.decode('utf-8', errors='ignore')
	full_status_line = response_text.split('\r\n')[0]
	print("Status Line:", full_status_line)
	split_status_line = full_status_line.split()
	status_code = split_status_line[1]
	print("Status:", split_status_line[1])
	assert status_code == '403'

def test_404_page_not_found():
	response = requests.get("http://127.0.0.1:8080/non-existant-page")
	assert response.status_code == 404

def test_405_method_not_allowed():
	response = requests.put("http://127.0.0.1:8080/")
	assert response.status_code == 405

def test_408_server_timeout_on_incomplete_request():
	s = socket.socket()
	s.connect(('127.0.0.1', 8080))

	req = "GET\r\n"

	s.send(req.encode())
	response = s.recv(1024)
	response_text = response.decode('utf-8', errors='ignore')
	full_status_line = response_text.split('\r\n')[0]
	print("Status Line:", full_status_line)
	split_status_line = full_status_line.split()
	status_code = split_status_line[1]
	print("Status:", split_status_line[1])
	assert status_code == '408'

def test_411_content_length_missing():
	conn = http.client.HTTPConnection("127.0.0.1", 8080)
	conn.putrequest("POST", "/submit/", "HTTP/1.1")
	conn.putheader("Host", "127.0.0.1")
	conn.putheader("User-Agent", "test-client")
	conn.putheader("Connection", "keep-alive")
	conn.endheaders()
	response = conn.getresponse()
	assert response.status == 411

def test_414_uri_too_long():
	temp_str = "x" * 99999999999
	response = requests.get("http://127.0.0.1:8080/" + temp_str)
	assert response.status_code == 414

def test_500_cgi_with_crash():
	response = requests.get("http://127.0.0.1:8080/cgi/crash.py?firstname=first&lastname=last&favcolor=color")
	assert response.status_code == 500

def test_504_cgi_with_infinite_sleep():
	response = requests.get("http://127.0.0.1:8080/cgi/infinite.py?firstname=first&lastname=last&favcolor=color")
	assert response.status_code == 504

def test_505_version_not_supported():
	s = socket.socket()
	s.connect(('127.0.0.1', 8080))

	req = "GET / HTTP/1.2\r\n\r\n"

	s.send(req.encode())
	response = s.recv(1024)
	response_text = response.decode('utf-8', errors='ignore')
	full_status_line = response_text.split('\r\n')[0]
	print("Status Line:", full_status_line)
	split_status_line = full_status_line.split()
	status_code = split_status_line[1]
	print("Status:", split_status_line[1])
	assert status_code == '505'

def test_OTHER_double_dots_that_are_not_forbidden():
	s = socket.socket()
	s.connect(('127.0.0.1', 8080))

	req = "GET /.../helloworld.txt HTTP/1.1\r\n\r\n"

	s.send(req.encode())
	response = s.recv(1024)
	response_text = response.decode('utf-8', errors='ignore')
	full_status_line = response_text.split('\r\n')[0]
	print("Status Line:", full_status_line)
	split_status_line = full_status_line.split()
	status_code = split_status_line[1]
	print("Status:", split_status_line[1])
	assert status_code == '404'

#TODO Test a cgi using a script that sleeps, then check other connection
#TODO Test that sends good chunks
#TODO Test that sends BAD chunks
