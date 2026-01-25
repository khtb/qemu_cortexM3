import socket
import sys
import time

UDP_IP = "127.0.0.1"
UDP_PORT = 12345

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.settimeout(10)

print(f"Listening on {UDP_IP}:{UDP_PORT}")

try:
    data, addr = sock.recvfrom(2048) # buffer size is 1024 bytes
    print(f"Received message: {len(data)} bytes from {addr}")
    # Hex dump first 32 bytes
    print("Data: " + data[:32].hex())
    # Check for EtherType 0x88B5 (Log)
    # Frame layout: [Dest:6][Src:6][Type:2][Payload...]
    # Dest matches? Src matches?
    # Type should be 88 B5
    if len(data) >= 14:
        etype = (data[12] << 8) | data[13]
        if etype == 0x88B5:
            print("SUCCESS: Log Packet Received")
            sys.exit(0)
        else:
            print(f"Received non-log packet type: {hex(etype)}")
except socket.timeout:
    print("TIMEOUT: No data received")
    sys.exit(1)
except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)
