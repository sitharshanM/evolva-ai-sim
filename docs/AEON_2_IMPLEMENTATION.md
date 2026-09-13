# AEON 2.0 implementation

The supplied architecture is the target design. The current application remains
a C++17 simulation with its existing ImGui presentation and subsystem engines.
The action boundary is now joined by an integrated simulation runtime, nation
cabinet cognition, replay, checkpoints, and an external service setup. The full
21-layer architecture is not yet complete; limits are listed below.

## Implemented in this increment

- Extracted the existing action validator into its own translation unit so it can
  be tested independently of LLM and graphics dependencies.
- Reject unknown actions, malformed numeric fields, invalid targets, dead actors,
  and military purchases without sufficient annual income.
- Revalidate AI proposals at execution against current world state and cooldowns.
  The proposal's validation flag is informational, not permission to mutate state.
- Record rejected proposals in existing history. Record action memory, fatigue,
  and cooldowns after execution rather than during proposal generation.
- Execute negotiated peace bilaterally, remove its active war event, and update
  treaty memory. Previously the selected peace action had no execution handler.
- Add standalone validation regression coverage and a dependency-free CMake mode.

## Runtime and service implementation

- Stable scheduled command ordering by due year, priority, and insertion ID.
- Recorded AI actions, direct engine actions, presidential war declarations,
  decrees, crisis responses, and system restoration actions.
- Political eligibility checks and explicit subsystem phases.
- Isolated nation observations, uncertain military estimates, cabinet proposals,
  critic explanations, risk-adjusted selection, memory, and planning horizons.
- Optional parallel cabinet workers with stable ordered application.
- Immutable runtime events with parent IDs, action deltas, and subsystem history.
- Exact RNG serialization and frame-independent market updates.
- In-memory full-engine checkpoints, rollback, and independent branches.
- Durable seed-and-command replay with metric, event, and pending-command checks.
  Failed replay leaves its destination unchanged.
- Cognition, intelligence, causal history, replay, checkpoint, and one-year
  research counterfactual controls in the UI.
- PostgreSQL archive storage, NATS delivery, analytics/history workers, Neo4j
  causal graphs, Qdrant lexical memory indexing, and a SQLite transfer retry cache.

See [service setup](../services/README.md) for the external development cluster.
Run `services/setup.ps1` after installing and starting Docker Desktop.

## Remaining work and limits

1. Docker is absent on this PC. Live cluster startup, database/broker integration,
   and worker recovery have not been verified. The cluster is not running.
2. Some legacy GUI/admin and specialized subsystem controls still mutate state
   directly. Complete command conversion is required before promising replay of
   every interactive workflow. Legacy JSON saves remain a separate format.
3. Durable replay reconstructs a managed run from its seed and commands. It is
   not an arbitrary-state binary snapshot or a cross-version replay guarantee.
   In-memory checkpoints copy all engine subsystems in deterministic cabinet mode.
4. Nation memory and strategy are basic. Stale intelligence, false reports,
   promises/betrayals, persistent multi-step plans, and richer debate remain.
5. Event links describe execution provenance, not scientific proof of every
   emergent cause. Detailed delayed-effect attribution remains to be added.
6. Remote workers project immutable archives; the authoritative simulation stays
   in one C++ process. Remote AI inference and simulation partitioning remain.
7. Qdrant uses lexical vectors rather than semantic embeddings. History labels
   are basic classifications rather than a fully developed historian.
8. Thousand-year balance/performance and visual GUI interaction need more testing.

## Validation

Configure with `-DAEON_VALIDATION_ONLY=ON`, build, and run CTest to test the action
validator without downloading the application's graphics dependencies. Tests
cover unknown actions, invalid numbers and targets, dead civilizations, military
affordability, alliance restrictions, trade/war cooldowns, and reciprocal peace.

Verification completed: full Release build; all three CTest suites; 27 validator
checks; eight execution checks; runtime checks for isolation, parallel equivalence,
RNG restoration, rollback, branches, scheduling, replay, and tamper rejection;
and all 10 existing government tests.

A seed-42 simulation and replay passed through 2051 (25 years). Six Python service
tests passed. Compose structure, driver contracts, and local Qdrant insertion and
retrieval were verified; live external services remain untested without Docker.

Run `build-ninja/bin/DigitalLife.exe --seed 42 --years 25 --export-replay build/run.json`,
then `build-ninja/bin/DigitalLife.exe --replay build/run.json`. Add
`--parallel-advisors` for concurrent nation deliberation.
