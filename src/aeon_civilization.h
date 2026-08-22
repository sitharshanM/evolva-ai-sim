#pragma once
#include "aeon_config.h"
#include "aeon_world_types.h"
#include "aeon_character.h"
#include <string>
#include <vector>
#include <array>
#include <unordered_map>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  City  —  Individual city within a civilization
// ─────────────────────────────────────────────────────────────────────────────
enum class CityTier { VILLAGE, TOWN, CITY, INDUSTRIAL, MEGACITY, RUINS };
enum class CitySpecialization { NONE, TRADE, MINING, FARMING, MILITARY, SCIENCE };

inline const char* city_spec_name(CitySpecialization s) {
    switch (s) {
        case CitySpecialization::NONE:     return "General";
        case CitySpecialization::TRADE:    return "Trade Hub";
        case CitySpecialization::MINING:   return "Mining City";
        case CitySpecialization::FARMING:  return "Agricultural";
        case CitySpecialization::MILITARY: return "Fortress";
        case CitySpecialization::SCIENCE:  return "Academy";
    }
    return "General";
}

inline const char* city_tier_name(CityTier t) {
    switch (t) {
        case CityTier::VILLAGE:    return "Village";
        case CityTier::TOWN:       return "Town";
        case CityTier::CITY:       return "City";
        case CityTier::INDUSTRIAL: return "Industrial City";
        case CityTier::MEGACITY:   return "Megacity";
        case CityTier::RUINS:      return "Ruins";
    }
    return "Settlement";
}

struct AeonCity {
    int         id         = 0;
    std::string name;
    int         civ_id     = -1;
    int         map_x = 0, map_y = 0;
    CityTier    tier       = CityTier::VILLAGE;
    long long   population = 500;
    float       food_supply  = 80.0f;
    float       happiness    = 70.0f;
    float       crime_rate   = 5.0f;
    float       industry_lvl = 0.0f;
    float       education    = 10.0f;
    int         founded_year = 0;
    bool        is_capital   = false;
    bool        is_ruins     = false;
    int         ruins_since  = -1;
    int         walls_level      = 0;   // 0-5; higher = harder to siege
    CitySpecialization specialization = CitySpecialization::NONE;
    float       pollution_index  = 0.0f;// rises with industry; hits health
    float       water_security   = 80.0f;// 0-100; drought/siege can collapse it
    int         siege_year_start = -1;  // year a siege began (-1 = not besieged)
    bool        under_siege      = false;

