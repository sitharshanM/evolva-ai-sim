#pragma once
#include "aeon_config.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Aeon {

// ─── Business Cycle ───────────────────────────────────────────────────────────
enum class BusinessCycle { BOOM, RECOVERY, STAGNATION, RECESSION, DEPRESSION };

inline const char* business_cycle_name(BusinessCycle b) {
    switch (b) {
        case BusinessCycle::BOOM:        return "Boom";
        case BusinessCycle::RECOVERY:    return "Recovery";
        case BusinessCycle::STAGNATION:  return "Stagnation";
        case BusinessCycle::RECESSION:   return "Recession";
        case BusinessCycle::DEPRESSION:  return "Depression";
    }
    return "Unknown";
}

// ─── Resources ────────────────────────────────────────────────────────────────
// ─── Resources ────────────────────────────────────────────────────────────────
struct ResourceStock {
    float food        = 500.0f;
    float wood        = 300.0f;
    float stone       = 300.0f;
    float iron        = 150.0f;
    float copper      = 100.0f;
    float coal        = 80.0f;
    float oil         = 50.0f;
    float gas         = 20.0f;
    float rare        = 10.0f; // Legacy compatibility
    float uranium     = 0.0f;
    float rare_earths = 10.0f;
    float water       = 500.0f;
    float luxury      = 25.0f;
    float energy      = 50.0f;
    float knowledge   = 10.0f;
    float tech        = 0.0f;
    // High-tech resources
    float silicon     = 0.0f;
    float lithium     = 0.0f;
    float helium3     = 0.0f;
};

// ─── Economy ──────────────────────────────────────────────────────────────────
struct EconomyState {
    float gdp             = 1000.0f;
    float gdp_growth      = 0.03f;    // 3% per year
    float inflation       = 0.02f;
    float debt            = 0.0f;
    float wealth_inequality = 0.35f;  // Gini coefficient 0-1
    float unemployment    = 0.05f;

    float tax_rate        = 0.20f;    // 20%
    float annual_income   = 200.0f;

    // Business cycle
    BusinessCycle business_cycle  = BusinessCycle::RECOVERY;
    float credit_expansion        = 0.0f;   // 0-1, drives boom/bust
    float asset_bubble_risk       = 0.0f;   // 0-1
    float reserve_currency_held   = 0.0f;   // gold value of foreign reserves held
    float manufacturing_output    = 100.0f; // base output; affected by Dutch disease
    float stranded_asset_cost     = 0.0f;   // ongoing maintenance of obsolete tech

    // Market prices for each resource (gold per unit)
    std::unordered_map<std::string, float> market_prices;
};

// ─── Population ───────────────────────────────────────────────────────────────
struct PopulationState {
    long long total         = 500000LL;
    float     birth_rate    = 0.025f;  // per year
    float     death_rate    = 0.015f;
    float     growth_rate   = 0.010f;
    float     happiness     = 70.0f;   // 0-100
    float     health        = 75.0f;
    float     education_lvl = 30.0f;
    float     food_security = 80.0f;
    float     housing_fill  = 85.0f;
    float     employment    = 95.0f;
    int       num_cities    = 1;
    // Demographic depth
    float     urban_pct         = 20.0f;  // 0-100% living in cities
    float     rural_pct         = 80.0f;
    float     ethnic_tension    = 0.0f;   // 0-100; drives separatism
    float     median_age        = 28.0f;
    float     infant_mortality  = 50.0f;  // per 1000 births; falls with tech
    long long urban_population  = 100000LL;
    long long rural_population  = 400000LL;
    long long working_count     = 300000LL; // age 19-64
    long long youth_count       = 150000LL; // age 0-18
    long long senior_count      = 50000LL;  // age 65+
};

// ─── Technology ───────────────────────────────────────────────────────────────
enum class TechEra {
    SURVIVAL, AGRICULTURE, METALLURGY, WRITING,
    ENGINEERING, SCIENCE, INDUSTRIALIZATION,
    ELECTRICITY, COMPUTING, MODERN, ADVANCED
};

inline const char* tech_era_name(TechEra t) {
    switch (t) {
        case TechEra::SURVIVAL:          return "Survival";
        case TechEra::AGRICULTURE:       return "Agriculture";
        case TechEra::METALLURGY:        return "Metallurgy";
        case TechEra::WRITING:           return "Writing & Records";
        case TechEra::ENGINEERING:       return "Engineering";
        case TechEra::SCIENCE:           return "Science";
        case TechEra::INDUSTRIALIZATION: return "Industrialization";
        case TechEra::ELECTRICITY:       return "Electricity";
        case TechEra::COMPUTING:         return "Computing";
        case TechEra::MODERN:            return "Modern Technology";
        case TechEra::ADVANCED:          return "Advanced Technology";
    }
    return "Unknown";
}

