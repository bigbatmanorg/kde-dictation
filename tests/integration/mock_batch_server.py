#!/usr/bin/env python3
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import sys

class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        if self.path != "/v1/audio/transcriptions":
            self.send_error(404)
            return
        length = int(self.headers.get("Content-Length", "0"))
        _ = self.rfile.read(length)
        body = json.dumps({"text": "mock transcript"}).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        pass

port = int(sys.argv[1]) if len(sys.argv) > 1 else 18765
HTTPServer(("127.0.0.1", port), Handler).serve_forever()
