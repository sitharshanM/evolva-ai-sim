#pragma once
#include "aeon_config.h"
#include "aeon_ascii_map.h"
#include "aeon_civilization.h"
#include "aeon_character.h"
#include "aeon_ruler_ai.h"
#include "aeon_history.h"
#include "aeon_economy.h"
#include "aeon_religion.h"
#include "aeon_chronicler.h"
#include "aeon_president.h"
#include "aeon_space_espionage.h"
#include "aeon_coalition.h"
#include "aeon_tech_tree.h"
#include "aeon_trade_caravan.h"
#include "aeon_disasters.h"
#include "aeon_religion.h"
#include "aeon_wonders.h"
#include "aeon_economy_market.h"
#include "aeon_naval.h"
#include "aeon_parliament.h"
#include "aeon_un_council.h"
#include "aeon_alliances.h"
#include "aeon_gis_climate.h"
#include "aeon_supply_demand.h"
#include "aeon_demographics.h"
#include "aeon_military.h"
#include "aeon_central_bank.h"
#include "aeon_autonomous_agent.h"
#include "aeon_rebellion.h"
#include "aeon_nuclear.h"
#include "aeon_space_race.h"
#include "aeon_dynasty.h"
#include "aeon_megawonders.h"
#include "aeon_maritime.h"
#include "aeon_individual_citizens.h"
#include "aeon_analytics.h"
#include "aeon_diplomatic_summit.h"
#include "aeon_genetics.h"
#include "aeon_tactical_battle.h"
#include "aeon_scenario_editor.h"
#include "aeon_ruler_psyche.h"
#include "aeon_deep_state.h"
#include "aeon_kinetic_strike.h"
#include "aeon_government.h"


#include "aeon_random.h"
#include "aeon_unique_id.h"
#include <vector>
#include <string>
#include <random>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  ActiveEvent  —  Something happening in the simulation right now
// ─────────────────────────────────────────────────────────────────────────────
struct ActiveEvent {
    std::string type;          // WAR, FAMINE, DISASTER, SUMMIT, REVOLUTION...
    std::string description;
    int         civ_id   = -1;
    int         civ2_id  = -1;
    float       years_remaining = 1.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
//  AeonEngine  —  Master AEON simulation state
// ─────────────────────────────────────────────────────────────────────────────
class AeonEngine {
public:
    AeonEngine() = default;

    void init(uint64_t seed);
    void tick_second(float real_dt); // real_dt seconds -> years at current speed

    // World time
    int   year   = 2026;
    int   month  = 1;
    int   day    = 1;
    float speed  = SPEED_NORMAL; // years / real second
    bool  paused = false;
    float time_accum = 0.0f; // accumulates fractional years

    uint64_t seed = 928374ULL;

    // Sub-systems
    AsciiMap           world_map;
    AeonHistory        history;
    GlobalMarketEngine market_engine;
    ReligionEngine     religion_engine;
    AeonChronicler     chronicler;


    AeonPresidentGame  president_game;
    AeonSpaceEspionage space_espionage;
    CoalitionEngine    coalition_engine;
    TechTreeEngine     tech_tree_engine;
    TradeCaravanEngine caravan_engine;
    AeonDisasterEngine disaster_engine;
    AeonReligionEngine aeon_religion_engine;
    AeonWonderEngine   wonder_engine;
    AeonEconomyMarketEngine economy_market_engine;
    AeonNavalEngine             naval_engine;
    AeonParliamentEngine        parliament_engine;
    AeonUNCouncilEngine         un_council_engine;
    AeonAllianceEngine          alliance_engine;
    AeonGISClimateEngine        gis_climate_engine;
    AeonSupplyDemandEngine      supply_demand_engine;
    AeonDemographicsEngine      demographics_engine;
    AeonMilitaryEngine          military_engine;
    AeonCentralBankEngine       central_bank_engine;
    AeonAgentEngine             agent_engine;
    AeonRebellionEngine         rebellion_engine;
    AeonNuclearEngine           nuclear_engine;
    AeonSpaceRaceEngine         space_race_engine;
    AeonDynastyEngine           dynasty_engine;
    AeonMegaWonderEngine        megawonder_engine;
    AeonMaritimeEngine          maritime_engine;
    AeonCitizenEngine           citizen_engine;
    AeonAnalyticsEngine         analytics_engine;
    AeonDiplomaticSummitEngine  diplomatic_summit_engine;
    AeonGeneticsEngine          genetics_engine;
    AeonTacticalBattleEngine    tactical_battle_engine;
    AeonScenarioEditorEngine    scenario_editor_engine;
    AeonRulerPsycheEngine       ruler_psyche_engine;
    AeonDeepStateEngine         deep_state_engine;
    AeonKineticStrikeEngine     kinetic_strike_engine;
    GovernmentTransitionEngine  gov_transition_engine;



    std::vector<AeonCivilization> civs;   // 5 empires + Commons = 6
    std::vector<AeonCharacter>    characters;
    std::vector<AeonRulerAI>      ai_controllers;
    std::vector<ActiveEvent>      active_events;

    // Interactive event choice popup state
    bool        show_event_modal = false;
    ActiveEvent active_modal_event;

    // Save/Load
    bool save_world(const std::string& filename) const;
    bool load_world(const std::string& filename);

    // ── Deterministic RNG (Phase 1) ──────────────────────────────────────────
    // rng is the authoritative deterministic RNG for ALL sub-systems.
    // Use rng.uniform(), rng.uniform_int(), rng.chance() etc.
    AeonRandom rng{928374ULL};

    // ── Unique ID registry (Phase 1) ─────────────────────────────────────────
    IDRegistry id_reg;

    // Legacy integer counters (kept for backward compat; prefer id_reg going forward)
    int next_char_id = 1;
    int next_city_id = 1;

    // Council of Nations (emerges dynamically)
    bool  council_exists  = false;
    int   council_year    = -1;
    std::vector<int> council_members;

    // Season name
    const char* current_season() const {
        return SEASONS[(month - 1) / 3];
    }

    // Inspect observability
    std::string inspect_ai_decision(int civ_id) const;
    std::string inspect_tile(int x, int y) const { return world_map.inspect(x, y); }
    std::string status_line() const;

    void tick_years(float years);
    void tick_one_year();

    void apply_decision(int civ_idx, const AIDecision& dec);
    void check_war_resolution(int year);
    void check_council_formation(int year);
    AeonCivilization make_civilization(int id, const std::string& name,
                                       int cap_x, int cap_y, char map_char,
                                       float aggression, float diplo, float sci);
    AeonCharacter    make_ruler(int id, const std::string& name,
                                int civ_id, int birth_year);

private:
    void spawn_random_event(int year);
    void check_nation_collapse(int year);   // NEW: nation death & annexation
    void spawn_new_nation(int year);        // NEW: splinter states & rebellions

    std::mt19937_64  rng_;
};

} // namespace Aeon
