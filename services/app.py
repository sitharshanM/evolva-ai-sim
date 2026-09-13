import hmac
import json
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse
from psycopg.types.json import Jsonb
from core import validate_archive
from store import connect, initialize


class Handler(BaseHTTPRequestHandler):
    def reply(self, code, body):
        data = json.dumps(body, allow_nan=False).encode()
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def authorized(self):
        expected = os.environ['AEON_API_TOKEN']
        return bool(expected) and hmac.compare_digest(self.headers.get('Authorization', ''), 'Bearer '+expected)

    def do_GET(self):
        path = urlparse(self.path).path
        if path == '/health':
            with connect() as db:
                db.execute('SELECT 1')
            return self.reply(200, {'status': 'ready'})
        if not self.authorized():
            return self.reply(401, {'error': 'Authentication required'})
        parts = path.strip('/').split('/')
        if len(parts) == 2 and parts[0] == 'runs':
            with connect() as db:
                row = db.execute('SELECT archive FROM runs WHERE id=%s', (parts[1],)).fetchone()
                projections = db.execute('SELECT role,result FROM projections WHERE run_id=%s', (parts[1],)).fetchall()
            return self.reply(200, {'archive': row[0], 'projections': dict(projections)}) if row else self.reply(404, {'error': 'Unknown run'})
        return self.reply(404, {'error': 'Unknown endpoint'})

    def do_POST(self):
        if not self.authorized():
            return self.reply(401, {'error': 'Authentication required'})
        if urlparse(self.path).path != '/runs':
            return self.reply(404, {'error': 'Unknown endpoint'})
        try:
            length = int(self.headers.get('Content-Length', '0'))
            if length <= 0 or length > 64*1024*1024:
                return self.reply(413, {'error': 'Archive must be between 1 byte and 64 MiB'})
            archive = json.loads(self.rfile.read(length))
            run_id = validate_archive(archive)
            with connect() as db:
                db.execute('INSERT INTO runs(id,archive) VALUES(%s,%s) ON CONFLICT DO NOTHING', (run_id, Jsonb(archive)))
                db.execute('INSERT INTO outbox(run_id) VALUES(%s) ON CONFLICT DO NOTHING', (run_id,))
            return self.reply(202, {'run_id': run_id, 'status': 'queued'})
        except (ValueError, KeyError, TypeError) as exc:
            return self.reply(400, {'error': str(exc)})


if __name__ == '__main__':
    initialize()
    ThreadingHTTPServer(('0.0.0.0', 8787), Handler).serve_forever()