enum class TechSpecialization { NONE, MILITARY, AGRICULTURE, SCIENCE, TRADE };

inline const char* tech_spec_name(TechSpecialization s) {
    switch (s) {
        case TechSpecialization::NONE:        return "None";
        case TechSpecialization::MILITARY:    return "Military";
        case TechSpecialization::AGRICULTURE: return "Agriculture";
        case TechSpecialization::SCIENCE:     return "Science";
        case TechSpecialization::TRADE:       return "Trade";
    }
    return "None";
}

struct TechState {
    TechEra era           = TechEra::AGRICULTURE;
    float   research_pts  = 0.0f;    // accumulates each year
    float   progress      = 0.0f;    // 0-100% to next era
    bool    lost_tech     = false;   // dark age regression
    // Specialization & parallel research
    TechSpecialization specialization = TechSpecialization::NONE;
    std::string parallel_research_node;          // second node being researched
    float       parallel_research_pts  = 0.0f;   // progress on secondary node
    std::vector<std::string> obsolete_techs;     // techs that became liabilities
    std::vector<std::string> unlocked_nodes;     // completed tech node IDs
    int         last_era_advance_year = 0;       // for luddite resistance calc
};

// ─── Government ───────────────────────────────────────────────────────────────
enum class GovForm {
    TRIBAL, CHIEFDOM, MONARCHY, REPUBLIC, DEMOCRACY,
    OLIGARCHY, MILITARY_JUNTA, DICTATORSHIP, THEOCRACY,
    EMPIRE, FEDERATION, TECHNOCRACY, AI_COUNCIL
};

inline const char* gov_form_name(GovForm g) {
    switch (g) {
        case GovForm::TRIBAL:         return "Tribal Confederation";
        case GovForm::CHIEFDOM:       return "Chiefdom";
        case GovForm::MONARCHY:       return "Monarchy";
        case GovForm::REPUBLIC:       return "Republic";
        case GovForm::DEMOCRACY:      return "Democracy";
        case GovForm::OLIGARCHY:      return "Oligarchy";
        case GovForm::MILITARY_JUNTA: return "Military Junta";
        case GovForm::DICTATORSHIP:   return "Dictatorship";
        case GovForm::THEOCRACY:      return "Theocracy";
        case GovForm::EMPIRE:         return "Empire";
        case GovForm::FEDERATION:     return "Federation";
        case GovForm::TECHNOCRACY:    return "Technocracy";
        case GovForm::AI_COUNCIL:     return "AI Governance Council";
    }
    return "Unknown";
}

// ─── Diplomatic Status ────────────────────────────────────────────────────────
enum class DiplomacyStatus {
    NEUTRAL, TRADE_PARTNER, ALLY, DEFENSIVE_PACT,
    RIVAL, HOSTILE, AT_WAR, VASSAL, OVERLORD, EMBARGOED, PUPPET_STATE
};

inline const char* diplomacy_status_name(DiplomacyStatus d) {
    switch (d) {
        case DiplomacyStatus::NEUTRAL:        return "Neutral";
        case DiplomacyStatus::TRADE_PARTNER:  return "Trade Partner";
        case DiplomacyStatus::ALLY:           return "Allied";
        case DiplomacyStatus::DEFENSIVE_PACT: return "Defensive Pact";
        case DiplomacyStatus::RIVAL:          return "Rival";
        case DiplomacyStatus::HOSTILE:        return "Hostile";
        case DiplomacyStatus::AT_WAR:         return "AT WAR";
        case DiplomacyStatus::VASSAL:         return "Vassal";
        case DiplomacyStatus::OVERLORD:       return "Overlord";
        case DiplomacyStatus::EMBARGOED:      return "Sanctioned & Embargoed";
        case DiplomacyStatus::PUPPET_STATE:   return "Puppet Regime";
    }
    return "Neutral";
}

// ─── Alliance Types ───────────────────────────────────────────────────────────
enum class AllianceType {
    DEFENSIVE_PACT,
    MILITARY_ALLIANCE,
    RESEARCH_PACT,
    ECONOMIC_UNION,
    FEDERATION,
    NON_AGGRESSION_PACT,
    MILITARY_ACCESS
};

inline const char* alliance_type_name(AllianceType a) {
    switch (a) {
        case AllianceType::DEFENSIVE_PACT:     return "Defensive Pact";
        case AllianceType::MILITARY_ALLIANCE:  return "Military Alliance";
        case AllianceType::RESEARCH_PACT:      return "Research Pact";
        case AllianceType::ECONOMIC_UNION:     return "Economic Union";
        case AllianceType::FEDERATION:         return "Federation";
        case AllianceType::NON_AGGRESSION_PACT:return "Non-Aggression Pact";
        case AllianceType::MILITARY_ACCESS:    return "Military Access Agreement";
    }
    return "Pact";
}

