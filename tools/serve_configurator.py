#!/usr/bin/env python3
import http.server
import socketserver
import sys
import threading
import webbrowser
from pathlib import Path


DEFAULT_PORT = 8765


class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address = True


class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


def main():
    root = Path(__file__).resolve().parents[1] / "web-config"
    if not root.exists():
        print(f"web-config not found: {root}", file=sys.stderr)
        return 1

    handler = lambda *args, **kwargs: Handler(*args, directory=str(root), **kwargs)
    server = None
    port = DEFAULT_PORT
    for candidate in range(DEFAULT_PORT, DEFAULT_PORT + 20):
        try:
            server = Server(("127.0.0.1", candidate), handler)
            port = candidate
            break
        except OSError:
            continue

    if server is None:
        print("No free localhost port found", file=sys.stderr)
        return 1

    with server:
        url = f"http://127.0.0.1:{port}/"
        print(f"Yobboy Configurator: {url}")
        threading.Timer(0.4, lambda: webbrowser.open(url)).start()
        server.serve_forever()


if __name__ == "__main__":
    raise SystemExit(main())
