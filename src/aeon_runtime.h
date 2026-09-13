#pragma once
#include "aeon_cognition.h"
#include <nlohmann/json.hpp>
#include <memory>

namespace Aeon {
class AeonEngine;
enum class CommandSource { PLAYER, AI, SYSTEM, ADMIN };
enum class CommandKind { ACTION, PRESIDENTIAL_DECREE, CRISIS_RESPONSE };
enum class SimulationPhase { IDLE, COMMANDS, WORLD, SOCIETY, COGNITION, CONSEQUENCES, HISTORY };
struct SimulationCommand {
    uint64_t id = 0, cause = 0;
    int actor = -1, due_year = 0, priority = 0;
    CommandSource source = CommandSource::SYSTEM;
    CommandKind kind = CommandKind::ACTION;
    int option = 0;
    std::string text;
    AIDecision proposal;
};
struct RuntimeEvent {
    uint64_t id = 0, cause = 0;
    int year = 0, actor = -1, target = -1;
    std::string type, description;
    nlohmann::json effects = nlohmann::json::array();
};
class SimulationRuntime {
public:
    bool enabled = true, replaying = false, parallel_advisors = false;
    SimulationPhase phase = SimulationPhase::IDLE;
    uint64_t next_command = 1, next_event = 1, current_cause = 0;
    std::map<int, NationCognition> cognition;
    std::vector<SimulationCommand> pending;
    const std::vector<RuntimeEvent>& events() const { return events_; }
    const nlohmann::json& journal() const { return journal_; }
    uint64_t submit(AeonEngine&, SimulationCommand);
    void dispatch_due(AeonEngine&);
    void think(AeonEngine&);
    uint64_t record(AeonEngine&, std::string, int, int, std::string,
                    nlohmann::json effects = nlohmann::json::array());
    void begin_tick(AeonEngine&);
    void end_tick(AeonEngine&);
    std::string explain(int nation) const;
    nlohmann::json export_data() const;
    bool write_archive(const AeonEngine&, const std::string&, std::string&) const;
    static bool replay_archive(AeonEngine&, const std::string&, std::string&);
private:
    size_t history_start_ = 0;
    std::vector<RuntimeEvent> events_;
    nlohmann::json journal_ = nlohmann::json::array();
};

// Checkpoints own independent copies of all engine subsystems, including RNG.
// Keep this outside AeonEngine to avoid snapshots recursively owning snapshots.
class SimulationTimeline {
public:
    void checkpoint(const AeonEngine&);
    bool restore(AeonEngine&, size_t index) const;
    std::unique_ptr<AeonEngine> branch(size_t index) const;
    size_t size() const { return snapshots_.size(); }
private:
    std::vector<std::shared_ptr<AeonEngine>> snapshots_;
};
nlohmann::json world_metrics(const AeonEngine&);
}
