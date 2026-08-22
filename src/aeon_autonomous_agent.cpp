#include "aeon_autonomous_agent.h"
#include "aeon_engine.h"
#include "aeon_ollama.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Aeon {

AeonAgentEngine::AeonAgentEngine() {
    init_agent(55, 20);
}

void AeonAgentEngine::init_agent(int start_x, int start_y) {
    agent.id = 1;
    agent.name = "Envoy Avatar X-1";
    agent.x = start_x;
    agent.y = start_y;
    agent.health = 100.0f;
    agent.energy = 100.0f;
    agent.gold = 1000;
    agent.mode = ControlMode::MANUAL;
    agent.current_goal = "Explore map and establish trade links";
    agent.last_ai_reasoning = "Agent initialized at central capital. Ready for manual or AI autonomous command.";
    agent.path_history.clear();
    agent.path_history.push_back({start_x, start_y});
}

void AeonAgentEngine::set_control_mode(ControlMode new_mode) {
    agent.mode = new_mode;
    if (new_mode == ControlMode::AUTONOMOUS_AI) {
        agent.last_ai_reasoning = "AI Autopilot Engaged: Scanning world map for optimal trade nodes & diplomatic targets...";
    } else {
        agent.last_ai_reasoning = "Manual Control Engaged: Player in direct command of Envoy movements.";
    }
}

void AeonAgentEngine::move_manual(int dx, int dy, AeonEngine& engine) {
    int nx = std::max(0, std::min(MAP_WIDTH - 1, agent.x + dx));
    int ny = std::max(0, std::min(MAP_HEIGHT - 1, agent.y + dy));

    agent.x = nx;
    agent.y = ny;
    agent.path_history.push_back({nx, ny});
    if (agent.path_history.size() > 50) agent.path_history.erase(agent.path_history.begin());

    agent.energy = std::max(0.0f, agent.energy - 1.0f);

    // Inspect target tile and gather resource/gold
    const auto& tile = engine.world_map.tile(nx, ny);
    if (!tile.resources.empty()) {
        agent.gold += 25;
        agent.last_ai_reasoning = "Gathered 25 gold resources from tile [" + std::to_string(nx) + ", " + std::to_string(ny) + "].";
    }
}

void AeonAgentEngine::update_ai_tick(AeonEngine& engine) {
    if (agent.mode != ControlMode::AUTONOMOUS_AI) return;

    // AI Autopilot Decision Logic: Pathfind toward nearest civilization capital or trade hub
    int best_target_x = agent.x;
    int best_target_y = agent.y;
    float min_dist = 9999.0f;
    std::string target_name = "Commons Hub";

    for (const auto& civ : engine.civs) {
        if (civ.is_alive <= 0.0f) continue;
        float d = (float)(std::abs(agent.x - civ.capital_x) + std::abs(agent.y - civ.capital_y));
        if (d > 0.0f && d < min_dist) {
            min_dist = d;
            best_target_x = civ.capital_x;
            best_target_y = civ.capital_y;
            target_name = civ.name;
        }
    }

    int dx = 0, dy = 0;
    if (best_target_x > agent.x) dx = 1;
    else if (best_target_x < agent.x) dx = -1;

    if (best_target_y > agent.y) dy = 1;
    else if (best_target_y < agent.y) dy = -1;

    // Move 1 step towards target
    move_manual(dx, dy, engine);

    agent.last_ai_reasoning = "AI Autopilot: Navigating toward " + target_name + " capital at (" +
                              std::to_string(best_target_x) + ", " + std::to_string(best_target_y) +
                              "). Distance: " + std::to_string((int)min_dist) + " tiles.";
}

} // namespace Aeon
