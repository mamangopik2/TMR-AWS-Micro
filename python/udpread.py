import socket

# Set up the UDP server
UDP_IP = "0.0.0.0"  # Listen on all available network interfaces
UDP_PORT = 1234

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening for UDP packets on {UDP_IP}:{UDP_PORT}...")

while True:
    data, addr = sock.recvfrom(65535)  # Buffer size is 1024 bytes
    print(f"Received message from {addr}: {data.decode('utf-8')}")
