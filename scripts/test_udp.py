import socket
import time

# Configuration
UDP_IP = "127.0.0.1"
UDP_PORT = 5000
MESSAGE = b"Hello from Python!"

print(f"UDP target IP: {UDP_IP}")
print(f"UDP target port: {UDP_PORT}")
print(f"message: {MESSAGE}")

# Set up the UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(2.0) # 2 seconds timeout for receiving a reply

try:
    # Send message
    sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
    print("Message sent.")

    # Wait for the echo reply
    print("Waiting for reply...")
    data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
    print(f"Received reply from {addr}: {data}")

except socket.timeout:
    print("Request timed out. No reply received.")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    sock.close()
