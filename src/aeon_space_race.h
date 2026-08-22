#ifndef AEON_SPACE_RACE_H
#define AEON_SPACE_RACE_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

enum class SpaceMilestone {
    ORBITAL_SATELLITE,
    LUNAR_BASE,
    MARS_COLONY,
    ASTEROID_MINING
};

struct EmpireSpaceProgram {
    int civ_id = 0;
    std::string civ_name;
    SpaceMilestone highest_milestone = SpaceMilestone::ORBITAL_SATELLITE;
    float space_budget_millions = 500.0f;
    int lunar_colonists = 0;
    int mars_colonists = 0;
    double helium3_mined_tons = 0.0;
    double titanium_mined_tons = 0.0;
    int satellites_in_orbit = 5;
};

class AeonSpaceRaceEngine {
public:
    std::vector<EmpireSpaceProgram> programs;
    float orbital_debris_count = 0.0f;
    bool kessler_syndrome_active = false;

    AeonSpaceRaceEngine();
    void init_programs(const AeonEngine& engine);
    void update_space_race_tick(AeonEngine& engine);
    const char* get_milestone_name(SpaceMilestone m) const;
    bool launch_space_mission(int civ_id, SpaceMilestone target_milestone, AeonEngine& engine);
    void fund_space_program(int civ_id, float added_budget_millions);
    void check_kessler_cascade(AeonEngine& engine);
    void process_asteroid_commodity_shock(AeonEngine& engine);
};

} // namespace Aeon

#endif // AEON_SPACE_RACE_H

