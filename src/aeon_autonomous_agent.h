#ifndef AEON_AUTONOMOUS_AGENT_H
#define AEON_AUTONOMOUS_AGENT_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

enum class ControlMode {
    MANUAL,
    AUTONOMOUS_AI
};

struct AutonomousAgent {
    int id = 1;
    std::string name = "Envoy Avatar X-1";
    int x = 55;
    int y = 20;
    float health = 100.0f;
    float energy = 100.0f;
    int gold = 500;
    ControlMode mode = ControlMode::MANUAL;
    std::string current_goal = "Explore map and negotiate trade";
    std::string last_ai_reasoning = "Idle — awaiting player command or AI autopilot activation.";
    std::vector<std::pair<int, int>> path_history;
};

class AeonAgentEngine {
public:
    AutonomousAgent agent;
    bool global_ai_governor_active = false; // Toggle full AI takeover for human nation

    AeonAgentEngine();
    void init_agent(int start_x, int start_y);
    void move_manual(int dx, int dy, AeonEngine& engine);
    void update_ai_tick(AeonEngine& engine);
    void set_control_mode(ControlMode new_mode);
};

} // namespace Aeon

#endif // AEON_AUTONOMOUS_AGENT_H
