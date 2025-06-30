import pytest
import subprocess
import time

@pytest.fixture(scope="function", autouse=True)
def start_server():
	print("\n=== Starting server ===")
	proc = subprocess.Popen(["./webserv"])
	time.sleep(0.1)
	yield proc
	print("=== Stopping server ===")
	proc.kill()