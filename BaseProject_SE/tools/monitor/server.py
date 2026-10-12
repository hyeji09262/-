"""Read-only loopback JSON bridge. No third-party packages, no game control API."""
import argparse
import json
import os
from pathlib import Path
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import webbrowser

ROOT = Path(__file__).resolve().parent
SNAPSHOT = Path(os.environ.get("LOCALAPPDATA", str(Path.home()))) / "Emberwick" / "world_monitor.json"
MAX_BYTES = 2 * 1024 * 1024


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        # Fixed allowlist: no arbitrary filesystem reads or directory browsing.
        route = self.path.split("?", 1)[0]
        if route == "/api/state":
            try:
                with SNAPSHOT.open("rb") as source:
                    payload = source.read(MAX_BYTES + 1)
                if len(payload) > MAX_BYTES:
                    raise ValueError("Snapshot too large")
                state = json.loads(payload)
                if not isinstance(state, dict) or state.get("schema_version") != 1:
                    raise ValueError("Unsupported schema")
                self.reply(200, payload, "application/json; charset=utf-8")
            except FileNotFoundError:
                self.reply_json(503, {"error": "게임 데이터를 기다리는 중입니다. 새 빌드로 게임을 실행하세요."})
            except (OSError, ValueError):
                self.reply_json(503, {"error": "스냅샷을 읽지 못했습니다. 다음 갱신에서 재시도합니다."})
        elif route in ("/", "/index.html"):
            self.reply(200, (ROOT / "index.html").read_bytes(), "text/html; charset=utf-8")
        else:
            self.reply_json(404, {"error": "Not found"})

    def reply_json(self, status, data):
        self.reply(status, json.dumps(data, ensure_ascii=False).encode("utf-8"), "application/json; charset=utf-8")

    def reply(self, status, payload, content_type):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Cache-Control", "no-store")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("Content-Security-Policy", "default-src 'self'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; connect-src 'self'; frame-ancestors 'none'")
        self.end_headers()
        self.wfile.write(payload)

    def log_message(self, *_):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--no-browser", action="store_true")
    args = parser.parse_args()
    with ThreadingHTTPServer(("127.0.0.1", args.port), Handler) as server:
        url = f"http://127.0.0.1:{args.port}"
        print(f"World monitor: {url}\nSnapshot: {SNAPSHOT}\nStop: Ctrl+C", flush=True)
        if not args.no_browser:
            webbrowser.open_new(url)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
