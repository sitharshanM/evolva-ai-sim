#pragma once
#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

enum class TacticalUnitType {
    INFANTRY  = 0,
    ARMOR     = 1,
    ARTILLERY = 2,
    AIR_SUPPORT = 3
};

const char* tactical_unit_name(TacticalUnitType type);

struct TacticalUnit {
    int id = 0;
    std::string name;
    TacticalUnitType type = TacticalUnitType::INFANTRY;
    int civ_id = 0;
    int x = 0;
    int y = 0;
    float hp = 100.0f;
    float max_hp = 100.0f;
    float attack_power = 25.0f;
    float defense = 15.0f;
    int attack_range = 1;
    bool has_moved = false;
};

class AeonTacticalBattleEngine {
public:
    AeonTacticalBattleEngine() = default;

    void init();
    void start_battle(int attacker_civ, int defender_civ, const std::string& location_name, AeonEngine& engine);
    bool move_unit(int unit_id, int new_x, int new_y);
    bool attack_unit(int attacker_unit_id, int target_unit_id, AeonEngine& engine);
    void resolve_turn(AeonEngine& engine);

    bool active_battle = false;
    int attacker_civ_id = 0;
    int defender_civ_id = 1;
    std::string battle_location;
    int current_turn_civ_id = 0;
    int turn_number = 1;
    int victor_civ_id = -1;

    std::vector<TacticalUnit> units;
};

} // namespace Aeon
