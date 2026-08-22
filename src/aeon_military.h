#ifndef AEON_MILITARY_H
#define AEON_MILITARY_H

#include <string>
#include <vector>
#include "aeon_civilization.h"
#include "aeon_world_types.h"

namespace Aeon {

class AeonEngine;

enum class UnitType {
    Infantry,
    ArmoredBrigade,
    AirSquadron,
    NavalFleet
};

// ─── Combat Terrain Modifier ───────────────────────────────────────────────────────────
struct TerrainCombatModifiers {
    float attacker_mult = 1.0f; // <1 = terrain hurts attacker
    float defender_mult = 1.0f; // >1 = terrain helps defender
    float supply_cost   = 1.0f; // >1 = supply lines cost more
    float siege_speed   = 1.0f; // >1 = sieges faster
};

// Returns terrain modifiers for a given biome in battle context
TerrainCombatModifiers get_terrain_modifiers(const std::string& terrain_type);

// ─── Guerrilla Cell ─────────────────────────────────────────────────────────────────
struct GuerrillaCell {
    int   occupier_civ_id = -1;  // civ being fought
    int   native_civ_id   = -1;  // civ the resistance fights for
    int   strength        = 100; // fighters; attritions per turn
    int   active_years    = 0;
    float sabotage_rate   = 0.05f;// fraction of occupier supply burned per year
};

struct MilitaryDivision {
    int id = 0;
    int civ_id = 0;
    std::string name;
    UnitType type = UnitType::Infantry;
    int x = 0;
    int y = 0;
    float personnel = 1000.0f;
    float max_personnel = 1000.0f;
    float supply_level = 1.0f;     // 0.0 to 1.0
    float fuel_ammo = 1.0f;        // 0.0 to 1.0
    float entrenchment = 0.0f;     // 0.0 to 100.0
    float combat_experience = 10.0f; // 0.0 to 100.0
    bool  in_combat = false;
    // Accuracy additions
    bool  deployed  = false;           // is unit deployed outside home territory
    int   siege_target_city_id = -1;   // -1 = not besieging any city
};

struct FrontlineZone {
    int civ1_id = -1;
    int civ2_id = -1;
    int center_x = 0;
    int center_y = 0;
    float intensity = 0.0f;
};

class AeonMilitaryEngine {
public:
    std::vector<MilitaryDivision> divisions;
    std::vector<FrontlineZone> frontlines;
    std::vector<GuerrillaCell> guerrilla_cells;

    AeonMilitaryEngine();
    void init_default_forces(const AeonEngine& engine);
    void update_military_tick(AeonEngine& engine);

    const char* get_unit_type_name(UnitType type) const;
    float get_civ_total_military_power(int civ_id) const;
    void simulate_war_engagement(AeonCivilization& attacker, AeonCivilization& defender, AeonEngine& engine);
    void check_for_guerrilla_uprising(int occupied_civ_id, int occupier_civ_id, AeonEngine& engine);
    void tick_supply_lines(AeonEngine& engine);
    void resolve_sieges(AeonEngine& engine);
    void execute_airstrike(int division_id, int target_x, int target_y, AeonEngine& engine);
private:
    float calculate_tech_era_combat_bonus(TechEra attacker_era, TechEra defender_era) const;
    void relocate_division(int division_id, int new_x, int new_y, AeonEngine& engine);
};

} // namespace Aeon

#endif // AEON_MILITARY_H
