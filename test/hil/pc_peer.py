#!/usr/bin/env python3
"""PC-side peers for the lwIP HIL tests.

Subcommands:
  udp-echo      bind :8080, echo datagrams back to the sender
  udp-listen    bind :8080, print datagrams (e.g. board broadcast)
  mcast-listen  join 224.0.1.0:8080, print datagrams
  tcp-echo      bind :8080, echo bytes per connection (multi-client)
  tcp-server    bind :8080, accept one connection and print
  udp-send H P  send a datagram to host H port P (for board UDP peers)
  http H        HTTP GET http://H/ and print the status/body header
  ping H        ICMP-ish check via TCP connect fallback (prints reachability)

Usage: python pc_peer.py <subcommand> [args] [--seconds N]
"""

import socket
import sys
import threading
import time

TIMEOUT = 6.0


def _run_with_timeout(fn, seconds):
    t = threading.Thread(target=fn, daemon=True)
    t.start()
    t.join(seconds)


def udp_echo(port, seconds):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("0.0.0.0", port))
    print("udp-echo :%d" % port)

    def loop():
        s.settimeout(0.5)
        while True:
            try:
                data, addr = s.recvfrom(2048)
                print("rx %s from %s" % (data[:64], addr))
            except socket.timeout:
                continue

    _run_with_timeout(loop, seconds)


def udp_listen(port, seconds):
    udp_echo(port, seconds)


def mcast_listen(group, port, seconds):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("", port))
    mreq = socket.inet_aton(group) + socket.inet_aton("0.0.0.0")
    s.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
    print("mcast-listen %s:%d" % (group, port))

    def loop():
        s.settimeout(0.5)
        while True:
            try:
                data, addr = s.recvfrom(2048)
                print("rx %s from %s" % (data[:64], addr))
            except socket.timeout:
                continue

    _run_with_timeout(loop, seconds)


def _echo_conn(c):
    while True:
        data = c.recv(2048)
        if not data:
            break
        c.sendall(data)
    c.close()


def tcp_echo(port, seconds):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("0.0.0.0", port))
    s.listen(8)
    print("tcp-echo :%d" % port)

    def loop():
        s.settimeout(0.5)
        while True:
            try:
                c, addr = s.accept()
            except socket.timeout:
                continue
            print("conn %s" % (addr,))
            threading.Thread(target=_echo_conn, args=(c,), daemon=True).start()

    _run_with_timeout(loop, seconds)


def udp_send(host, port, seconds):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    msg = b"HELLO FROM PC\r\n"
    for _ in range(int(seconds * 2)):
        s.sendto(msg, (host, port))
        time.sleep(0.5)


def http(host, seconds):
    s = socket.create_connection((host, 80), timeout=TIMEOUT)
    s.sendall(b"GET / HTTP/1.0\r\nHost: %s\r\n\r\n" % host.encode())
    s.settimeout(TIMEOUT)
    data = s.recv(256)
    print(data.decode("latin1", "ignore"))
    s.close()


def main():
    args = sys.argv[1:]
    seconds = 8
    if "--seconds" in args:
        i = args.index("--seconds")
        seconds = float(args[i + 1])
        del args[i : i + 2]
    cmd = args[0]
    if cmd == "udp-echo":
        udp_echo(int(args[1]) if len(args) > 1 else 8080, seconds)
    elif cmd == "udp-listen":
        udp_listen(int(args[1]) if len(args) > 1 else 8080, seconds)
    elif cmd == "mcast-listen":
        mcast_listen(
            args[1] if len(args) > 1 else "224.0.1.0",
            int(args[2]) if len(args) > 2 else 8080,
            seconds,
        )
    elif cmd == "tcp-echo":
        tcp_echo(int(args[1]) if len(args) > 1 else 8080, seconds)
    elif cmd == "udp-send":
        udp_send(args[1], int(args[2]), seconds)
    elif cmd == "http":
        http(args[1], seconds)
    else:
        print(__doc__)
        sys.exit(1)


if __name__ == "__main__":
    main()
