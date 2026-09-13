"""Transfer immutable replay archives with a durable local SQLite outbox."""
import argparse
import json
import os
import sqlite3
import urllib.request
from pathlib import Path
from core import validate_archive


def enqueue(database, archive):
    identity = validate_archive(archive)
    database.execute('CREATE TABLE IF NOT EXISTS outbox (id TEXT PRIMARY KEY, archive TEXT NOT NULL, sent INTEGER NOT NULL DEFAULT 0)')
    database.execute('INSERT OR IGNORE INTO outbox(id,archive) VALUES(?,?)', (identity, json.dumps(archive, allow_nan=False)))
    database.commit()
    return identity


def flush(database, server, token):
    if not token:
        raise ValueError('Set AEON_API_TOKEN to the token in services/.env')
    database.execute('CREATE TABLE IF NOT EXISTS outbox (id TEXT PRIMARY KEY, archive TEXT NOT NULL, sent INTEGER NOT NULL DEFAULT 0)')
    sent = 0
    for identity, payload in database.execute('SELECT id,archive FROM outbox WHERE sent=0 ORDER BY rowid').fetchall():
        request = urllib.request.Request(server.rstrip('/')+'/runs', data=payload.encode(),
            headers={'Authorization':'Bearer '+token,'Content-Type':'application/json'}, method='POST')
        with urllib.request.urlopen(request, timeout=30) as response:
            result = json.load(response)
        if result.get('run_id') != identity:
            raise ValueError('Server returned an unexpected archive identity')
        database.execute('UPDATE outbox SET sent=1 WHERE id=?', (identity,))
        database.commit()
        sent += 1
    return sent


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--server', default='http://127.0.0.1:8787')
    parser.add_argument('--cache', type=Path, default=Path('saves/service-outbox.db'))
    args = parser.parse_args()
    args.cache.parent.mkdir(parents=True, exist_ok=True)
    with sqlite3.connect(args.cache) as database:
        if args.archive:
            print('Queued', enqueue(database, json.loads(args.archive.read_text(encoding='utf-8'))))
        print('Transferred', flush(database, args.server, os.getenv('AEON_API_TOKEN','')), 'archives')
