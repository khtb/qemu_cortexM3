import socket
import struct
import time

# Configuration
UDP_IP = "127.0.0.1"
UDP_PORT = 5000

# C struct layout: 
# uint32_t header;       // Magic word
# uint32_t msg_id;       // Message sequence ID
# uint32_t cmd;          // Command ID
# uint32_t payload_len;  // Length of valid data
# uint8_t  payload[1184]; // Padding to exactly 1200 bytes
# Total: 16 + 1184 = 1200 bytes
# '<' for little-endian, 'I' for uint32 (4 bytes), '1184s' for bytes
STRUCT_FORMAT = "<IIII1184s"

def create_packet(msg_id, cmd, data_str):
    header = 0xDEADBEEF
    payload_len = len(data_str)
    # Ensure data fits in 1184 bytes
    if payload_len > 1184:
        data_str = data_str[:1184]
        payload_len = 1184
    
    # Pad payload with null bytes
    payload = data_str.encode('utf-8') + b'\x00' * (1184 - payload_len)
    
    return struct.pack(STRUCT_FORMAT, header, msg_id, cmd, payload_len, payload)

def parse_packet(data):
    if len(data) != 1200:
        return None
    
    header, msg_id, cmd, payload_len, payload = struct.unpack(STRUCT_FORMAT, data)
    payload_str = payload[:payload_len].decode('utf-8', errors='ignore')
    
    return {
        'header': hex(header),
        'msg_id': msg_id,
        'cmd': hex(cmd),
        'payload_len': payload_len,
        'payload': payload_str
    }

print(f"UDP target IP: {UDP_IP}")
print(f"UDP target port: {UDP_PORT}")

# Set up the UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(2.0) # 2 seconds timeout for receiving a reply

try:
    # Send message
    msg_id = 1
    cmd = 0x1001 # Dummy command
    data_str = "Hello Firmware! This is Python."
    
    packet = create_packet(msg_id, cmd, data_str)
    
    sock.sendto(packet, (UDP_IP, UDP_PORT))
    print(f"Sent 1200-byte CommReq: msg_id={msg_id}, cmd={hex(cmd)}")

    # Wait for the echo reply
    print("Waiting for response...")
    response_data, addr = sock.recvfrom(2048)
    
    parsed = parse_packet(response_data)
    if parsed:
        print(f"Received {len(response_data)} bytes from {addr}:")
        print(f"  Header:  {parsed['header']}")
        print(f"  Msg ID:  {parsed['msg_id']}")
        print(f"  Command: {parsed['cmd']}")
        print(f"  Payload: {parsed['payload']}")
    else:
        print(f"Received invalid packet size: {len(response_data)} bytes from {addr}")

except socket.timeout:
    print("Request timed out. No reply received.")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    sock.close()
