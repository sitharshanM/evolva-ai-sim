#ifndef AEON_REBELLION_H
#define AEON_REBELLION_H

#include "aeon_world_types.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Aeon {

class AeonEngine;
class AeonCivilization;

struct RebelFaction {
    int id = 0;
    int parent_civ_id = -1;
    int rebel_civ_id = -1;
    std::string name;             // e.g. "Northern Republic"
    std::string ideology;         // e.g. "Democratic Separatists"
    IdeologyType ideology_type = IdeologyType::REPUBLICANISM;
    float strength = 50.0f;
    bool active = false;

    // Defecting generals & military split
    int leader_character_id = -1;
    std::string leader_name;
    std::vector<int> defecting_general_ids;
    float defected_army_size = 0.0f;
    int defected_division_count = 0;

    // Physical territory
    std::vector<std::string> seceded_provinces;

    // Foreign recognition & funding
    std::vector<int> recognized_by_civ_ids;
    std::vector<int> funded_by_civ_ids;
    float foreign_financial_aid = 0.0f;
};

class AeonRebellionEngine {
public:
    std::vector<RebelFaction> rebellions;
    int last_crisis_year = -999;
    std::unordered_map<int, int> civ_last_crisis_year;

    AeonRebellionEngine();
    void update_rebellions_tick(AeonEngine& engine);
    void trigger_secession(int civ_id, AeonEngine& engine);
    void trigger_state_splinter(int civ_id, AeonEngine& engine);
    std::string generate_unique_successor_name(const AeonCivilization& parent,
                                               const std::string& prov_name,
                                               IdeologyType ideology,
                                               GovForm gov,
                                               const std::vector<AeonCivilization>& all_civs);
};

} // namespace Aeon

#endif // AEON_REBELLION_H