// ─── Internal Factions (9 Types) ──────────────────────────────────────────────
enum class FactionType {
    MILITARY_NOBLES, // Legacy compat: Military
    MERCHANT_GUILD,  // Legacy compat: Merchants
    RELIGIOUS_CLERGY,// Legacy compat: Religious
    POPULAR_FRONT,   // Legacy compat: Commoners
    NOBILITY,        // Aristocrats / Old Elites
    SCIENTISTS,      // Technocrats / Academy
    REFORMISTS,      // Institutional reformers
    AUTHORITARIANS,  // Centralizers / Secret Police
    INDUSTRIALISTS   // Factory owners / Capitalists
};

inline const char* faction_type_name(FactionType f) {
    switch (f) {
        case FactionType::MILITARY_NOBLES:  return "Military Command";
        case FactionType::MERCHANT_GUILD:   return "Merchants & Traders";
        case FactionType::RELIGIOUS_CLERGY: return "Religious Clergy";
        case FactionType::POPULAR_FRONT:    return "Commoners & Labor";
        case FactionType::NOBILITY:         return "Nobility & Aristocrats";
        case FactionType::SCIENTISTS:       return "Scientists & Scholars";
        case FactionType::REFORMISTS:       return "Reformists";
        case FactionType::AUTHORITARIANS:   return "Authoritarian Elites";
        case FactionType::INDUSTRIALISTS:   return "Industrialists & Miners";
    }
    return "Faction";
}

struct InternalFaction {
    FactionType type             = FactionType::MILITARY_NOBLES;
    std::string name;
    float       influence        = 30.0f; // 0-100% clout in empire
    float       loyalty          = 70.0f; // 0-100% loyalty to ruler
    float       rebellion_risk   = 0.0f;  // 0-100% risk of civil war
    float       wealth           = 50.0f; // 0-100% economic resources
    float       popularity       = 50.0f; // 0-100% public appeal
    float       military_support = 20.0f; // 0-100% fraction of armed forces backing this faction
    float       political_power  = 30.0f; // 0-100% institutional leverage
    float       satisfaction     = 70.0f; // 0-100% happiness with government policy
};

// ─── Provinces & Regions ──────────────────────────────────────────────────────
struct Province {
    int         id                = 0;
    std::string name;
    int         civ_id            = -1;
    long long   population        = 100000LL;
    float       gdp               = 200.0f;
    float       stability         = 75.0f;   // 0-100
    float       unrest            = 15.0f;   // 0-100
    float       loyalty           = 80.0f;   // 0-100 to central government
    float       autonomy_demand   = 0.0f;    // 0-100 desire for self-rule / independence
    float       infrastructure    = 50.0f;   // 0-100
    float       military_presence = 1000.0f; // garrison troops
    std::string dominant_culture;
    ResourceStock resource_yield;
    bool        in_rebellion      = false;
};

// ─── War Objectives ───────────────────────────────────────────────────────────
enum class WarObjectiveType {
    CONQUER_TERRITORY,
    CAPTURE_CAPITAL,
    DESTROY_MILITARY,
    SEIZE_RESOURCES,
    FORCE_TRIBUTE,
    LIBERATE_REGION,
    CHANGE_GOVERNMENT,
    PUNISH_BETRAYAL,
    STEAL_TECHNOLOGY,
    FORCE_CONCESSIONS
};

inline const char* war_objective_name(WarObjectiveType o) {
    switch (o) {
        case WarObjectiveType::CONQUER_TERRITORY: return "Conquer Border Territory";
        case WarObjectiveType::CAPTURE_CAPITAL:    return "Capture Enemy Capital";
        case WarObjectiveType::DESTROY_MILITARY:   return "Decimate Armed Forces";
        case WarObjectiveType::SEIZE_RESOURCES:    return "Seize Strategic Resources";
        case WarObjectiveType::FORCE_TRIBUTE:      return "Force Annual Tribute";
        case WarObjectiveType::LIBERATE_REGION:    return "Liberate Occupied Region";
        case WarObjectiveType::CHANGE_GOVERNMENT:  return "Enforce Regime Change";
        case WarObjectiveType::PUNISH_BETRAYAL:    return "Punish Treaty Betrayal";
        case WarObjectiveType::STEAL_TECHNOLOGY:   return "Extract Advanced Technology";
        case WarObjectiveType::FORCE_CONCESSIONS:  return "Force Diplomatic Concessions";
    }
    return "Conquest";
}

// ─── Covert Espionage Missions ────────────────────────────────────────────────
enum class EspionageMissionType {
    SPY,
    STEAL_TECHNOLOGY,
    SABOTAGE,
    INFILTRATE_MILITARY,
    FUND_REBELS,
    STEAL_RESOURCES,
    COUNTER_INTELLIGENCE,
    DISCOVER_SECRET
};

