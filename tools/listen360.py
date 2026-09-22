import socket

LOG = r"E:\360ports\out\results.log"

srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("0.0.0.0", 4243))
srv.settimeout(30)
srv.listen(4)
open(LOG, "a").write("listening on :4243\n")
try:
    while True:
        conn, addr = srv.accept()
        conn.settimeout(10)
        data = b""
        try:
            while True:
                chunk = conn.recv(256)
                if not chunk:
                    break
                data += chunk
        except socket.timeout:
            pass
        open(LOG, "a").write(f"{addr}: {data!r}\n")
        print(f"{addr}: {data!r}", flush=True)
        conn.close()
except socket.timeout:
    open(LOG, "a").write("listener idle timeout\n")
    print("idle timeout", flush=True)
