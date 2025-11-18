#!/usr/bin/env python3
"""
Simple UDP tester for Bootstrap server.
Sends RUN_NODE_REQUEST (0x7b,nodeId) and STOP_NODE_REQUEST (0x7d,nodeId)
and prints binary responses in hex.
"""
import socket
import sys
import time

HOST = '127.0.0.1'
PORT = 8080
NODE_ID = 1
TIMEOUT = 2.0

def send_and_recv(msg_bytes):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(TIMEOUT)
    try:
        s.sendto(bytes(msg_bytes), (HOST, PORT))
        data, addr = s.recvfrom(1024)
        return data
    except socket.timeout:
        return None
    finally:
        s.close()

if __name__ == '__main__':
    if len(sys.argv) >= 2:
        HOST = sys.argv[1]
    if len(sys.argv) >= 3:
        PORT = int(sys.argv[2])
    if len(sys.argv) >= 4:
        NODE_ID = int(sys.argv[3])

    print(f"Testing Bootstrap at {HOST}:{PORT} for node {NODE_ID}")

    print('\n-> Sending RUN_NODE_REQUEST')
    resp = send_and_recv([0x7b, NODE_ID])
    if resp is None:
        print('No response to RUN_NODE_REQUEST (timeout)')
    else:
        print('RUN response:', ' '.join(f"0x{b:02x}" for b in resp))

    time.sleep(0.5)

    print('\n-> Sending STOP_NODE_REQUEST')
    resp = send_and_recv([0x7d, NODE_ID])
    if resp is None:
        print('No response to STOP_NODE_REQUEST (timeout)')
    else:
        print('STOP response:', ' '.join(f"0x{b:02x}" for b in resp))

    print('\nDone')
