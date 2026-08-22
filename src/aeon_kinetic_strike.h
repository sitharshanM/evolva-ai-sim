#pragma once
#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct SpaceElevatorNode {
    int civ_id = 0;
    std::string location_name;
    int x = 0;
    int y = 0;
    bool operational = false;
    float construction_pct = 0.0f;
};

struct KineticRodSatellite {
    int id = 0;
    int civ_id = 0;
    int tungsten_rods = 6;
    float orbit_alt_km = 400.0f;
};

class AeonKineticStrikeEngine {
public:
    AeonKineticStrikeEngine() = default;

    void init();
    bool build_space_elevator(int civ_id, int x, int y, const std::string& location_name, AeonEngine& engine);
    bool launch_kinetic_strike(int civ_id, int target_x, int target_y, AeonEngine& engine);

    std::vector<SpaceElevatorNode> elevators;
    std::vector<KineticRodSatellite> kinetic_satellites;
};

} // namespace Aeon
