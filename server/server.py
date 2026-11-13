import socket
import struct

HOST = "0.0.0.0"  
PORT = 5050

ULTRASONIC_ID = 1
ULTRASONIC_FMT = "<B3xI20f"                    # id, 3 pad bytes, uint32 millis, 20 floats
ULTRASONIC_SIZE = struct.calcsize(ULTRASONIC_FMT)  # 88

def recv_exact(conn, n):
    buf = b""
    while len(buf) < n:
        chunk = conn.recv(n - len(buf))
        if not chunk:
            return None  # client disconnected
        buf += chunk
    return buf

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((HOST, PORT))
    s.listen()
    print(f"Listening on {PORT}, expecting {ULTRASONIC_SIZE}-byte payloads")
    while True:
        conn, addr = s.accept()
        print("ESP32 connected from", addr)
        with conn:
            while True:
                data = recv_exact(conn, ULTRASONIC_SIZE)
                if data is None:
                    print("ESP32 disconnected")
                    break
                pid, millis, *distances = struct.unpack(ULTRASONIC_FMT, data)
                if pid != ULTRASONIC_ID:
                    print("Unknown payload id", pid)
                    continue
                print(f"t={millis}ms  distances={[round(d, 1) for d in distances]}")