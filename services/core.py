"""Pure archive validation and deterministic research projections."""
import hashlib
import json
import math
import re


def validate_archive(archive):
    if archive.get('format') != 'aeon-replay-1':
        raise ValueError('Unsupported archive format')
    if not isinstance(archive.get('seed'), int):
        raise ValueError('Missing seed')
    events = archive['runtime']['events']
    if not isinstance(events, list) or len(events) > 1000000:
        raise ValueError('Invalid event collection')
    seen = {0}
    for event in events:
        identity = event['id']
        if not isinstance(identity, int) or identity <= 0 or identity in seen:
            raise ValueError('Duplicate or invalid event ID')
        if event['cause'] not in seen:
            raise ValueError('Causal parent must precede child')
        if not isinstance(event['description'], str):
            raise ValueError('Invalid event description')
        seen.add(identity)
    encoded = json.dumps(archive, sort_keys=True, separators=(',', ':'), allow_nan=False)
    return hashlib.sha256(encoded.encode()).hexdigest()


def lexical_vector(text, dimensions=128):
    """Stable feature hashing; lexical retrieval, not semantic embeddings."""
    vector = [0.0] * dimensions
    for word in re.findall(r'\w+', text.lower()):
        digest = hashlib.sha256(word.encode()).digest()
        vector[int.from_bytes(digest[:4], 'big') % dimensions] += 1 if digest[4] & 1 else -1
    norm = math.sqrt(sum(x*x for x in vector)) or 1
    return [x/norm for x in vector]


def analyze(archive):
    events = archive['runtime']['events']
    counts = {}
    rejected = []
    for event in events:
        counts[event['type']] = counts.get(event['type'], 0) + 1
        if event['type'] == 'REJECTED':
            rejected.append({'id': event['id'], 'cause': event['cause'], 'reason': event['description']})
    return {'event_counts': counts, 'rejections': rejected, 'final_metrics': archive['metrics']}


def chronicle(archive):
    years = {}
    for event in archive['runtime']['events']:
        if event['type'] == 'APPLIED':
            years.setdefault(event['year'], []).append(event['description'])
    return {'eras': [{'year': year, 'title': 'Conflict and diplomacy' if 'DECLARE_WAR' in actions
                    else 'Domestic development', 'actions': actions}
                    for year, actions in sorted(years.items())]}
