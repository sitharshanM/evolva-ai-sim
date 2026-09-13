"""Offline checks for service contracts and the Compose service graph."""
import ast
from pathlib import Path
import yaml
from qdrant_client import QdrantClient, models
from neo4j import GraphDatabase
from nats.js.client import JetStreamContext
import psycopg
from core import lexical_vector


root = Path(__file__).parent
for path in root.glob('*.py'):
    ast.parse(path.read_text(encoding='utf-8'), filename=str(path))
compose = yaml.safe_load((root/'compose.yaml').read_text())
services = compose['services']
assert {'postgres','nats','neo4j','qdrant','api','relay','analytics','history','knowledge'} <= services.keys()
for name, service in services.items():
    for dependency in service.get('depends_on', []):
        assert dependency in services, (name, dependency)
    for port in service.get('ports', []):
        assert port.startswith('127.0.0.1:'), 'Development ports must remain on loopback'
assert callable(GraphDatabase.driver)
assert callable(JetStreamContext.pull_subscribe)
assert callable(psycopg.connect)
client = QdrantClient(':memory:')
client.create_collection('test', vectors_config=models.VectorParams(size=128, distance=models.Distance.COSINE))
client.upsert('test', points=[models.PointStruct(id=1, vector=lexical_vector('research investment'))])
result = client.query_points('test', query=lexical_vector('research investment'), limit=1)
assert result.points[0].id == 1
client.close()
print('Compose graph, Python modules, driver contracts, and local vector index verified')
