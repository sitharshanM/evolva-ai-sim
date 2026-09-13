# AEON 2.0 implementation

The supplied architecture is the target design. The current application remains
a C++17 simulation with its existing ImGui presentation and subsystem engines.
This first increment establishes the authoritative action boundary; it does not
implement all 21 layers.

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

## Remaining architecture work

1. Route player and other subsystem commands through a shared typed dispatcher;
   political transition eligibility still needs a shared policy validator.
2. Introduce a deterministic scheduler and explicit subsystem phase contracts.
3. Add per-nation observations with known, believed, and unknown facts; prevent
   advisor code from consulting hidden world state.
4. Build cabinet proposals, critic assessments, risk scoring, and ruler selection
   over those observations, connecting existing personality and memory systems.
5. Introduce structured consequences and causal event IDs. Existing history is
   a narrative log, not a complete event-sourced replay implementation.
6. Add snapshots, replay, rollback, and alternate-history branching with tests
   of saved random state and deterministic execution.
7. Connect cognition inspection and causal analytics to the UI, then evaluate
   durable graph/vector storage and distributed workers as separate integrations.

## Validation

Configure with `-DAEON_VALIDATION_ONLY=ON`, build, and run CTest to test the action
validator without downloading the application's graphics dependencies. Tests
cover unknown actions, invalid numbers and targets, dead civilizations, military
affordability, alliance restrictions, trade/war cooldowns, and reciprocal peace.

Verification completed: full Release application build; 22 standalone validator
checks; and eight execution checks covering stale validation, rejected-action
cooldowns, bilateral war and peace, war-event cleanup, and affordability.
Both CTest suites pass. The application exposes the execution suite through
`--test-actions`, which creates an isolated two-nation fixture without LLM calls.
Long-running simulation balance and GUI behavior have not been assessed.
