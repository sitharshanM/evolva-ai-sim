import os
import psycopg

SCHEMA = '''
CREATE TABLE IF NOT EXISTS runs (
 id text PRIMARY KEY, archive jsonb NOT NULL, created_at timestamptz NOT NULL DEFAULT now());
CREATE TABLE IF NOT EXISTS outbox (
 run_id text PRIMARY KEY REFERENCES runs(id), sent boolean NOT NULL DEFAULT false);
CREATE TABLE IF NOT EXISTS projections (
 run_id text NOT NULL REFERENCES runs(id), role text NOT NULL, result jsonb NOT NULL,
 PRIMARY KEY(run_id, role));
'''


def connect():
    return psycopg.connect(os.environ['DATABASE_URL'])


def initialize():
    with connect() as connection:
        connection.execute(SCHEMA)
