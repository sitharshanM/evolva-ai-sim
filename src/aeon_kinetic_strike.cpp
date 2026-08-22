#include "aeon_kinetic_strike.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

void AeonKineticStrikeEngine::init() {
    elevators.clear();
    kinetic_satellites.clear();

    // Equatorial Space Elevator nodes
    elevators.push_back({0, "Quito Equatorial Spire", 98, 90, true, 100.0f});
    elevators.push_back({1, "Kenya Rift Tether", 216, 90, false, 45.0f});
    elevators.push_back({2, "Sumatra Skyhook Pillar", 290, 90, false, 20.0f});

    // Default Kinetic Satellite Platforms
    kinetic_satellites.push_back({1, 0, 6, 420.0f});
    kinetic_satellites.push_back({2, 1, 4, 400.0f});
}

bool AeonKineticStrikeEngine::build_space_elevator(int civ_id, int x, int y, const std::string& location_name, AeonEngine& engine) {
    elevators.push_back({civ_id, location_name, x, y, true, 100.0f});
    engine.history.record(engine.year, engine.month, "SPACE",
        engine.civs[civ_id].name + " Completes Equatorial Space Elevator",
        "Carbon-nanotube tether reaches LEO orbit at " + location_name, civ_id);
    return true;
}

bool AeonKineticStrikeEngine::launch_kinetic_strike(int civ_id, int target_x, int target_y, AeonEngine& engine) {
    for (auto& sat : kinetic_satellites) {
        if (sat.civ_id == civ_id && sat.tungsten_rods > 0) {
            sat.tungsten_rods--;

            // Find target civ at coordinates
            const auto& tile = engine.world_map.tile(target_x, target_y);
            int target_civ = tile.owner_civ;

            engine.history.record(engine.year, engine.month, "KINETIC_STRIKE",
                "⚡ ORBITAL KINETIC STRIKE (Rods from God) Launched by " + engine.civs[civ_id].name,
                "Hypervelocity 20-ton tungsten rod impacts Tile [" + std::to_string(target_x) + ", " + std::to_string(target_y) + "] at Mach 10! Zero radioactive fallout.", civ_id, target_civ);

            if (target_civ >= 0 && target_civ < (int)engine.civs.size()) {
                engine.civs[target_civ].military_power = std::max(0.0f, engine.civs[target_civ].military_power - 120.0f);
                engine.civs[target_civ].stability = std::max(5.0f, engine.civs[target_civ].stability - 20.0f);
            }

            std::cout << "[KINETIC STRIKE] Tungsten rod impacted target [" << target_x << ", " << target_y << "]" << std::endl;
            return true;
        }
    }
    return false;
}

} // namespace Aeon
