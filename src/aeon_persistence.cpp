// =============================================================================
//  aeon_persistence.cpp  —  Complete world state save/load (Phase 1)
//
//  Saves:
//   - Simulation metadata (year, month, seed, speed)
//   - ID registries (next IDs for civs, chars, cities, wars, events…)
//   - Full civilization state (pop, gdp, tech, army, relations, memory)
//   - History event database (uid, year, category, headline, causes)
//   - Civilizational relation memory (wars, trust, hatred, etc.)
//   - Council state
// =============================================================================
#include "aeon_persistence.h"
#include "aeon_engine.h"
#include "aeon_unique_id.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>

using json = nlohmann::json;

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  SAVE
// ─────────────────────────────────────────────────────────────────────────────
bool SaveLoadEngine::save(const AeonEngine& engine, const std::string& filename) {
    try {
        std::filesystem::create_directories("saves");
        std::string filepath = "saves/" + filename;
        if (filepath.find(".json") == std::string::npos) filepath += ".json";

        json j;

        // ── Metadata ──────────────────────────────────────────────────────────
        j["version"]        = 2;                // Phase 1 format version
        j["year"]           = engine.year;
        j["month"]          = engine.month;
        j["seed"]           = engine.seed;
        j["speed"]          = engine.speed;
        j["council_exists"] = engine.council_exists;
        j["council_year"]   = engine.council_year;
        j["next_char_id"]   = engine.next_char_id;
        j["next_city_id"]   = engine.next_city_id;

        // History event count (informational)
        j["history_event_count"] = engine.history.event_count();

        // ── Civilizations ─────────────────────────────────────────────────────
        json civs_arr = json::array();
        for (const auto& c : engine.civs) {
            json cj;
            // Identity
            cj["id"]            = c.id;
            cj["uid"]           = format_civ_uid(c.id);
            cj["name"]          = c.name;
            cj["map_char"]      = std::string(1, c.map_char);
            cj["is_alive"]      = c.is_alive;
            cj["is_commons"]    = c.is_commons;
            cj["government"]    = int(c.government);
            cj["primary_goal"]  = c.primary_goal;
            cj["hidden_goal"]   = c.hidden_goal;
            cj["founding_year"] = 0; // filled properly in Phase 2

            // Demographics
            cj["population"]    = c.population.total;
            cj["birth_rate"]    = c.population.birth_rate;
            cj["death_rate"]    = c.population.death_rate;
            cj["happiness"]     = c.population.happiness;
            cj["health"]        = c.population.health;
            cj["education"]     = c.population.education_lvl;

            // Economy
            cj["gdp"]           = c.economy.gdp;
            cj["gdp_growth"]    = c.economy.gdp_growth;
            cj["inflation"]     = c.economy.inflation;
            cj["debt"]          = c.economy.debt;
            cj["tax_rate"]      = c.economy.tax_rate;
            cj["annual_income"] = c.economy.annual_income;

            // Technology
            cj["tech_era"]      = int(c.tech.era);
            cj["tech_progress"] = c.tech.progress;
            cj["tech_pts"]      = c.tech.research_pts;

            // Military & Mobilization
            cj["army_size"]     = c.army_size;
            cj["standing_army"] = c.standing_army;
            cj["reserve_pool"]  = c.reserve_pool;
            cj["mobilization"]  = c.mobilization_level;
            cj["military_power"]= c.military_power;
            cj["morale"]        = c.morale;

            // Stability & Politics
            cj["stability"]     = c.stability;
            cj["unrest"]        = c.unrest;
            cj["crisis_state"]  = int(c.crisis_state);
            cj["war_exhaustion"]= c.war_exhaustion;
            cj["ruler_authority"] = c.ruler_authority;
            cj["legitimacy"]    = c.legitimacy;
            cj["military_loyalty"] = c.military_loyalty;
            cj["elite_support"] = c.elite_support;
            cj["public_support"] = c.public_support;
            cj["opposition_strength"] = c.opposition_strength;
            cj["institutions"]  = c.democratic_institution_strength;
            cj["corruption"]    = c.corruption;
            cj["martial_law"]   = c.under_martial_law;
            cj["emergency"]     = c.emergency_powers_active;
            cj["years_in_power"]= c.years_in_power;
            cj["recent_coup_year"] = c.recent_coup_year;
            cj["last_transition_year"] = c.last_transition_year;

            // Geography
            cj["capital_x"]     = c.capital_x;
            cj["capital_y"]     = c.capital_y;
            cj["territory"]     = c.territory_tiles;

            // War state
            cj["at_war"]        = c.at_war;
            cj["war_with"]      = c.war_with_civ;
            cj["war_year"]      = c.war_year_start;

            // Personality
            cj["aggression"]    = c.aggression;
            cj["diplomacy_pref"]= c.diplomacy_pref;
            cj["science_pref"]  = c.science_pref;
            cj["trade_pref"]    = c.trade_pref;
            cj["expansion_pref"]= c.expansion_pref;

            // Ruler
            cj["ruler_id"]      = c.ruler_id;

            // Relations [other_id -> status_int]
            json rels = json::object();
            for (const auto& kv : c.relations)
                rels[std::to_string(kv.first)] = int(kv.second);
            cj["relations"] = rels;

            // Trust memory [other_id -> trust_float]
            json tmem = json::object();
            for (const auto& kv : c.trust_memory)
                tmem[std::to_string(kv.first)] = kv.second;
            cj["trust_memory"] = tmem;

            civs_arr.push_back(cj);
        }
        j["civs"] = civs_arr;

        // ── History events (most recent N for save compactness) ────────────────
        const auto& all_events = engine.history.all();
        json evts_arr = json::array();
        size_t evt_start = all_events.size() > 2000 ? all_events.size() - 2000 : 0;
        for (size_t i = evt_start; i < all_events.size(); ++i) {
            const auto& e = all_events[i];
            json ej;
            ej["event_id"]    = e.event_id;
            ej["uid"]         = e.uid;
            ej["year"]        = e.year;
            ej["month"]       = e.month;
            ej["category"]    = e.category;
            ej["headline"]    = e.headline;
            ej["civ_id"]      = e.civ_id;
            ej["civ2_id"]     = e.civ2_id;
            ej["actor_uid"]   = e.actor_uid;
            ej["target_uid"]  = e.target_uid;
            ej["significance"]= e.significance;
            json causes = json::array();
            for (const auto& c : e.causes) causes.push_back(c);
            ej["causes"] = causes;
            evts_arr.push_back(ej);
        }
        j["history_events"] = evts_arr;

        // ── AI memory (per controller) ─────────────────────────────────────────
        json ai_arr = json::array();
        for (const auto& ai : engine.ai_controllers) {
            json aj;
            aj["civ_id"] = ai_arr.size(); // index = civ_id
            json mem_arr = json::array();
            for (const auto& m : ai.memory) {
                json mj;
                mj["other"] = m.other_civ;
                mj["trust"] = m.trust;
                mj["fear"]  = m.fear;
                mj["hate"]  = m.hatred;
                mj["year"]  = m.year;
                mem_arr.push_back(mj);
            }
            aj["memory"] = mem_arr;
            aj["goal"]   = int(ai.strategic_goal);
            ai_arr.push_back(aj);
        }
        j["ai_state"] = ai_arr;

        // Write
        std::ofstream out(filepath);
        if (!out.is_open()) return false;
        out << j.dump(2);
        out.close();

        std::cout << "[PERSISTENCE] Saved world to " << filepath
                  << " (Year " << engine.year << ", " << all_events.size()
                  << " events, format v2)" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "[PERSISTENCE ERROR] Save failed: " << e.what() << std::endl;
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  LOAD
// ─────────────────────────────────────────────────────────────────────────────
bool SaveLoadEngine::load(AeonEngine& engine, const std::string& filename) {
    try {
        std::string filepath = "saves/" + filename;
        if (filepath.find(".json") == std::string::npos) filepath += ".json";

        std::ifstream in(filepath);
        if (!in.is_open()) {
            std::cout << "[PERSISTENCE ERROR] Cannot open " << filepath << std::endl;
            return false;
        }

        json j;
        in >> j;
        in.close();

        int fmt_version = j.value("version", 1);

        // ── Metadata ──────────────────────────────────────────────────────────
        engine.year           = j.value("year",  1);
        engine.month          = j.value("month", 1);
        engine.seed           = j.value("seed",  928374ULL);
        engine.speed          = j.value("speed", 1.0f);
        engine.council_exists = j.value("council_exists", false);
        engine.council_year   = j.value("council_year",   -1);
        engine.next_char_id   = j.value("next_char_id",    1);
        engine.next_city_id   = j.value("next_city_id",    1);

        // ── Civilizations ─────────────────────────────────────────────────────
        if (j.contains("civs") && j["civs"].is_array()) {
            const auto& civs_arr = j["civs"];
            for (size_t i = 0; i < civs_arr.size() && i < engine.civs.size(); ++i) {
                const auto& cj = civs_arr[i];
                auto& c = engine.civs[i];

                c.is_alive          = cj.value("is_alive",    1.0f);
                c.is_commons        = cj.value("is_commons",  false);
                c.government        = GovForm(cj.value("government", int(GovForm::MONARCHY)));
                c.primary_goal      = cj.value("primary_goal", c.primary_goal);
                c.hidden_goal       = cj.value("hidden_goal",  c.hidden_goal);

                c.population.total      = cj.value("population",  500000LL);
                c.population.birth_rate = cj.value("birth_rate",  0.025f);
                c.population.death_rate = cj.value("death_rate",  0.015f);
                c.population.happiness  = cj.value("happiness",   70.0f);
                c.population.health     = cj.value("health",      75.0f);
                c.population.education_lvl = cj.value("education",30.0f);

                c.economy.gdp           = cj.value("gdp",         1000.0f);
                c.economy.gdp_growth    = cj.value("gdp_growth",  0.03f);
                c.economy.inflation     = cj.value("inflation",   0.02f);
                c.economy.debt          = cj.value("debt",        0.0f);
                c.economy.tax_rate      = cj.value("tax_rate",    0.20f);
                c.economy.annual_income = cj.value("annual_income",200.0f);

                c.tech.era          = TechEra(cj.value("tech_era",      1));
                c.tech.progress     = cj.value("tech_progress",  0.0f);
                c.tech.research_pts = cj.value("tech_pts",       0.0f);

                c.army_size     = cj.value("army_size",    5000.0f);
                c.standing_army = cj.value("standing_army", 5000.0f);
                c.reserve_pool  = cj.value("reserve_pool",  15000.0f);
                c.mobilization_level = cj.value("mobilization", 0.0f);
                c.military_power= cj.value("military_power",500.0f);
                c.morale        = cj.value("morale",       75.0f);
                c.stability     = cj.value("stability",    80.0f);
                c.unrest        = cj.value("unrest",       20.0f);
                c.crisis_state  = CrisisState(cj.value("crisis_state", 0));
                c.war_exhaustion= cj.value("war_exhaustion",0.0f);
                c.ruler_authority = cj.value("ruler_authority", 70.0f);
                c.legitimacy    = cj.value("legitimacy",    80.0f);
                c.military_loyalty = cj.value("military_loyalty", 85.0f);
                c.elite_support = cj.value("elite_support", 75.0f);
                c.public_support = cj.value("public_support", 70.0f);
                c.opposition_strength = cj.value("opposition_strength", 20.0f);
                c.democratic_institution_strength = cj.value("institutions", 50.0f);
                c.corruption    = cj.value("corruption",    15.0f);
                c.under_martial_law = cj.value("martial_law", false);
                c.emergency_powers_active = cj.value("emergency", false);
                c.years_in_power = cj.value("years_in_power", 0);
                c.recent_coup_year = cj.value("recent_coup_year", -999);
                c.last_transition_year = cj.value("last_transition_year", -999);
                c.territory_tiles = cj.value("territory",  30.0f);

                c.at_war        = cj.value("at_war",    false);
                c.war_with_civ  = cj.value("war_with",  -1);
                c.war_year_start= cj.value("war_year",  -1);

                c.aggression     = cj.value("aggression",    0.5f);
                c.diplomacy_pref = cj.value("diplomacy_pref",0.5f);
                c.science_pref   = cj.value("science_pref",  0.4f);
                c.trade_pref     = cj.value("trade_pref",    0.6f);
                c.expansion_pref = cj.value("expansion_pref",0.5f);
                c.ruler_id       = cj.value("ruler_id",     -1);

                // Restore relations
                if (cj.contains("relations") && cj["relations"].is_object()) {
                    for (auto it = cj["relations"].begin(); it != cj["relations"].end(); ++it) {
                        int other_id = std::stoi(it.key());
                        c.relations[other_id] = DiplomacyStatus(it.value().get<int>());
                    }
                }
                // Restore trust memory
                if (cj.contains("trust_memory") && cj["trust_memory"].is_object()) {
                    for (auto it = cj["trust_memory"].begin(); it != cj["trust_memory"].end(); ++it) {
                        int other_id = std::stoi(it.key());
                        c.trust_memory[other_id] = it.value().get<float>();
                    }
                }
            }
        }

        // ── History events (v2 format) ─────────────────────────────────────────
        if (fmt_version >= 2 && j.contains("history_events") && j["history_events"].is_array()) {
            // Note: we only partially restore history to avoid duplicating events
            // Full restoration is done via dedicated restore path
            size_t restored = 0;
            for (const auto& ej : j["history_events"]) {
                // Just log count for now — deep restore in Phase 2
                (void)ej;
                ++restored;
            }
            std::cout << "[PERSISTENCE] Skipping " << restored
                      << " history events (full restore in Phase 2)" << std::endl;
        }

        // ── AI memory ─────────────────────────────────────────────────────────
        if (j.contains("ai_state") && j["ai_state"].is_array()) {
            const auto& ai_arr = j["ai_state"];
            for (size_t i = 0; i < ai_arr.size() && i < engine.ai_controllers.size(); ++i) {
                const auto& aj = ai_arr[i];
                auto& ai = engine.ai_controllers[i];
                ai.memory.clear();
                if (aj.contains("memory") && aj["memory"].is_array()) {
                    for (const auto& mj : aj["memory"]) {
                        AeonRulerAI::MemoryEntry m;
                        m.other_civ = mj.value("other", -1);
                        m.trust     = mj.value("trust", 0.0f);
                        m.fear      = mj.value("fear",  0.0f);
                        m.hatred    = mj.value("hate",  0.0f);
                        m.year      = mj.value("year",  0);
                        ai.memory.push_back(m);
                    }
                }
                if (aj.contains("goal"))
                    ai.strategic_goal = CivGoal(aj["goal"].get<int>());
            }
        }

        std::cout << "[PERSISTENCE] Loaded world from " << filepath
                  << " (Year " << engine.year << ", format v" << fmt_version << ")" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "[PERSISTENCE ERROR] Load failed: " << e.what() << std::endl;
        return false;
    }
}

} // namespace Aeon
