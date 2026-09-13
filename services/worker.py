import asyncio
import json
import os
import sys
import uuid
import nats
from nats.js.errors import NotFoundError
from psycopg.types.json import Jsonb
from neo4j import GraphDatabase
from qdrant_client import QdrantClient, models
from core import analyze, chronicle, lexical_vector
from store import connect, initialize


def project_knowledge(run_id, archive):
    graph = GraphDatabase.driver(os.environ['NEO4J_URL'], auth=('neo4j', os.environ['NEO4J_PASSWORD']))
    vectors = QdrantClient(url=os.environ['QDRANT_URL'], timeout=30)
    if not vectors.collection_exists('aeon_memory'):
        vectors.create_collection('aeon_memory', vectors_config=models.VectorParams(size=128, distance=models.Distance.COSINE))
    try:
        graph.execute_query('CREATE CONSTRAINT aeon_event_key IF NOT EXISTS FOR (e:Event) REQUIRE e.key IS UNIQUE')
        graph.execute_query('CREATE CONSTRAINT aeon_nation_key IF NOT EXISTS FOR (n:Nation) REQUIRE n.key IS UNIQUE')
        for event in archive['runtime']['events']:
            key = f"{run_id}:{event['id']}"
            graph.execute_query('MERGE (e:Event {key:$key}) SET e.type=$type, e.year=$year, e.description=$description',
                                key=key, type=event['type'], year=event['year'], description=event['description'])
            if event['cause']:
                graph.execute_query('MATCH (a:Event {key:$parent}),(b:Event {key:$child}) MERGE (a)-[:CAUSES]->(b)',
                                    parent=f"{run_id}:{event['cause']}", child=key)
            if event['actor'] >= 0:
                graph.execute_query('MERGE (n:Nation {key:$nation}) WITH n MATCH (e:Event {key:$event}) MERGE (n)-[:ACTED]->(e)',
                                    nation=f"{run_id}:nation:{event['actor']}", event=key)
            vectors.upsert('aeon_memory', points=[models.PointStruct(id=str(uuid.uuid5(uuid.NAMESPACE_URL, key)),
                vector=lexical_vector(event['description']), payload={'run_id': run_id, **event})], wait=True)
    finally:
        graph.close()
        vectors.close()
    return {'indexed_events': len(archive['runtime']['events']), 'retrieval': 'lexical feature hashing'}


def process(role, run_id):
    with connect() as db:
        if db.execute('SELECT 1 FROM projections WHERE run_id=%s AND role=%s', (run_id, role)).fetchone():
            return
        row = db.execute('SELECT archive FROM runs WHERE id=%s', (run_id,)).fetchone()
        if row is None:
            raise ValueError('Unknown run')
        archive = row[0]
    result = analyze(archive) if role == 'analytics' else chronicle(archive) if role == 'history' else project_knowledge(run_id, archive)
    with connect() as db:
        db.execute('INSERT INTO projections(run_id,role,result) VALUES(%s,%s,%s) ON CONFLICT DO NOTHING', (run_id, role, Jsonb(result)))


async def main(role):
    initialize()
    client = await nats.connect(os.environ['NATS_URL'], max_reconnect_attempts=-1)
    stream = client.jetstream()
    try:
        await stream.stream_info('AEON')
    except NotFoundError:
        try:
            await stream.add_stream(name='AEON', subjects=['aeon.runs'])
        except Exception:
            await stream.stream_info('AEON')
    if role == 'relay':
        while True:
            with connect() as db:
                rows = db.execute('SELECT run_id FROM outbox WHERE NOT sent FOR UPDATE SKIP LOCKED LIMIT 10').fetchall()
                for (run_id,) in rows:
                    await stream.publish('aeon.runs', run_id.encode(), headers={'Nats-Msg-Id': run_id})
                    db.execute('UPDATE outbox SET sent=true WHERE run_id=%s', (run_id,))
            await asyncio.sleep(1)
    else:
        subscription = await stream.pull_subscribe('aeon.runs', durable='aeon-'+role)
        while True:
            try:
                messages = await subscription.fetch(1, timeout=5)
            except nats.errors.TimeoutError:
                continue
            for message in messages:
                try:
                    task = asyncio.create_task(asyncio.to_thread(process, role, message.data.decode()))
                    while not task.done():
                        done, _ = await asyncio.wait({task}, timeout=10)
                        if not done:
                            await message.in_progress()
                    await task
                    await message.ack()
                except Exception as error:
                    print(f'{role}: {error}', flush=True)
                    await message.nak(delay=5)


if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] not in {'relay', 'analytics', 'history', 'knowledge'}:
        raise SystemExit('Expected relay, analytics, history, or knowledge')
    asyncio.run(main(sys.argv[1]))