inline const char* espionage_mission_name(EspionageMissionType m) {
    switch (m) {
        case EspionageMissionType::SPY:                  return "Gather Intelligence";
        case EspionageMissionType::STEAL_TECHNOLOGY:     return "Steal Technological Blueprint";
        case EspionageMissionType::SABOTAGE:             return "Sabotage Infrastructure";
        case EspionageMissionType::INFILTRATE_MILITARY:  return "Infiltrate Military High Command";
        case EspionageMissionType::FUND_REBELS:          return "Secretly Fund Insurgent Factions";
        case EspionageMissionType::STEAL_RESOURCES:      return "Covertly Siphon Treasury & Resources";
        case EspionageMissionType::COUNTER_INTELLIGENCE: return "Counter-Intelligence Sweep";
        case EspionageMissionType::DISCOVER_SECRET:      return "Expose Secret Geopolitical Agenda";
    }
    return "Covert Operation";
}

// ─── Secret Strategic Goals ───────────────────────────────────────────────────
enum class SecretGoalType {
    DESTROY_RIVAL,
    DOMINATE_RESOURCE,
    TECH_LEADER,
    CONTROL_TRADE_ROUTE,
    CREATE_FEDERATION,
    MILITARY_HEGEMON,
    PREVENT_HEGEMON,
    AVENGE_DEFEAT,
    INCITE_FOREIGN_CIVIL_WAR
};

inline const char* secret_goal_name(SecretGoalType g) {
    switch (g) {
        case SecretGoalType::DESTROY_RIVAL:           return "Secretly Destroy Primary Rival";
        case SecretGoalType::DOMINATE_RESOURCE:       return "Monopolize Strategic Resource";
        case SecretGoalType::TECH_LEADER:             return "Secret Technological Superiority";
        case SecretGoalType::CONTROL_TRADE_ROUTE:     return "Control Global Trade Arteries";
        case SecretGoalType::CREATE_FEDERATION:       return "Form Pan-Continental Federation";
        case SecretGoalType::MILITARY_HEGEMON:        return "Covert Military Hegemony";
        case SecretGoalType::PREVENT_HEGEMON:         return "Contain Rising Regional Superpower";
        case SecretGoalType::AVENGE_DEFEAT:           return "Avenge Historic Humiliation";
        case SecretGoalType::INCITE_FOREIGN_CIVIL_WAR:return "Incite Civil War in Neighboring Realm";
    }
    return "Geopolitical Dominance";
}

// ─── Emergent Religion ────────────────────────────────────────────────────────
struct Religion {
    int         id          = 0;
    std::string name;
    std::string holy_city;
    int         founder_civ = -1;
    int         founded_year = 0;
    float       fervor      = 50.0f; // 0-100 zealotry
    long long   followers   = 10000LL;
    bool        holy_war_active = false;
    // Reformation & schism
    int         sect_count       = 1;
    bool        has_reformed     = false;
    int         reformation_year = -1;
    float       tithe_rate       = 0.05f;  // fraction of civ GDP paid as tithe
    float       secular_erosion_rate = 0.0f; // grows near SCIENCE-era civs
    float       schism_risk      = 0.0f;   // 0-100
    std::vector<std::string> relic_powers;         // bonus descriptions
    std::vector<int>         missionary_route_ids; // active trade route IDs
};

// ─── Religious Schism ─────────────────────────────────────────────────────────
struct ReligiousSchism {
    int         parent_religion_id = -1;
    int         new_religion_id    = -1;
    int         year               = 0;
    std::string doctrine_dispute;
};

// ─── Tithe Record ─────────────────────────────────────────────────────────────
struct TitheRecord {
    int   religion_id  = -1;
    int   paying_civ_id = -1;
    float annual_gold  = 0.0f;
};

// ─── Trade Route ──────────────────────────────────────────────────────────────
struct TradeRoute {
    int         id          = 0;
    int         civ_a       = -1;
    int         civ_b       = -1;
    std::string origin_city;
    std::string dest_city;
    std::string primary_good;
    float       annual_volume    = 100.0f; // gold value traded per year
    bool        active           = true;
    // Realism additions
    float       distance_tiles   = 10.0f;  // map tile distance between endpoints
    float       terrain_cost_mult = 1.0f;  // 1.0 = flat, 2.0 = mountain crossing
    float       insurance_premium = 0.02f; // fraction of cargo value as insurance
    float       effective_income  = 100.0f;// after distance/terrain deduction
};

// ─── Sovereign Debt Crisis ────────────────────────────────────────────────────
struct SovereignDebtCrisis {
    int   civ_id            = -1;
    int   creditor_civ_id   = -1;
    float demanded_concession = 0.0f; // stability% the creditor demands
    int   year_started       = 0;
    bool  resolved           = false;
};

} // namespace Aeon

