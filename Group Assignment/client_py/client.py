import os
import socket
import time

HOST = os.getenv("SERVER_HOST", "server_py")
PORT = int(os.getenv("SERVER_PORT", "5002"))

time.sleep(3)
with socket.create_connection((HOST, PORT), timeout=5) as s:
    s.sendall(b"GET\n")
    data = s.recv(1024)
    print("Reply:", data.decode(errors="ignore").strip())
