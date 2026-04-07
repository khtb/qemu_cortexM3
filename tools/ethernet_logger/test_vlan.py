import socket, time

def test_vlan_cli():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(2.0)
    try:
        print("Connecting to 127.0.0.1:2323...")
        s.connect(('127.0.0.1', 2323))
        print("Welcome:", s.recv(1024).decode(errors='ignore'))
        
        print("Sending: vlan")
        s.send(b'vlan\r\n')
        time.sleep(0.1)
        print(s.recv(1024).decode(errors='ignore'))

        print("Sending: vlan set tagged 10")
        s.send(b'vlan set tagged 10\r\n')
        time.sleep(0.1)
        print(s.recv(1024).decode(errors='ignore'))

        print("Sending: vlan set untagged 20")
        s.send(b'vlan set untagged 20\r\n')
        time.sleep(0.1)
        print(s.recv(1024).decode(errors='ignore'))

        print("Sending: vlan show")
        s.send(b'vlan show\r\n')
        time.sleep(0.2)
        print(s.recv(4096).decode(errors='ignore'))

        s.close()
        print('SUCCESS: All VLAN tests completed.')
    except Exception as e:
        print('ERROR:', e)

if __name__ == "__main__":
    test_vlan_cli()
