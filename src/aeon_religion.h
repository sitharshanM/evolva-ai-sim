#ifndef AEON_RELIGION_H
#define AEON_RELIGION_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct WorldReligion {
    int id = 0;
    std::string name;
    std::string holy_city;
    int founder_civ_id = 0;
    float global_followers_pct = 0.0f;
    float faith_power = 100.0f;
    std::string sacred_relic;
    bool holy_war_active = false;
    int crusade_target_civ_id = -1;
    float schism_risk = 5.0f; // 0..100%
    bool is_heresy = false;
    int parent_religion_id = -1;
    std::vector<int> coalition_member_civ_ids;
};

class AeonReligionEngine {
public:
    std::vector<WorldReligion> religions;
    float player_faith_points = 500.0f;
    int active_missionaries = 2;
    std::vector<std::string> holy_relics_collected;

    void init();
    void tick_year(AeonEngine& engine);
    bool found_religion(AeonEngine& engine, const std::string& name, const std::string& relic_name);
    bool dispatch_missionary(AeonEngine& engine, int target_civ_id);
    bool declare_holy_war(AeonEngine& engine, int target_civ_id);
    bool consecrate_shrine(AeonEngine& engine);
    void check_religious_schism(AeonEngine& engine);
    void resolve_holy_war_tick(AeonEngine& engine);

    // Compatibility method
    std::string report() const {
        return "--- WORLD RELIGIONS & FAITH ---\nActive Faiths: " + std::to_string(religions.size());
    }
};

using ReligionEngine = AeonReligionEngine;

} // namespace Aeon

#endif // AEON_RELIGION_H

