#pragma once
#include "aeon_world_types.h"
#include <string>
#include <vector>
#include <map>

namespace Aeon {

class AeonEngine;

// ─────────────────────────────────────────────────────────────────────────────
//  Space Program State
// ─────────────────────────────────────────────────────────────────────────────
struct SpaceProgramState {
    int level = 0; // 0: None, 1: Agency Founded, 2: Satellites, 3: Moon Landing, 4: Orbital Defense
    int satellites_launched = 0;
    bool moon_landing_achieved = false;
    bool orbital_defense_active = false;
    float orbital_debris_density = 0.0f; // Kessler syndrome risk (0.0 to 1.0)
};

// ─────────────────────────────────────────────────────────────────────────────
//  Nuclear Arsenal State
// ─────────────────────────────────────────────────────────────────────────────
struct NuclearState {
    int silos_built = 0;
    int icbm_stockpile = 0;
    int nuclear_submarines = 0;
    int stealth_bombers = 0;
    bool non_proliferation_signed = false;
    float early_warning_radar = 0.0f; // 0-100% intercept chance
};

// ─────────────────────────────────────────────────────────────────────────────
//  Covert Intelligence State
// ─────────────────────────────────────────────────────────────────────────────
struct EspionageState {
    int agency_level = 1;
    int active_spies = 2;
    float intel_budget = 100.0f; // gold/yr
    std::map<int, float> infiltration_levels; // civ_id -> infiltration % (0-100)
    std::vector<std::string> covert_ops_log;
};

enum class MegaprojectType {
    MOON_LANDING,
    MANHATTAN_PROJECT,
    SATELLITE_CONSTELLATION,
    FUSION_POWER,
    GENOME_SYNTHESIZER
};

inline const char* megaproject_type_name(MegaprojectType t) {
    switch (t) {
        case MegaprojectType::MOON_LANDING:            return "Apollo Lunar Program 🌕";
        case MegaprojectType::MANHATTAN_PROJECT:       return "Manhattan Atomic Project ⚛️";
        case MegaprojectType::SATELLITE_CONSTELLATION: return "Global GPS & Defense Grid 🛰️";
        case MegaprojectType::FUSION_POWER:            return "Net Fusion Energy Grid ⚡";
        case MegaprojectType::GENOME_SYNTHESIZER:      return "Human Genome Synthesizer 🧬";
    }
    return "Megaproject";
}

struct Megaproject {
    MegaprojectType type = MegaprojectType::MOON_LANDING;
    std::string name;
    int current_phase = 0; // 0 = Not started, 1..4 = Active phase
    int max_phases = 4;
    float phase_cost[4] = {40000.0f, 60000.0f, 80000.0f, 100000.0f}; // IRL scale ($280k Gold total)
    float phase_progress[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    bool completed = false;
    float failure_risk = 0.20f; // 20% risk of test explosion
};

// ─────────────────────────────────────────────────────────────────────────────
//  AeonSpaceEspionage Engine
// ─────────────────────────────────────────────────────────────────────────────
class AeonSpaceEspionage {
public:
    AeonSpaceEspionage() = default;

    SpaceProgramState space;
    NuclearState      nuke;
    EspionageState    espionage;
    std::vector<Megaproject> megaprojects;

    void init();
    void tick_year(AeonEngine& engine);

    // Realistic Multi-Stage Megaproject Progression
    bool invest_in_megaproject(AeonEngine& engine, int project_idx, float gold_amount);
    bool attempt_phase_launch(AeonEngine& engine, int project_idx);

    // Space Actions
    bool launch_satellite(AeonEngine& engine);
    bool launch_moon_mission(AeonEngine& engine);
    bool build_orbital_defense(AeonEngine& engine);

    // Nuclear Actions
    bool build_nuke_silo(AeonEngine& engine);
    bool construct_icbm(AeonEngine& engine);
    bool launch_icbm_strike(AeonEngine& engine, int target_civ_id);

    // Espionage Actions
    bool execute_mission(int actor_civ_id, int target_civ_id, EspionageMissionType mission, AeonEngine& engine, bool& out_detected);
    bool stage_foreign_coup(AeonEngine& engine, int target_civ_id);
    bool assassinate_ruler(AeonEngine& engine, int target_civ_id);
    bool inject_disinformation(AeonEngine& engine, int target_civ_id);
    
    // New Cyberwarfare & Deep Covert Operations
    bool steal_tech_blueprints(AeonEngine& engine, int target_civ_id);
    bool execute_power_grid_blackout(AeonEngine& engine, int target_civ_id);
    bool execute_financial_hack(AeonEngine& engine, int target_civ_id);
    bool stage_false_flag_incident(AeonEngine& engine, int victim_civ_id, int framed_civ_id);
    bool fund_proxy_rebellion(AeonEngine& engine, int target_civ_id, float funding_gold);
    float get_infiltration(int civ_id) const;
};

} // namespace Aeon

