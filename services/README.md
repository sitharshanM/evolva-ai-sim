# AEON local service cluster

Run `./services/setup.ps1` from PowerShell after Docker Desktop is installed and
running. The script generates random development credentials in `services/.env`
without printing them, then builds and starts the cluster. Rerunning it preserves
credentials and database volumes. Only the API is exposed to the host, at
`http://127.0.0.1:8787`; databases and the broker remain on the Compose network.

The current machine does not have Docker installed. Configuration, driver APIs,
the local Qdrant index, and pure service tests have been checked, but live Compose
startup and database/broker integration have not been verified here.

## Components

| Service | Responsibility |
| --- | --- |
| PostgreSQL | Immutable content-addressed run archives, transactional outbox, worker results |
| NATS JetStream | Persistent run delivery to independent durable consumers |
| Analytics worker | Event counts, rejected actions, final nation metrics |
| History worker | Deterministic annual narrative classifications grounded in executed actions |
| Knowledge worker | Causal and actor links in Neo4j; lexical memory vectors in Qdrant |
| API | Token-authenticated archive ingestion and retrieval |
| SQLite client cache | Durable archive uploads with retry after connection failures |

The simulation stays authoritative in the C++ process. Workers process immutable
archives and cannot change world state. Worker deliveries are at least once;
content-addressed archive IDs and keyed projection writes make retries idempotent.
This is a development topology, not a high-availability production deployment.

## Transfer a simulation

1. Run `build-ninja/bin/DigitalLife.exe --seed 42 --years 25 --export-replay saves/run.json`.
   Create the `saves` directory first if necessary.
2. Set `AEON_API_TOKEN` in the current shell to the generated value from
   `services/.env`. Do not commit or share that file.
3. Run `python services/client.py --archive saves/run.json`.
4. Query `GET /runs/<run_id>` with `Authorization: Bearer <token>` to retrieve the
   archive and completed analytics, history, and knowledge projections.

If a transfer fails, its archive stays in `saves/service-outbox.db`. Run the client
again without `--archive` to retry pending transfers. `GET /health` checks API/SQL
availability. Worker errors are visible through `docker compose logs`.

Install `services/requirements.txt` into a Python environment for service use
outside Docker. The transfer client and pure tests use only the standard library.

## Checks

- `python -m unittest discover -s services -p 'test_*.py'`
- `python services/verify.py` (requires service dependencies and PyYAML)
- `docker compose --env-file services/.env -f services/compose.yaml config --quiet`

Qdrant memory uses deterministic lexical feature hashing; it is not presented as
semantic embeddings or an LLM. The history worker classifies observed actions;
its era titles are generated labels, not scientific causal conclusions.

API implementations follow the [NATS JetStream documentation](https://docs.nats.io/using-nats/developer/develop_jetstream),
[Neo4j Python driver](https://neo4j.com/docs/python-manual/current/query-simple/),
and [Qdrant quickstart](https://qdrant.tech/documentation/quickstart/).