    void tick_year(float food_bonus, float tech_mod);
    void upgrade_tier();
    char map_symbol() const;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Explicit Crisis States
// ─────────────────────────────────────────────────────────────────────────────
enum class CrisisState {
    NORMAL,
    POLITICAL_CRISIS,
    ECONOMIC_CRISIS,
    MILITARY_CRISIS,
    CIVIL_UNREST,
    SUCCESSION_CRISIS,
    WAR_CRISIS,
    INSTITUTIONAL_CRISIS,
    // ── Military & Coup States ──────────────────────────────────────
    MILITARY_TENSION,   // Military discontent rising; no open threat yet
    COUP_CRISIS,        // Coup conditions met; threshold crossed; imminent
    MILITARY_REGIME     // Post-coup military junta in control
};

inline const char* crisis_state_name(CrisisState cs) {
    switch (cs) {
        case CrisisState::NORMAL:               return "Normal";
        case CrisisState::POLITICAL_CRISIS:     return "Political Crisis";
        case CrisisState::ECONOMIC_CRISIS:      return "Economic Crisis";
        case CrisisState::MILITARY_CRISIS:      return "Military Crisis";
        case CrisisState::CIVIL_UNREST:         return "Civil Unrest";
        case CrisisState::SUCCESSION_CRISIS:    return "Succession Crisis";
        case CrisisState::WAR_CRISIS:           return "War Crisis";
        case CrisisState::INSTITUTIONAL_CRISIS: return "Institutional Crisis";
        case CrisisState::MILITARY_TENSION:     return "Military Tension";
        case CrisisState::COUP_CRISIS:          return "Coup Crisis";
        case CrisisState::MILITARY_REGIME:      return "Military Regime";
    }
    return "Normal";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Realm Personality Archetypes & Permanent National Traits
// ─────────────────────────────────────────────────────────────────────────────
enum class RealmArchetype {
    MILITARISTIC,
    MERCHANT,
    SCIENTIFIC,
    DIPLOMATIC,
    EXPANSIONIST,
    ISOLATIONIST,
    AUTHORITARIAN,
    DEMOCRATIC,
    INDUSTRIAL,
    RELIGIOUS,
    DEFENSIVE,
    AGGRESSIVE
};

inline const char* realm_archetype_name(RealmArchetype a) {
    switch (a) {
        case RealmArchetype::MILITARISTIC:  return "Militaristic";
        case RealmArchetype::MERCHANT:      return "Merchant";
        case RealmArchetype::SCIENTIFIC:    return "Scientific";
        case RealmArchetype::DIPLOMATIC:    return "Diplomatic";
        case RealmArchetype::EXPANSIONIST:  return "Expansionist";
        case RealmArchetype::ISOLATIONIST:  return "Isolationist";
        case RealmArchetype::AUTHORITARIAN: return "Authoritarian";
        case RealmArchetype::DEMOCRATIC:    return "Democratic";
        case RealmArchetype::INDUSTRIAL:    return "Industrial";
        case RealmArchetype::RELIGIOUS:     return "Religious";
        case RealmArchetype::DEFENSIVE:     return "Defensive";
        case RealmArchetype::AGGRESSIVE:    return "Aggressive";
    }
    return "Balanced";
}

struct RealmPersonality {
    RealmArchetype archetype       = RealmArchetype::DIPLOMATIC;
    float military_pref            = 0.5f; // 0..1
    float economic_pref            = 0.5f; // 0..1
    float science_pref             = 0.5f; // 0..1
    float diplomacy_pref           = 0.5f; // 0..1
    float expansion_pref           = 0.5f; // 0..1
    float risk_tolerance           = 0.5f; // 0..1
    float aggression               = 0.5f; // 0..1
    float stability_pref           = 0.7f; // 0..1
    float alliance_pref            = 0.6f; // 0..1

    static RealmPersonality from_archetype(RealmArchetype arch) {
        RealmPersonality p;
        p.archetype = arch;
        switch (arch) {
            case RealmArchetype::MILITARISTIC:
                p.military_pref = 0.85f; p.aggression = 0.75f; p.expansion_pref = 0.70f; p.diplomacy_pref = 0.30f; break;
            case RealmArchetype::MERCHANT:
                p.economic_pref = 0.90f; p.diplomacy_pref = 0.75f; p.alliance_pref = 0.70f; p.aggression = 0.25f; break;
            case RealmArchetype::SCIENTIFIC:
                p.science_pref = 0.90f; p.economic_pref = 0.60f; p.stability_pref = 0.70f; p.aggression = 0.30f; break;
            case RealmArchetype::DIPLOMATIC:
                p.diplomacy_pref = 0.90f; p.alliance_pref = 0.85f; p.stability_pref = 0.80f; p.aggression = 0.20f; break;
            case RealmArchetype::EXPANSIONIST:
                p.expansion_pref = 0.90f; p.military_pref = 0.70f; p.risk_tolerance = 0.70f; p.aggression = 0.65f; break;
            case RealmArchetype::ISOLATIONIST:
                p.diplomacy_pref = 0.15f; p.expansion_pref = 0.20f; p.stability_pref = 0.85f; p.alliance_pref = 0.15f; break;
            case RealmArchetype::AUTHORITARIAN:
                p.stability_pref = 0.90f; p.military_pref = 0.75f; p.diplomacy_pref = 0.35f; p.aggression = 0.60f; break;
            case RealmArchetype::DEMOCRATIC:
                p.diplomacy_pref = 0.80f; p.alliance_pref = 0.80f; p.science_pref = 0.70f; p.stability_pref = 0.75f; break;
            case RealmArchetype::INDUSTRIAL:
                p.economic_pref = 0.85f; p.science_pref = 0.70f; p.military_pref = 0.60f; break;
            case RealmArchetype::RELIGIOUS:
                p.stability_pref = 0.80f; p.expansion_pref = 0.50f; p.risk_tolerance = 0.60f; break;
            case RealmArchetype::DEFENSIVE:
                p.military_pref = 0.70f; p.aggression = 0.20f; p.stability_pref = 0.90f; p.alliance_pref = 0.75f; break;
            case RealmArchetype::AGGRESSIVE:
                p.aggression = 0.90f; p.military_pref = 0.85f; p.risk_tolerance = 0.80f; p.diplomacy_pref = 0.20f; break;
        }
        return p;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  8-Dimensional Bilateral Diplomatic Relation & Interaction History
// ─────────────────────────────────────────────────────────────────────────────
struct DiplomaticInteractionRecord {
    std::string type; // WAR, PEACE, BETRAYAL, AID, TRADE, SANCTION, ESPIONAGE_DISCOVERED, INSULT, GIFT
    int         year = 0;
    std::string description;
    float       severity = 1.0f; // 0.1 minor -> 1.0 major
};

struct BilateralRelation {
    // 8 Core Emotional & Diplomatic Dimensions
    float trust              = 0.0f;   // -100.0 to +100.0 (dynamically changes)
    float fear               = 0.0f;   // 0.0 to 100.0 (based on relative military threat)
    float hatred             = 0.0f;   // 0.0 to 100.0 (historical grievances, wars)
    float respect            = 50.0f;  // 0.0 to 100.0 (culture, prestige, tech)
    float gratitude          = 0.0f;   // 0.0 to 100.0 (military aid, gifts, treaties)
    float suspicion          = 10.0f;  // 0.0 to 100.0 (espionage, broken treaties, military buildups)
    float diplomatic_debt    = 0.0f;   // -100.0 to +100.0 (favors owed / owed to)
    float rivalry            = 0.0f;   // 0.0 to 100.0 (competing for same territory/resources/hegemony)

    float military_threat    = 0.0f;   // 0.0 to 100.0 (border concentration)
    float trade_dependency   = 0.0f;   // 0.0 to 100.0 (economic intertwining)
    int   last_treaty_year   = -999;
    int   last_war_year      = -999;
    int   broken_treaties    = 0;

    std::vector<DiplomaticInteractionRecord> interaction_history;

    void add_interaction(const std::string& type, int year, const std::string& desc, float severity = 1.0f) {
        interaction_history.push_back({type, year, desc, severity});
        if (interaction_history.size() > 40) {
            interaction_history.erase(interaction_history.begin());
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  3-Tier Strategic Planning Layer
// ─────────────────────────────────────────────────────────────────────────────
struct StrategicPlan {
    // Immediate Goal (1-3 years): Tactical execution (e.g. Build Military, Sign Trade, Suppress Unrest)
    std::string immediate_goal = "MAINTAIN_STABILITY";
    int         immediate_target_civ = -1;
    int         immediate_plan_year_start = 0;
    int         immediate_plan_duration = 3;

    // Medium-Term Goal (5-15 years): Strategic milestone (e.g. Military Superiority, Economic Dominance)
    std::string medium_term_goal = "EXPAND_ECONOMIC_CAPACITY";
    int         medium_target_civ = -1;
    int         medium_plan_year_start = 0;
    int         medium_plan_duration = 10;

    // Long-Term Goal (20-50 years): Grand Doctrine (e.g. Hegemony, Pan-Continental Federation)
    std::string long_term_goal = "CONTINENTAL_HEGEMONY";
    int         long_term_plan_year_start = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Secret Strategic Goal
// ─────────────────────────────────────────────────────────────────────────────
struct SecretStrategicGoal {
    SecretGoalType type = SecretGoalType::MILITARY_HEGEMON;
    int target_civ_id = -1;
    std::string target_resource;
    float progress = 0.0f; // 0..100%
    bool is_revealed = false; // true if exposed via foreign counter-intelligence
};

// ─────────────────────────────────────────────────────────────────────────────
//  Historical Memory Rolling Window (20-30 years)
// ─────────────────────────────────────────────────────────────────────────────
struct ActionMemoryRecord {
    std::string action_type;
    int         target_civ = -1;
    int         year       = 0;
    float       utility    = 0.0f;
};

struct EventMemoryRecord {
    std::string event_type; // COUP, DICTATORSHIP, ELECTION, WAR, PEACE, REFORM, REVOLUTION
    int         year       = 0;
    std::string desc;
    int         target_civ = -1;
};

struct RealmHistoryMemory {
    std::vector<ActionMemoryRecord> recent_actions; // last 5-10 years
    std::vector<EventMemoryRecord>  recent_events;  // last 30 years
    int recent_coup_year       = -999;
    int recent_transition_year = -999;
    int recent_election_year   = -999;
    int recent_reform_year     = -999;
    int recent_defeat_year     = -999;
    int recent_victory_year    = -999;
    int recent_crisis_year     = -999;

    void add_action(const std::string& action, int target, int year, float util) {
        recent_actions.push_back({action, target, year, util});
        if (recent_actions.size() > 10) {
            recent_actions.erase(recent_actions.begin());
        }
    }

    void add_event(const std::string& type, int year, const std::string& desc, int target = -1) {
        recent_events.push_back({type, year, desc, target});
        if (recent_events.size() > 30) {
            recent_events.erase(recent_events.begin());
        }
        if (type == "MILITARY_COUP" || type == "COUP") recent_coup_year = year;
        if (type == "GOV_TRANSITION") recent_transition_year = year;
        if (type == "ELECTION") recent_election_year = year;
        if (type == "REFORM") recent_reform_year = year;
    }

    int count_consecutive_action(const std::string& action) const {
        int count = 0;
        for (auto it = recent_actions.rbegin(); it != recent_actions.rend(); ++it) {
            if (it->action_type == action) count++;
            else break;
        }
        return count;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Civilization  —  Full nation state
// ─────────────────────────────────────────────────────────────────────────────
struct AeonCivilization {
    int         id          = 0;
    std::string name;           // e.g. "NORDRA"
    std::string adjective;      // e.g. "Nordran"
    std::string color_code;     // terminal ANSI color
    char        map_char    = 'N'; // single char territory marker

    // State
    GovForm         government  = GovForm::MONARCHY;
    CrisisState     crisis_state= CrisisState::NORMAL;
    PopulationState population;
    ResourceStock   resources;
    EconomyState    economy;
    TechState       tech;

    // People
    int             ruler_id    = -1;   // ID into characters list
    std::vector<int> character_ids;

    // Geography & Regions
    int capital_x = 0, capital_y = 0;
    std::vector<int> city_ids;
    std::vector<Province> provinces;    // Subdivided territory
    float territory_tiles = 0.0f;

    // Stability & Power
    float stability = 80.0f;    // 0-100
    float unrest    = 20.0f;    // 0-100; high unrest creates civil conflict / revolutions
    float military_power = 500.0f;
    float army_size = 5000.0f;
    float standing_army = 5000.0f;      // Peacetime standing active forces
    float reserve_pool  = 15000.0f;     // Mobilizable population reserves
    float mobilization_level = 0.0f;    // 0.0 (peacetime) to 1.0 (total war mobilization)
    float morale    = 75.0f;
    float is_alive  = 1.0f;     // 0.0 = extinct

    // Political Institutions & State Governance
    float ruler_authority               = 70.0f; // 0-100; concentration of executive power
    float legitimacy                    = 80.0f; // 0-100; recognized right to rule
    float military_loyalty              = 85.0f; // 0-100; armed forces loyalty to head of state
    float elite_support                 = 75.0f; // 0-100; nobility / oligarch / corporate loyalty
    float public_support                = 70.0f; // 0-100; populace approval
    float opposition_strength           = 20.0f; // 0-100; underground/dissident faction strength
    float democratic_institution_strength = 50.0f; // 0-100; judicial/legislative checks & balances
    float corruption                    = 15.0f; // 0-100; graft, bribery, embezzlement

    int   years_in_power                = 0;
    int   years_current_gov             = 0;
    int   last_transition_year          = -999;
    int   recent_coup_year              = -999;
    int   coup_attempt_year             = -999;  // Last attempt year (even if failed)
    bool  under_martial_law             = false;
    bool  emergency_powers_active       = false;

    // ── Military Political Metrics (separate from loyalty to ruler) ──────────
    float military_discontent = 0.0f; // 0-100: rank-and-file unhappiness (drives coup)
    float coup_support        = 0.0f; // 0-100: % of military actively backing a coup

    // History & Memory
    RealmHistoryMemory history_memory;

    // Extended stability and cultural fields
    float war_exhaustion        = 0.0f;  // 0-100; high = forced peace
    float cultural_prestige     = 30.0f; // 0-100; boosts diplomacy & soft power
    float soft_power            = 20.0f; // 0-100; from art, wonders, culture
    float carbon_output         = 0.0f;  // CO2 contributed per year
    bool  is_reserve_currency_holder = false;
    float luddite_resistance    = 0.0f;  // 0-100; faction resistance to rapid tech
    float global_co2_ppm        = 280.0f;// world-shared CO2 concentration (ppm)

    // ── Permanent Realm Personality (Independent of ruler) ───────────────────
    RealmPersonality national_personality;

    // AI personality preferences (dynamically modulated by ruler)
    float aggression     = 0.5f;
    float diplomacy_pref = 0.5f;
    float science_pref   = 0.4f;
    float trade_pref     = 0.6f;
    float expansion_pref = 0.5f;

    // Strategic Planning & Secret Goals
    StrategicPlan       strategic_plan;
    SecretStrategicGoal secret_goal;

    // Diplomacy relations [other_civ_id -> status]
    std::unordered_map<int, DiplomacyStatus> relations;
    // Multi-dimensional diplomatic relation matrix [other_civ_id -> BilateralRelation]
    std::unordered_map<int, BilateralRelation> bilateral_relations;
    // Legacy Memory of grievances [other_civ_id -> trust -100..+100]
    std::unordered_map<int, float> trust_memory;

    // Active bilateral trade agreements [partner_civ_id -> years_remaining]
    std::unordered_map<int, int> active_trade_agreements;

    // Goals
    std::string primary_goal;
    std::string hidden_goal;

    // Culture & Traditions
    std::string  religion_name;
    int          religion_id  = -1;
    float        religiosity  = 0.4f;
    std::string  culture_desc;
    std::string  ideology;      // Militarism, Mercantilism, Scientism...
    std::string  cultural_values;
    std::string  military_traditions;
    std::string  scientific_traditions;

    // Internal Factions (9 Types)
    std::vector<InternalFaction> factions;

    // Trade & Economy
    std::vector<int> trade_route_ids;
    std::vector<int> embargoed_civs;

    // Active war state & War Objectives
    bool                          at_war        = false;
    int                           war_with_civ  = -1;
    int                           war_year_start = -1;
    std::vector<WarObjectiveType> war_objectives;

    bool         is_commons    = false; // The Commons region

    void tick_year(int year, float dt_years);
    void init_default_factions();
    bool check_civil_war(int year);

    // Carrying capacity helper
    long long get_carrying_capacity() const;
};

} // namespace Aeon
