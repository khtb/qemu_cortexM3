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
# uint8_t  payload[1180]; // Padding
# uint32_t eof_marker;   // EOF Marker
# Total: 16 + 1180 + 4 = 1200 bytes
# '<' for little-endian, 'I' for uint32 (4 bytes), '1180s' for bytes
STRUCT_FORMAT = "<IIII1180sI"

CMD_ECHO = 0x1001
CMD_GET_3_FRAMES = 0x1002
EOF_MARKER_VAL = 0xE0F0E0F0

def create_packet(msg_id, cmd, data_str):
    header = 0xDEADBEEF
    payload_len = len(data_str)
    # Ensure data fits in 1180 bytes
    if payload_len > 1180:
        data_str = data_str[:1180]
        payload_len = 1180
    
    # Pad payload with null bytes
    payload = data_str.encode('utf-8') + b'\x00' * (1180 - payload_len)
    
    return struct.pack(STRUCT_FORMAT, header, msg_id, cmd, payload_len, payload, EOF_MARKER_VAL)

def parse_packet(data):
    if len(data) != 1200:
        return None
    
    header, msg_id, cmd, payload_len, payload, eof_marker = struct.unpack(STRUCT_FORMAT, data)
    payload_str = payload[:payload_len].decode('utf-8', errors='ignore')
    
    return {
        'header': hex(header),
        'msg_id': msg_id,
        'cmd': hex(cmd),
        'payload_len': payload_len,
        'payload': payload_str,
        'eof_marker': hex(eof_marker),
        'eof_valid': eof_marker == EOF_MARKER_VAL
    }

print(f"UDP target IP: {UDP_IP}")
print(f"UDP target port: {UDP_PORT}")

# Set up the UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(2.0) # 2 seconds timeout for receiving a reply

try:
    # Send message to get 3 frames
    msg_id = 1
    cmd = CMD_GET_3_FRAMES
    data_str = "Requesting 3 frames!"
    
    packet = create_packet(msg_id, cmd, data_str)
    
    sock.sendto(packet, (UDP_IP, UDP_PORT))
    print(f"Sent 1200-byte CommReq: msg_id={msg_id}, cmd={hex(cmd)}")

    print("Waiting for 3 responses...")
    
    for i in range(3):
        try:
            response_data, addr = sock.recvfrom(2048)
            parsed = parse_packet(response_data)
            
            print(f"\n--- Frame {i+1} ---")
            if parsed:
                print(f"  Received {len(response_data)} bytes from {addr}:")
                print(f"  Header:     {parsed['header']}")
                print(f"  Msg ID:     {parsed['msg_id']}")
                print(f"  Command:    {parsed['cmd']}")
                print(f"  Payload:    {parsed['payload']}")
                print(f"  EOF Marker: {parsed['eof_marker']} (Valid: {parsed['eof_valid']})")
                
                if not parsed['eof_valid']:
                    print("  [ERROR] EOF Marker mismatch!")
            else:
                print(f"  [ERROR] Received invalid packet size: {len(response_data)} bytes")
        except socket.timeout:
            print(f"  [ERROR] Timeout waiting for frame {i+1}")
            break

except Exception as e:
    print(f"An error occurred: {e}")
finally:
    sock.close()
