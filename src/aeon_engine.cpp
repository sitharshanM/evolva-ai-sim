#include "aeon_engine.h"
#include "aeon_ollama.h"
#include "aeon_persistence.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <set>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  Civilization factory
// ─────────────────────────────────────────────────────────────────────────────
AeonCivilization AeonEngine::make_civilization(
        int id, const std::string& name,
        int cap_x, int cap_y, char map_char,
        float aggression, float diplo, float sci) {
    AeonCivilization c;
    c.id          = id;
    c.name        = name;
    c.map_char    = map_char;
    c.capital_x   = cap_x;
    c.capital_y   = cap_y;
    c.aggression      = aggression;
    c.diplomacy_pref  = diplo;
    c.science_pref    = sci;
    c.trade_pref      = std::clamp(1.0f - aggression * 0.7f, 0.2f, 0.9f);
    c.expansion_pref  = std::clamp(aggression * 0.7f, 0.2f, 0.9f);
    c.stability       = 80.0f;
    c.army_size       = 5000.0f + aggression * 4000.0f;
    c.military_power  = c.army_size * 0.1f;
    c.population.total = 500000LL;
    c.primary_goal    = "Develop and expand " + name;
    c.hidden_goal     = "Become the dominant civilization of the age";
    return c;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Ruler Generator with authentic naming, skills, and traits
// ─────────────────────────────────────────────────────────────────────────────
AeonCharacter AeonEngine::make_ruler(int id, const std::string& name,
                                      int civ_id, int birth_year) {
    GovForm gov = GovForm::MONARCHY;
    if (civ_id >= 0 && civ_id < (int)civs.size()) {
        gov = civs[civ_id].government;
    }

    static const char* royal_titles[] = {
        "King", "Queen", "Grand Duke", "High Sovereign"
    };
    static const char* imperial_titles[] = {
        "Emperor", "Empress", "Imperator", "High Sovereign"
    };
    static const char* democratic_titles[] = {
        "President", "Chancellor", "Prime Minister", "Consul"
    };
    static const char* junta_titles[] = {
        "Generalissimo", "Lord General", "Commander-in-Chief", "Field Marshal"
    };
    static const char* theocratic_titles[] = {
        "High Patriarch", "Hierophant", "Archbishop", "Grand Inquisitor"
    };
    static const char* autocratic_titles[] = {
        "Supreme Leader", "Chairman", "General Secretary", "Lord Protector"
    };

    static const char* first_names[] = {
        "Aurelius", "Valeria", "Thorne", "Seraphina", "Kaelen",
        "Theodora", "Balian", "Helena", "Darius", "Lyanna",
        "Cassian", "Miriel", "Alaric", "Vaelor", "Isolde"
    };
    static const char* last_names[] = {
        "Thorne", "Vaelor", "Darius", "Alaric", "Sterling",
        "Kaelen", "Hawthorne", "Blackwood", "Ashford", "Mercer"
    };

    AeonCharacter r;
    r.id         = id;
    r.civ_id     = civ_id;
    r.birth_year = birth_year;
    r.age        = std::max(22, 2026 - birth_year);
    r.is_ruler   = true;
    r.influence  = 80.0f;
    r.reputation = 60.0f;

    if (name.empty() || name.find("Successor of") != std::string::npos || name.find("High Sovereign of") != std::string::npos) {
        int fn_idx = rng.uniform_int(0, 14);
        int ln_idx = rng.uniform_int(0, 9);
        std::string roman[] = {"I", "II", "III", "IV", "V"};
        int r_idx = rng.uniform_int(0, 4);

        if (gov == GovForm::REPUBLIC || gov == GovForm::DEMOCRACY || gov == GovForm::FEDERATION) {
            int t_idx = rng.uniform_int(0, 3);
            r.title = democratic_titles[t_idx];
            r.name  = std::string(democratic_titles[t_idx]) + " " + first_names[fn_idx] + " " + last_names[ln_idx];
        } else if (gov == GovForm::MILITARY_JUNTA) {
            int t_idx = rng.uniform_int(0, 3);
            r.title = junta_titles[t_idx];
            r.name  = std::string(junta_titles[t_idx]) + " " + first_names[fn_idx] + " " + last_names[ln_idx];
        } else if (gov == GovForm::THEOCRACY) {
            int t_idx = rng.uniform_int(0, 3);
            r.title = theocratic_titles[t_idx];
            r.name  = std::string(theocratic_titles[t_idx]) + " " + first_names[fn_idx];
        } else if (gov == GovForm::DICTATORSHIP) {
            int t_idx = rng.uniform_int(0, 3);
            r.title = autocratic_titles[t_idx];
            r.name  = std::string(autocratic_titles[t_idx]) + " " + first_names[fn_idx] + " " + last_names[ln_idx];
        } else if (gov == GovForm::EMPIRE) {
            int t_idx = rng.uniform_int(0, 3);
            r.title = imperial_titles[t_idx];
            r.name  = std::string(imperial_titles[t_idx]) + " " + first_names[fn_idx] + " " + roman[r_idx];
        } else {
            // Monarchy / default
            int t_idx = rng.uniform_int(0, 3);
            r.title = royal_titles[t_idx];
            r.name  = std::string(royal_titles[t_idx]) + " " + first_names[fn_idx] + " " + roman[r_idx];
        }
    } else {
        r.title = (gov == GovForm::REPUBLIC || gov == GovForm::DEMOCRACY) ? "President" : "High Sovereign";
        r.name  = name;
    }

    r.competence       = float(rng.uniform_int(40, 95)) / 100.0f;
    r.military_skill   = float(rng.uniform_int(30, 95)) / 100.0f;
    r.diplomatic_skill = float(rng.uniform_int(30, 95)) / 100.0f;
    r.economic_skill   = float(rng.uniform_int(35, 95)) / 100.0f;
    r.scientific_skill = float(rng.uniform_int(30, 95)) / 100.0f;

    int trait_roll = rng.uniform_int(0, 7);
    r.trait = static_cast<RulerTrait>(trait_roll);

    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
//  init
// ─────────────────────────────────────────────────────────────────────────────
void AeonEngine::init(uint64_t s) {
    *this = AeonEngine{};
    seed = s;
    rng_.seed(seed);
    rng.reseed(seed);
    id_reg = IDRegistry();
    history.set_id_registry(&id_reg);

    world_map.generate(seed);

    struct CivDef { const char* name; int x; int y; char ch; float agg; float dip; float sci; const char* model; };
    CivDef defs[5] = {
        { "NORDRA",   103, 51, 'N', 0.60f, 0.70f, 0.90f, "nemotron" },
        { "ELDORIA",  296, 50, 'E', 0.65f, 0.65f, 0.85f, "openrouter_eldoria" },
        { "VALORIA",  184, 40, 'V', 0.35f, 0.85f, 0.80f, "openrouter" },
        { "DRAKOR",   217, 34, 'D', 0.80f, 0.40f, 0.75f, "openrouter_drakor" },
        { "SOLARIA",  257, 61, 'S', 0.45f, 0.75f, 0.75f, "llama3.1:latest" },
    };

    civs.clear();
    ai_controllers.clear();
    characters.clear();

    for (int i = 0; i < 5; ++i) {
        const auto& d = defs[i];
        auto civ = make_civilization(i, d.name, d.x, d.y, d.ch, d.agg, d.dip, d.sci);
        civ.territory_tiles = 80.0f;
        civs.push_back(civ);

        AeonRulerAI ai(i);
        ai.primary_goal = "Lead " + std::string(d.name) + " to technological hegemony and prosperity";
        ai.hidden_goal  = "Expand influence across the world map and dominate rival realms";
        ai.model_name   = d.model;
        ai_controllers.push_back(ai);

        auto ruler = make_ruler(next_char_id++, "", i, year - 35);
        civs.back().ruler_id = ruler.id;
        civs.back().character_ids.push_back(ruler.id);
        characters.push_back(ruler);

        world_map.set_city(d.x, d.y, 0, '@');

        history.record(year, month, "CIVILIZATION",
            std::string(d.name) + " Empire founded under " + ruler.name,
            "The empire of " + std::string(d.name) + " establishes its seat of power.",
            i);
    }

    military_engine.init_default_forces(*this);
    central_bank_engine.init_default_currencies(*this);
    nuclear_engine.init_arsenals(*this);
    space_race_engine.init_programs(*this);
    dynasty_engine.init_dynasties(*this);
    megawonder_engine.init_megawonders(*this);
    maritime_engine.init_maritime(*this);
    citizen_engine.init_citizens(*this);

    // The Commons
    AeonCivilization commons;
    commons.id       = 5;
    commons.name     = "THE COMMONS";
    commons.map_char = 'C';
    commons.capital_x = 180; commons.capital_y = 90;
    commons.is_commons = true;
    commons.government = GovForm::FEDERATION;
    commons.population.total = 100000000LL;
    commons.stability = 55.0f;
    commons.army_size = 5000.0f;
    commons.primary_goal = "Preserve independence and open maritime shipping lanes";
    civs.push_back(commons);

    AeonRulerAI commons_ai(5);
    commons_ai.primary_goal = "Maintain global balance of power";
    commons_ai.hidden_goal  = "Mediate conflicts between Great Empires";
    commons_ai.model_name   = "llama3.1";
    ai_controllers.push_back(commons_ai);

    auto commons_ruler = make_ruler(next_char_id++, "Consul of the Commons", 5, year - 45);
    commons_ruler.trait = RulerTrait::DIPLOMAT;
    civs.back().ruler_id = commons_ruler.id;
    civs.back().character_ids.push_back(commons_ruler.id);
    characters.push_back(commons_ruler);

    caravan_engine.init_map_resources(360.0f, 180.0f, seed);
    space_espionage.init();
    disaster_engine.init();
    aeon_religion_engine.init();
    wonder_engine.init();
    economy_market_engine.init();
    naval_engine.init();
    parliament_engine.init();
    un_council_engine.init();
    alliance_engine.init();
    analytics_engine.init();
    diplomatic_summit_engine.init();
    genetics_engine.init(*this);
    tactical_battle_engine.init();
    scenario_editor_engine.init();
    ruler_psyche_engine.init(*this);
    deep_state_engine.init(*this);
    kinetic_strike_engine.init();
    analytics_engine.record_year(*this);

    history.record(year, month, "CIVILIZATION",
        "THE COMMONS Council Founded",
        "A federation of unaligned free ports, trade hubs, and neutral territories emerges.", 5);

    year   = 2026;
    month  = 1;
    day    = 1;
    speed  = SPEED_NORMAL;
    paused = false;

    std::cout << "\n=======================================================\n";
    std::cout << "  AEON  --  Authoritative Simulation Engine\n";
    std::cout << "=======================================================\n";
    std::cout << "  Empires: NORDRA  ELDORIA  VALORIA  DRAKOR  SOLARIA\n";
    std::cout << "  World Seed: " << seed << "\n";
    std::cout << "  AI Engine : DETERMINISTIC NATION CABINETS\n\n";

    market_engine.init();
    religion_engine.init();
    president_game.init();
    space_espionage.init();

    for (auto& ai : ai_controllers)
        ai.model_name = "rule_based";
}

bool AeonEngine::save_world(const std::string& filename) const {
    return SaveLoadEngine::save(*this, filename);
}

bool AeonEngine::load_world(const std::string& filename) {
    return SaveLoadEngine::load(*this, filename);
}

std::string AeonEngine::status_line() const {
    std::ostringstream ss;
    ss << "[YEAR " << year << " / Month " << month << "] (Seed:" << seed << ") Realms: ";
    for (const auto& c : civs) {
        if (c.is_alive > 0.0f) {
            ss << c.name << "[" << c.population.total / 1000 << "k] ";
        }
    }
    return ss.str();
}

std::string AeonEngine::inspect_ai_decision(int civ_id) const {
    if (civ_id >= 0 && civ_id < (int)ai_controllers.size()) {
        const auto& ai = ai_controllers[civ_id];
        std::ostringstream ss;
        ss << "Goal: " << civ_goal_name(ai.strategic_goal) << "\n";
        ss << "Last decisions log:\n" << ai.last_decision_log << "\n";
        return ss.str();
    }
    return "No AI state found for civilization ID " + std::to_string(civ_id);
}

void AeonEngine::tick_second(float real_dt) {
    if (paused) return;
    if (real_dt > 0.25f) real_dt = 0.25f;

    float years_delta = (real_dt / SECONDS_PER_YEAR) * speed;
    time_accum += years_delta;
    while (time_accum >= 1.0f) {
        time_accum -= 1.0f;
        year++;
        month = 1;
        tick_one_year();
    }
    month = (static_cast<int>(time_accum * MONTHS_PER_YEAR) % 12) + 1;

    // Presentation frames must not consume simulation randomness or mutate markets.
}

// ─────────────────────────────────────────────────────────────────────────────
//  tick_one_year
// ─────────────────────────────────────────────────────────────────────────────
void AeonEngine::tick_one_year() {
    runtime.begin_tick(*this);
    gis_climate_engine.update_climate_physics(float(year % 12) * 0.5235988f);
    supply_demand_engine.update_prices_tick(*this);
    demographics_engine.update_demographics_year(*this);
    military_engine.update_military_tick(*this);
    central_bank_engine.update_central_banks_tick(*this);
    agent_engine.update_ai_tick(*this);
    rebellion_engine.update_rebellions_tick(*this);
    nuclear_engine.update_nuclear_tick(*this);
    space_race_engine.update_space_race_tick(*this);
    dynasty_engine.update_dynasties_tick(*this);
    megawonder_engine.update_megawonders_tick(*this);
    maritime_engine.update_maritime_tick(*this);
    citizen_engine.update_citizens_tick(*this);

    gis_climate_engine.tick_year_climate(*this);
    gis_climate_engine.update_seasonal_yield(month > 0 ? month : 1);

    central_bank_engine.check_hyperinflation(*this);
    central_bank_engine.advance_business_cycle(*this);
    central_bank_engine.update_reserve_currency(*this);

    military_engine.tick_supply_lines(*this);
    military_engine.resolve_sieges(*this);

    for (auto& civ : civs) {
        if (!civ.at_war && civ.war_exhaustion > 0.0f) {
            civ.war_exhaustion = std::max(0.0f, civ.war_exhaustion - 6.0f);
        }
    }

    demographics_engine.update_urbanization(*this);
    demographics_engine.apply_inequality_feedback(*this);
    demographics_engine.apply_generational_education(*this);
    demographics_engine.process_refugee_intake(*this);
    demographics_engine.advance_pandemic_season(*this, month > 0 ? month : 1);

    parliament_engine.shift_seats_by_events(*this);
    parliament_engine.process_lobbying(*this);
    parliament_engine.check_protest_escalation(*this);
    parliament_engine.enforce_term_limits(*this);

    caravan_engine.process_resource_nodes_tick(*this);
    caravan_engine.process_manufacturing_chains_tick(*this);
    citizen_engine.check_oligarch_monopolies(*this);
    tech_tree_engine.process_brain_drain_tick(*this);
    disaster_engine.check_disaster_cascades(*this);
    space_race_engine.check_kessler_cascade(*this);
    space_race_engine.process_asteroid_commodity_shock(*this);

    history.decay_relations(1.0f);

    runtime.phase = SimulationPhase::SOCIETY;
    // 1. Tick civilization states & Government Modifiers
    for (auto& civ : civs) {
        if (civ.is_alive > 0.0f) {
            civ.tick_year(year, 1.0f);

            auto mod = GovernmentSystem::get_modifiers(civ.government);
            civ.military_power = civ.army_size * 0.12f * mod.military_power_mult;
            civ.corruption = std::clamp(civ.corruption + mod.corruption_growth, 0.0f, 100.0f);
            civ.unrest = std::clamp(civ.unrest - (2.0f * mod.unrest_decay_rate) + (civ.corruption > 40.0f ? 1.5f : 0.0f), 0.0f, 100.0f);

            const AeonCharacter* ruler = nullptr;
            if (civ.ruler_id >= 0) {
                for (const auto& ch : characters) {
                    if (ch.id == civ.ruler_id && ch.is_alive) { ruler = &ch; break; }
                }
            }
            gov_transition_engine.tick_power_consolidation(civ, ruler, year, *this);
        }
    }

    if (president_game.active) {
        president_game.tick_year(*this);
        space_espionage.tick_year(*this);
    }

    // 2. Age characters and handle Succession
    std::vector<AeonCharacter> new_rulers_to_add;
    for (auto& ch : characters) {
        ch.age_one_year();
        if (ch.is_ruler && ch.is_alive && ch.check_natural_death(year)) {
            ch.is_alive = false;
            ch.death_year = year;

            for (auto& civ : civs) {
                if (civ.ruler_id == ch.id) {
                    std::cout << "\n[YEAR " << year << "] 👑 SUCCESSION: " << civ.name
                              << "'s ruler " << ch.name << " has died aged " << ch.age << "." << std::endl;

                    if (GovernmentSystem::is_authoritarian(civ.government)) {
                        gov_transition_engine.handle_authoritarian_succession(civ, ch, characters, *this, year);
                        break;
                    }

                    auto new_ruler = make_ruler(next_char_id++, "", civ.id, year - rng.uniform_int(25, 45));
                    civ.ruler_id = new_ruler.id;
                    civ.character_ids.push_back(new_ruler.id);
                    new_rulers_to_add.push_back(new_ruler);

                    // Succession instability shock
                    civ.stability = std::clamp(civ.stability - float(rng.uniform_int(5, 12)), 10.0f, 100.0f);

                    switch (new_ruler.trait) {
                        case RulerTrait::WARMONGER:
                        case RulerTrait::MILITARIST:
                            civ.aggression     = std::clamp(civ.aggression + 0.25f, 0.50f, 0.95f);
                            civ.diplomacy_pref = std::clamp(civ.diplomacy_pref - 0.20f, 0.10f, 0.50f);
                            break;
                        case RulerTrait::AUTHORITARIAN:
                            civ.ruler_authority = std::clamp(civ.ruler_authority + 20.0f, 0.0f, 100.0f);
                            civ.aggression      = std::clamp(civ.aggression + 0.15f, 0.30f, 0.95f);
                            break;
                        case RulerTrait::DEMOCRATIC:
                            civ.democratic_institution_strength = std::clamp(civ.democratic_institution_strength + 15.0f, 0.0f, 100.0f);
                            civ.diplomacy_pref = std::clamp(civ.diplomacy_pref + 0.25f, 0.40f, 0.95f);
                            break;
                        case RulerTrait::DIPLOMAT:
                            civ.diplomacy_pref = std::clamp(civ.diplomacy_pref + 0.30f, 0.50f, 0.95f);
                            civ.aggression     = std::clamp(civ.aggression - 0.25f, 0.10f, 0.45f);
                            break;
                        case RulerTrait::SCHOLAR:
                        case RulerTrait::TECHNOCRAT:
                            civ.science_pref   = std::clamp(civ.science_pref + 0.30f, 0.50f, 0.95f);
                            break;
                        case RulerTrait::MERCHANT:
                            civ.trade_pref     = std::clamp(civ.trade_pref + 0.30f, 0.50f, 0.95f);
                            break;
                        case RulerTrait::EXPANSIONIST:
                            civ.expansion_pref = std::clamp(civ.expansion_pref + 0.30f, 0.50f, 0.95f);
                            break;
                        case RulerTrait::REFORMER:
                            civ.stability      = std::clamp(civ.stability + 15.0f, 0.0f, 100.0f);
                            break;
                        case RulerTrait::TYRANT:
                        case RulerTrait::PARANOID:
                            civ.aggression     = std::clamp(civ.aggression + 0.20f, 0.40f, 0.95f);
                            civ.stability      = std::clamp(civ.stability - 12.0f, 0.0f, 100.0f);
                            break;
                        case RulerTrait::IDEALIST:
                            civ.religiosity    = std::clamp(civ.religiosity + 0.25f, 0.0f, 1.0f);
                            break;
                        case RulerTrait::PRAGMATIST:
                        case RulerTrait::ISOLATIONIST:
                            civ.expansion_pref = std::clamp(civ.expansion_pref - 0.30f, 0.10f, 0.35f);
                            civ.diplomacy_pref = std::clamp(civ.diplomacy_pref - 0.25f, 0.10f, 0.35f);
                            break;
                    }

                    std::cout << "[YEAR " << year << "] 👑 NEW RULER: " << civ.name
                              << " crowns " << new_ruler.name
                              << " (" << ruler_trait_name(new_ruler.trait)
                              << ", Competence: " << int(new_ruler.competence * 100)
                              << "%, Mil: " << int(new_ruler.military_skill * 100)
                              << "%, Diplo: " << int(new_ruler.diplomatic_skill * 100) << "%)\n" << std::endl;

                    history.record(year, month, "PERSON",
                        ch.name + " dies; " + new_ruler.name + " ascends throne of " + civ.name,
                        new_ruler.name + " takes power with " + ruler_trait_name(new_ruler.trait) + " doctrine.",
                        civ.id, -1, {"ruler_succession"}, 0.70f);
                    break;
                }
            }
        }
    }
    for (const auto& nr : new_rulers_to_add) {
        characters.push_back(nr);
    }

    // 3. AI decisions use isolated nation observations by default.
    if (runtime.enabled) runtime.think(*this);
    else for (int i = 0; i < (int)civs.size(); ++i) {
        if (civs[i].is_alive <= 0.0f) continue;
        if (president_game.active && i == president_game.player_civ_id) continue;
        if (i >= (int)ai_controllers.size()) {
            ai_controllers.push_back(AeonRulerAI(civs[i].id));
        }

        auto dec = ai_controllers[i].decide(civs[i], civs, history, characters, year);
        apply_decision(i, dec);
    }

    // 4. War resolution
    check_war_resolution(year);

    // 5. Nation collapse & new nation emergence
    check_nation_collapse(year);
    spawn_new_nation(year);

    // 6. Council of Nations emergence
    check_council_formation(year);

    // 7. Random events
    spawn_random_event(year);

    // 8. Markets & Religions
    market_engine.tick_year(civs, year);
    religion_engine.tick_year(*this);
    chronicler.update_eras(*this, year);
    chronicler.update_chronicle(history, year);
    coalition_engine.check_automatic_coalition_formation(*this, year);
    caravan_engine.update_caravans(1.0f, *this);
    disaster_engine.tick_year(*this);
    aeon_religion_engine.tick_year(*this);
    wonder_engine.tick_year(*this);
    economy_market_engine.tick_year(*this);
    naval_engine.tick_year(*this);
    parliament_engine.tick_year(*this);
    un_council_engine.tick_year(*this);
    alliance_engine.tick_year(*this);
    genetics_engine.tick_year(*this);
    ruler_psyche_engine.tick_year(*this);
    deep_state_engine.tick_year(*this);
    analytics_engine.record_year(*this);

    // 9. Comprehensive World State Chronicle Summary (Every 10 Years)
    if (year % 10 == 0) {
        std::cout << "\n========================================================================\n";
        std::cout << "  📜 GLOBAL STATE CHRONICLE & GEOPOLITICAL RANKING (Year " << year << ")\n";
        std::cout << "========================================================================\n";

        // Find leaders
        int top_military_id = -1, top_econ_id = -1, top_tech_id = -1, top_pop_id = -1;
        float max_mil = -1.0f, max_gdp = -1.0f, max_tech = -1.0f;
        long long max_pop = -1;

        for (int i = 0; i < (int)civs.size(); ++i) {
            const auto& c = civs[i];
            if (c.is_alive <= 0.0f || c.is_commons) continue;
            if (c.army_size > max_mil) { max_mil = c.army_size; top_military_id = i; }
            if (c.economy.gdp > max_gdp) { max_gdp = c.economy.gdp; top_econ_id = i; }
            if ((int)c.tech.era * 100 + c.tech.progress > max_tech) {
                max_tech = (int)c.tech.era * 100 + c.tech.progress; top_tech_id = i;
            }
            if (c.population.total > max_pop) { max_pop = c.population.total; top_pop_id = i; }
        }

        std::cout << "  🎖️ Military Hegemon : " << (top_military_id >= 0 ? civs[top_military_id].name : "None")
                  << " (" << int(max_mil) << " soldiers)\n";
        std::cout << "  💰 Richest Realm     : " << (top_econ_id >= 0 ? civs[top_econ_id].name : "None")
                  << " (" << int(max_gdp) << " GDP)\n";
        std::cout << "  🔬 Tech Pioneer      : " << (top_tech_id >= 0 ? civs[top_tech_id].name : "None")
                  << " (" << (top_tech_id >= 0 ? tech_era_name(civs[top_tech_id].tech.era) : "None") << ")\n";
        std::cout << "  👥 Most Populous     : " << (top_pop_id >= 0 ? civs[top_pop_id].name : "None")
                  << " (" << (max_pop / 1000) << "k citizens)\n";

        std::cout << "\n  [REALM STATUS OVERVIEW]\n";
        for (const auto& c : civs) {
            if (c.is_alive <= 0.0f) continue;
            std::cout << "  • " << std::left << std::setw(12) << c.name
                      << " | Pop: " << std::setw(6) << c.population.total / 1000 << "k"
                      << " | GDP: " << std::setw(6) << int(c.economy.gdp)
                      << " | Army: " << std::setw(5) << int(c.army_size)
                      << " | Stab: " << std::setw(3) << int(c.stability) << "%"
                      << " | Trades: " << c.active_trade_agreements.size()
                      << " | " << tech_era_name(c.tech.era)
                      << (c.at_war ? " ⚔️ [AT WAR]" : "") << "\n";
        }
        std::cout << "========================================================================\n\n";
    }

    // Print Annual World Newspaper
    std::cout << chronicler.generate_aeon_daily(*this, year) << std::endl;
    runtime.end_tick(*this);
}

// ─────────────────────────────────────────────────────────────────────────────
//  apply_decision  —  Authoritative execution with strict validation
// ─────────────────────────────────────────────────────────────────────────────
void AeonEngine::apply_decision(int civ_idx, const AIDecision& dec) {
    if (runtime.phase != SimulationPhase::COMMANDS) {
        SimulationCommand command;
        command.actor = civ_idx; command.proposal = dec;
        command.source = runtime.phase == SimulationPhase::IDLE ? CommandSource::PLAYER : CommandSource::SYSTEM;
        runtime.submit(*this, command);
        return;
    }
    if (civ_idx < 0 || civ_idx >= (int)civs.size()) {
        runtime.record(*this, "REJECTED", civ_idx, dec.target_civ, "Actor does not exist");
        return;
    }
    // Defense-in-depth: Strict self-target rejection
    if (dec.target_civ == civ_idx && (dec.action_type == "DECLARE_WAR" ||
                                      dec.action_type == "FORM_ALLIANCE" ||
                                      dec.action_type == "PROPOSE_TRADE" ||
                                      dec.action_type == "NEGOTIATE_PEACE")) {
        runtime.record(*this, "REJECTED", civ_idx, dec.target_civ, "Cannot target self");
        return;
    }
    auto& civ = civs[civ_idx];

    const std::unordered_map<int, int> empty_cooldowns;
    const auto& war_cooldowns = civ_idx < (int)ai_controllers.size()
        ? ai_controllers[civ_idx].war_cooldown_ : empty_cooldowns;
    const auto& trade_cooldowns = civ_idx < (int)ai_controllers.size()
        ? ai_controllers[civ_idx].trade_cooldown_ : empty_cooldowns;
    std::string rejection;
    // Never trust a proposal's is_validated flag: the world may have changed.
    if (!ActionValidator::validate(dec, civ, civs, year, war_cooldowns, trade_cooldowns, rejection)) {
        runtime.record(*this, "REJECTED", civ_idx, dec.target_civ, rejection);
        history.record(year, month, "ACTION_REJECTED", civ.name + ": " + dec.action_type,
            rejection, civ.id, dec.target_civ, {"execution_validation"}, 0.1f);
        return;
    }
    const auto before = world_metrics(*this);

    if (dec.action_type == "DECLARE_WAR" && dec.target_civ >= 0 && dec.target_civ < (int)civs.size() && dec.target_civ != civ_idx) {
        auto& target = civs[dec.target_civ];
        if (!civ.at_war && !target.at_war && target.is_alive > 0.0f &&
            !civ.is_under_truce_with(dec.target_civ, year) && !target.is_under_truce_with(civ_idx, year)) {
            civ.at_war         = true;
            civ.war_with_civ   = dec.target_civ;
            civ.war_year_start = year;

            target.at_war         = true;
            target.war_with_civ   = civ_idx;
            target.war_year_start = year;

            // Cancel any active trade agreements between them
            civ.active_trade_agreements.erase(dec.target_civ);
            target.active_trade_agreements.erase(civ_idx);

            civ.relations[dec.target_civ] = DiplomacyStatus::AT_WAR;
            target.relations[civ_idx]     = DiplomacyStatus::AT_WAR;

            if (dec.target_civ < (int)ai_controllers.size()) {
                ai_controllers[dec.target_civ].add_memory(civ_idx, -40.0f, 25.0f, 35.0f, year);
            }
            if (civ_idx < (int)ai_controllers.size()) {
                ai_controllers[civ_idx].add_memory(dec.target_civ, -20.0f, 10.0f, 15.0f, year);
            }

            history.relation(civ_idx, dec.target_civ).record_war_start();
            history.relation(civ_idx, dec.target_civ).last_interaction_year = year;

            std::vector<std::string> causes;
            if (civ.aggression > 0.65f) causes.push_back("high_aggression");
            if (civ.army_size > target.army_size * 1.15f) causes.push_back("military_advantage");

            history.record(year, month, "WAR",
                civ.name + " declares war on " + target.name,
                dec.declaration, civ_idx, dec.target_civ, causes, 0.85f);

            active_events.push_back({"WAR",
                "War: " + civ.name + " vs " + target.name,
                civ_idx, dec.target_civ, dec.duration_years});
        }
    } else if (dec.action_type == "FORM_ALLIANCE" && dec.target_civ >= 0 && dec.target_civ < (int)civs.size() && dec.target_civ != civ_idx) {
        auto& target = civs[dec.target_civ];
        if (target.is_alive > 0.0f && !civ.at_war && !target.at_war) {
            civ.relations[dec.target_civ] = DiplomacyStatus::ALLY;
            target.relations[civ_idx]     = DiplomacyStatus::ALLY;

            if (civ_idx < (int)ai_controllers.size()) {
                ai_controllers[civ_idx].add_memory(dec.target_civ, 35.0f, -10.0f, -15.0f, year);
            }
            if (dec.target_civ < (int)ai_controllers.size()) {
                ai_controllers[dec.target_civ].add_memory(civ_idx, 35.0f, -10.0f, -15.0f, year);
            }

            history.relation(civ_idx, dec.target_civ).record_treaty();
            history.relation(civ_idx, dec.target_civ).last_interaction_year = year;

            history.record(year, month, "DIPLOMACY",
                civ.name + " and " + target.name + " form a mutual defensive alliance",
                dec.declaration, civ_idx, dec.target_civ,
                {"diplomatic_affinity", "shared_defense"}, 0.70f);
        }
    } else if (dec.action_type == "PROPOSE_TRADE" && dec.target_civ >= 0 && dec.target_civ < (int)civs.size() && dec.target_civ != civ_idx) {
        auto& target = civs[dec.target_civ];
        if (target.is_alive > 0.0f && !civ.at_war && !target.at_war) {
            // Establish 10-year bilateral trade agreement
            civ.active_trade_agreements[dec.target_civ] = 10;
            target.active_trade_agreements[civ_idx]     = 10;

            civ.economy.gdp += 35.0f;
            target.economy.gdp += 35.0f;

            if (civ_idx < (int)ai_controllers.size()) {
                ai_controllers[civ_idx].add_memory(dec.target_civ, 20.0f, 0.0f, 0.0f, year);
            }
            if (dec.target_civ < (int)ai_controllers.size()) {
                ai_controllers[dec.target_civ].add_memory(civ_idx, 20.0f, 0.0f, 0.0f, year);
            }

            history.relation(civ_idx, dec.target_civ).record_trade();
            history.relation(civ_idx, dec.target_civ).trade_volume += 50.0f;
            history.relation(civ_idx, dec.target_civ).last_interaction_year = year;

            history.record(year, month, "ECONOMY",
                civ.name + " and " + target.name + " sign a 10-year bilateral trade agreement",
                dec.declaration, civ_idx, dec.target_civ,
                {"commercial_interests", "mutual_benefit"}, 0.50f);
        }
    } else if (dec.action_type == "NEGOTIATE_PEACE" && dec.target_civ >= 0 && dec.target_civ < (int)civs.size() && dec.target_civ != civ_idx) {
        resolve_peace_conference(civ_idx, dec.target_civ, year);
    } else if (dec.action_type == "BUILD_MILITARY") {
        float cost = 40.0f;
        civ.economy.annual_income = std::max(0.0f, civ.economy.annual_income - cost);
        civ.army_size += 1500.0f;
        civ.military_power = civ.army_size * 0.12f;
    } else if (dec.action_type == "RESEARCH") {
        civ.tech.research_pts += 60.0f;
    } else if (dec.action_type == "EXPAND") {
        if (!civ.at_war && civ.territory_tiles < 400.0f) {
            civ.territory_tiles += 3.0f;
            civ.population.total += 20000;
        }
    } else if (dec.action_type == "BUILD_INFRASTRUCTURE") {
        civ.stability = std::clamp(civ.stability + 6.0f, 0.0f, 100.0f);
        civ.economy.gdp += 40.0f;
    } else if (dec.action_type == "QUELL_UNREST") {
        civ.stability = std::clamp(civ.stability + 18.0f, 0.0f, 100.0f);
        civ.unrest = std::clamp(civ.unrest - 20.0f, 0.0f, 100.0f);
        civ.morale = std::clamp(civ.morale + 10.0f, 0.0f, 100.0f);
    } else if (dec.action_type != "HOLD") {
        // Check if it's a government transition or political crisis action
        if (!gov_transition_engine.apply_transition(dec.action_type, civ, civs, characters, *this, year))
            return;
    }
    civ.history_memory.add_action(dec.action_type, dec.target_civ, year, dec.utility_score);
    runtime.record(*this, "APPLIED", civ_idx, dec.target_civ, dec.action_type,
        nlohmann::json::diff(before, world_metrics(*this)));
    if (civ_idx < (int)ai_controllers.size()) {
        auto& ai = ai_controllers[civ_idx];
        ai.action_last_used_year_[dec.action_type] = year;
        if (dec.action_type == "PROPOSE_TRADE") ai.trade_cooldown_[dec.target_civ] = year;
        if (dec.action_type == "DECLARE_WAR") ai.war_cooldown_[dec.target_civ] = year;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  check_war_resolution
// ─────────────────────────────────────────────────────────────────────────────
void AeonEngine::check_war_resolution(int current_year) {
    std::set<std::pair<int, int>> resolved_pairs;
    for (auto& civ : civs) {
        if (!civ.at_war || civ.war_with_civ < 0 || civ.war_with_civ >= (int)civs.size()) continue;
        if (civ.war_with_civ == civ.id) {
            civ.at_war = false;
            civ.war_with_civ = -1;
            continue;
        }

        auto& enemy = civs[civ.war_with_civ];
        if (enemy.is_alive <= 0.0f) {
            civ.at_war = false;
            civ.war_with_civ = -1;
            continue;
        }

        auto pair_key = std::make_pair(std::min(civ.id, enemy.id), std::max(civ.id, enemy.id));
        if (resolved_pairs.count(pair_key)) continue;

        if (civ.id < enemy.id) {
            civ.military_power  = civ.army_size * 0.12f;
            enemy.military_power = enemy.army_size * 0.12f;

            float civ_loss   = std::min(civ.army_size, enemy.military_power * 0.35f + 80.0f);
            float enemy_loss = std::min(enemy.army_size, civ.military_power * 0.35f + 80.0f);

            civ.army_size   = std::max(100.0f, civ.army_size - civ_loss);
            enemy.army_size = std::max(100.0f, enemy.army_size - enemy_loss);

            civ.war_exhaustion   = std::clamp(civ.war_exhaustion + 8.0f, 0.0f, 100.0f);
            enemy.war_exhaustion = std::clamp(enemy.war_exhaustion + 8.0f, 0.0f, 100.0f);
        }

        int war_duration = current_year - civ.war_year_start;
        float end_chance = (civ.war_exhaustion > 50.0f || enemy.war_exhaustion > 50.0f || war_duration > 4) ? 0.35f : 0.08f;
        float roll = float((current_year * 17 + civ.id * 31) % 100) / 100.0f;

        if (roll < end_chance && war_duration >= 2) {
            resolved_pairs.insert(pair_key);
            resolve_peace_conference(civ.id, enemy.id, current_year);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  resolve_peace_conference
// ─────────────────────────────────────────────────────────────────────────────
void AeonEngine::resolve_peace_conference(int civ1_id, int civ2_id, int current_year) {
    if (civ1_id < 0 || civ1_id >= (int)civs.size() || civ2_id < 0 || civ2_id >= (int)civs.size()) return;
    if (civ1_id == civ2_id) return; // Strict rejection of self-peace
    auto& c1 = civs[civ1_id];
    auto& c2 = civs[civ2_id];
    if (c1.is_alive <= 0.0f || c2.is_alive <= 0.0f) return;

    // Must be actively at war with each other
    bool at_war_bilateral = (c1.at_war && c1.war_with_civ == civ2_id) ||
                            (c2.at_war && c2.war_with_civ == civ1_id) ||
                            (c1.relations[civ2_id] == DiplomacyStatus::AT_WAR) ||
                            (c2.relations[civ1_id] == DiplomacyStatus::AT_WAR);
    if (!at_war_bilateral) {
        return;
    }

    int occ_by_1 = 0;
    int occ_by_2 = 0;
    for (const auto& p : c2.provinces) if (p.is_occupied && p.occupier_civ_id == civ1_id) occ_by_1++;
    for (const auto& p : c1.provinces) if (p.is_occupied && p.occupier_civ_id == civ2_id) occ_by_2++;

    float score_1 = (occ_by_1 - occ_by_2) * 30.0f + (c1.army_size - c2.army_size) * 0.05f + (c2.war_exhaustion - c1.war_exhaustion) * 0.5f;

    int victor_id = -1;
    int defeated_id = -1;
    if (score_1 > 10.0f) {
        victor_id = civ1_id;
        defeated_id = civ2_id;
    } else if (score_1 < -10.0f) {
        victor_id = civ2_id;
        defeated_id = civ1_id;
    }

    int negotiation_rounds = 4 + (current_year % 3);
    int truce_years = 15;

    PeaceTreaty treaty;
    treaty.year_signed = current_year;
    treaty.truce_duration_years = truce_years;
    treaty.negotiation_rounds = negotiation_rounds;

    // Clean venue naming
    std::string venue = c1.provinces.empty() ? c1.name : c1.provinces[0].name;
    while (!venue.empty() && (venue.back() == ' ' || venue.back() == ',')) venue.pop_back();
    std::string venue_upper = venue;
    std::transform(venue_upper.begin(), venue_upper.end(), venue_upper.begin(), ::toupper);
    treaty.treaty_name = "TREATY OF " + venue_upper + " — " + std::to_string(current_year);

    if (victor_id >= 0 && defeated_id >= 0) {
        auto& victor = civs[victor_id];
        auto& defeated = civs[defeated_id];
        treaty.victor_civ_id = victor_id;
        treaty.defeated_civ_id = defeated_id;

        // Cede up to 1-2 occupied provinces to victor
        std::vector<int> ceded_pids;
        for (auto it = defeated.provinces.begin(); it != defeated.provinces.end(); ) {
            if (it->is_occupied && it->occupier_civ_id == victor_id && (int)ceded_pids.size() < 2) {
                Province ceded = *it;
                ceded.civ_id = victor_id;
                ceded.is_occupied = false;
                ceded.occupier_civ_id = -1;
                ceded.occupation_resistance = 25.0f;
                ceded.stability = 60.0f;

                treaty.ceded_province_ids.push_back(ceded.id);
                treaty.ceded_province_names.push_back(ceded.name);
                victor.provinces.push_back(ceded);

                it = defeated.provinces.erase(it);
            } else {
                ++it;
            }
        }

        // Liberate any remaining occupied provinces
        for (auto& p : defeated.provinces) {
            if (p.is_occupied) {
                p.is_occupied = false;
                p.occupier_civ_id = -1;
                p.occupation_resistance = 0.0f;
            }
        }
        for (auto& p : victor.provinces) {
            if (p.is_occupied) {
                p.is_occupied = false;
                p.occupier_civ_id = -1;
                p.occupation_resistance = 0.0f;
            }
        }

        // War Reparations (dynamic scaling based on GDP and war exhaustion, not hardcoded $300B)
        float reparations = std::clamp(defeated.economy.gdp * 0.12f, 10.0f, 150.0f);
        reparations = std::min(reparations, defeated.economy.gdp * 0.25f);
        defeated.economy.gdp = std::max(40.0f, defeated.economy.gdp - reparations);
        victor.economy.gdp += reparations;
        treaty.reparations_total = reparations;

        // Revanchism & historical memory
        for (size_t pi = 0; pi < treaty.ceded_province_names.size(); ++pi) {
            defeated.bilateral_relations[victor_id].border_claim_score = std::min(100.0f, defeated.bilateral_relations[victor_id].border_claim_score + 40.0f);
            defeated.bilateral_relations[victor_id].provinces_lost++;
            victor.bilateral_relations[defeated_id].provinces_taken++;
        }
        defeated.bilateral_relations[victor_id].hatred = std::min(100.0f, defeated.bilateral_relations[victor_id].hatred + 25.0f);
        victor.bilateral_relations[defeated_id].respect = std::max(10.0f, victor.bilateral_relations[defeated_id].respect - 10.0f);

        victor.history_memory.recent_victory_year = current_year;
        defeated.history_memory.recent_defeat_year = current_year;
        history.relation(victor_id, defeated_id).record_victory_for_a();
        history.relation(defeated_id, victor_id).record_defeat_for_a();

        std::cout << "\n========================================================================\n";
        std::cout << "  🕊️ PEACE CONFERENCE: " << treaty.treaty_name << "\n";
        std::cout << "========================================================================\n";
        std::cout << "  Victorious Realm  : " << victor.name << " [ID:" << victor_id << "]\n";
        std::cout << "  Conceding Realm   : " << defeated.name << " [ID:" << defeated_id << "]\n";
        std::cout << "  Negotiation Rounds: " << negotiation_rounds << "\n";
        if (!treaty.ceded_province_names.empty()) {
            std::cout << "  Territory Ceded   : ";
            for (size_t i = 0; i < treaty.ceded_province_names.size(); ++i) {
                std::cout << treaty.ceded_province_names[i] << (i + 1 < treaty.ceded_province_names.size() ? ", " : "\n");
            }
        } else {
            std::cout << "  Territory Ceded   : None (Status Quo Borders)\n";
        }
        std::cout << "  War Reparations   : $" << int(reparations) << "B gold paid to " << victor.name << "\n";
        std::cout << "  Peace Guarantee   : " << truce_years << " Years (Truce active until " << (current_year + truce_years) << ")\n";
        std::cout << "========================================================================\n\n";

        std::string cede_desc = treaty.ceded_province_names.empty() ? "Status quo borders." : "Territory ceded.";
        history.record(current_year, month, "PEACE_TREATY",
            treaty.treaty_name + " concludes war between " + victor.name + " and " + defeated.name,
            victor.name + " emerges victorious. Reparations: $" + std::to_string(int(reparations)) + "B. " + cede_desc +
            " Peace guaranteed for " + std::to_string(truce_years) + " years.",
            victor_id, defeated_id, {"peace_conference", "treaty_signed"}, 0.90f);
    } else {
        // Inconclusive / White Peace
        treaty.is_white_peace = true;
        for (auto& p : c1.provinces) { p.is_occupied = false; p.occupier_civ_id = -1; p.occupation_resistance = 0.0f; }
        for (auto& p : c2.provinces) { p.is_occupied = false; p.occupier_civ_id = -1; p.occupation_resistance = 0.0f; }

        std::cout << "\n========================================================================\n";
        std::cout << "  🕊️ PEACE CONFERENCE: " << treaty.treaty_name << " (WHITE PEACE)\n";
        std::cout << "========================================================================\n";
        std::cout << "  Belligerents      : " << c1.name << " [ID:" << civ1_id << "] vs " << c2.name << " [ID:" << civ2_id << "] (Inconclusive)\n";
        std::cout << "  Negotiation Rounds: " << negotiation_rounds << "\n";
        std::cout << "  Territorial Terms : Mutual Return of Occupied Lands (Status Quo Ante Bellum)\n";
        std::cout << "  Peace Guarantee   : " << truce_years << " Years (Truce active until " << (current_year + truce_years) << ")\n";
        std::cout << "========================================================================\n\n";

        history.record(current_year, month, "PEACE_TREATY",
            treaty.treaty_name + " (Armistice): " + c1.name + " and " + c2.name,
            "Both realms exhaust war capacity. White peace signed with mutual return of captured territory. "
            "Peace guaranteed for " + std::to_string(truce_years) + " years.",
            civ1_id, civ2_id, {"white_peace", "treaty_signed"}, 0.80f);
    }

    // Set truces and store treaty records
    c1.signed_treaties.push_back(treaty);
    c2.signed_treaties.push_back(treaty);
    c1.bilateral_relations[civ2_id].truce_until_year = current_year + truce_years;
    c2.bilateral_relations[civ1_id].truce_until_year = current_year + truce_years;
    c1.bilateral_relations[civ2_id].wars_fought++;
    c2.bilateral_relations[civ1_id].wars_fought++;

    // Reset war states
    c1.at_war = false;
    c1.war_with_civ = -1;
    c1.relations[civ2_id] = DiplomacyStatus::NEUTRAL;
    c2.at_war = false;
    c2.war_with_civ = -1;
    c2.relations[civ1_id] = DiplomacyStatus::NEUTRAL;

    // Synchronize All Allied Belligerents to prevent war re-entry!
    for (auto& other_civ : civs) {
        if (other_civ.id == civ1_id || other_civ.id == civ2_id) continue;
        if (other_civ.at_war && (other_civ.war_with_civ == civ1_id || other_civ.war_with_civ == civ2_id)) {
            other_civ.at_war = false;
            other_civ.war_with_civ = -1;
            other_civ.relations[civ1_id] = DiplomacyStatus::NEUTRAL;
            other_civ.relations[civ2_id] = DiplomacyStatus::NEUTRAL;
            other_civ.bilateral_relations[civ1_id].truce_until_year = current_year + truce_years;
            other_civ.bilateral_relations[civ2_id].truce_until_year = current_year + truce_years;
            c1.bilateral_relations[other_civ.id].truce_until_year = current_year + truce_years;
            c2.bilateral_relations[other_civ.id].truce_until_year = current_year + truce_years;
        }
    }

    // Remove active WAR events
    active_events.erase(std::remove_if(active_events.begin(), active_events.end(),
        [&](const ActiveEvent& e) {
            return e.type == "WAR" &&
                ((e.civ_id == civ1_id && e.civ2_id == civ2_id) ||
                 (e.civ_id == civ2_id && e.civ2_id == civ1_id));
        }), active_events.end());
}

// ─────────────────────────────────────────────────────────────────────────────
//  check_council_formation
// ─────────────────────────────────────────────────────────────────────────────
void AeonEngine::check_council_formation(int current_year) {
    if (council_exists || current_year < 100) return;

    int war_count = 0;
    for (const auto& c : civs) if (c.at_war) war_count++;
    bool tired_of_war = war_count == 0 && current_year > 150;
    int alliance_count = 0;
    for (const auto& c : civs)
        for (const auto& r : c.relations)
            if (r.second == DiplomacyStatus::ALLY) alliance_count++;

    float roll = float((current_year * 7 + 3) % 100) / 100.0f;
    if (tired_of_war && alliance_count >= 3 && roll < 0.08f) {
        council_exists = true;
        council_year   = current_year;
        for (const auto& c : civs) if (c.is_alive > 0.0f) council_members.push_back(c.id);

        std::cout << "\n[YEAR " << current_year << "] 🏛️ COUNCIL OF NATIONS ESTABLISHED\n\n";

        history.record(current_year, month, "DIPLOMACY",
            "Council of Nations established",
            "After centuries of conflict, the great civilizations establish the Council of Nations.",
            -1, -1, {"global_peace", "multilateral_diplomacy"}, 0.90f);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  spawn_random_event
// ─────────────────────────────────────────────────────────────────────────────
void AeonEngine::spawn_random_event(int current_year) {
    float roll = float((current_year * 53 + 7) % 100) / 100.0f;
    if (roll > 0.08f) return;

    int civ_idx = rng.uniform_int(0, (int)civs.size() - 1);
    auto& civ = civs[civ_idx];
    if (civ.is_alive <= 0.0f) return;

    int ev_type = rng.uniform_int(0, 3);
    if (ev_type == 0) {
        civ.population.total = (civ.population.total * 95) / 100;
        civ.stability = std::clamp(civ.stability - 6.0f, 0.0f, 100.0f);
        history.record(current_year, month, "DISASTER",
            "Famine strikes " + civ.name,
            "Crop failures cause localized shortages and unrest.", civ_idx);
    } else if (ev_type == 1) {
        civ.economy.gdp += 100.0f;
        civ.stability = std::clamp(civ.stability + 5.0f, 0.0f, 100.0f);
        history.record(current_year, month, "ECONOMY",
            "Economic boom in " + civ.name,
            "Rich mineral deposits and agricultural surplus boost the economy.", civ_idx);
    } else if (ev_type == 2) {
        civ.tech.research_pts += 80.0f;
        history.record(current_year, month, "TECH",
            "Scientific breakthrough in " + civ.name,
            "Scholars discover major innovations in metallurgy and philosophy.", civ_idx);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  check_nation_collapse
// ─────────────────────────────────────────────────────────────────────────────
void AeonEngine::check_nation_collapse(int current_year) {
    for (int i = 0; i < (int)civs.size(); ++i) {
        auto& civ = civs[i];
        if (civ.is_alive <= 0.0f || civ.is_commons) continue;

        if (civ.population.total <= 5000 || (civ.stability <= 5.0f && civ.army_size <= 200.0f)) {
            civ.is_alive = 0.0f;
            civ.at_war   = false;
            civ.war_with_civ = -1;

            for (int ty = 0; ty < MAP_HEIGHT; ++ty)
                for (int tx = 0; tx < MAP_WIDTH; ++tx)
                    if (world_map.tile(tx, ty).owner_civ == i)
                        world_map.tile(tx, ty).owner_civ = -1;

            std::cout << "[YEAR " << current_year << "] 💀 COLLAPSE: " << civ.name
                      << " (ID:" << civ.id << ") disintegrates into ruins!" << std::endl;

            history.record(current_year, month, "COLLAPSE",
                civ.name + " collapses into history",
                "With shattered stability and exhausted population, " + civ.name + " has fallen.", i);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  spawn_new_nation
// ─────────────────────────────────────────────────────────────────────────────
void AeonEngine::spawn_new_nation(int current_year) {
    int alive_count = 0;
    for (const auto& c : civs) if (c.is_alive > 0.0f) alive_count++;
    if (alive_count >= 10) return;
    if (current_year < 40) return;

    float roll = float((current_year * 37 + 11) % 1000) / 1000.0f;
    if (roll > 0.03f) return;

    static const char* unique_clan_names[] = {
        "KORROTH", "ZYMNARA", "VAELTHAR", "DURNOK", "SALINDRA",
        "HROTHGAR", "CALYX", "MORVAINE", "KELDRITH", "TARKON",
        "VERIDIA", "ILTHOR", "XANTHAR", "BRENTHIA", "OAKMERE",
        "SOLIS", "AEGINA", "CYRENE", "THALASSA", "MYRMIDON"
    };
    static const char map_chars[] = "ABCFGHJKLMPQRTUWXYZ";

    int new_id = (int)civs.size();
    AeonCivilization new_civ;
    new_civ.id       = new_id;
    new_civ.is_alive = 1.0f;
    new_civ.stability = 65.0f;

    std::string candidate_name;
    for (int attempt = 0; attempt < 20; ++attempt) {
        int idx = rng.uniform_int(0, 19);
        std::string test = unique_clan_names[idx];
        bool name_taken = false;
        for (const auto& c : civs) {
            if (c.name == test) { name_taken = true; break; }
        }
        if (!name_taken) {
            candidate_name = test;
            break;
        }
    }
    if (candidate_name.empty()) {
        candidate_name = "REALM OF " + std::string(1, map_chars[new_id % 19]) + std::to_string(new_id);
    }

    new_civ.name = candidate_name;
    new_civ.map_char = map_chars[new_id % (sizeof(map_chars)-1)];

    new_civ.capital_x = 15 + rng.uniform_int(0, MAP_WIDTH  - 30);
    new_civ.capital_y = 10 + rng.uniform_int(0, MAP_HEIGHT - 20);

    new_civ.aggression      = float(rng.uniform_int(35, 75)) / 100.0f;
    new_civ.diplomacy_pref  = float(rng.uniform_int(30, 80)) / 100.0f;
    new_civ.science_pref    = float(rng.uniform_int(30, 75)) / 100.0f;
    new_civ.trade_pref      = float(rng.uniform_int(40, 80)) / 100.0f;
    new_civ.expansion_pref  = float(rng.uniform_int(30, 70)) / 100.0f;
    new_civ.territory_tiles = float(rng.uniform_int(18, 30));
    new_civ.population.total = 120000 + rng.uniform_int(0, 150000);
    new_civ.army_size       = 3500.0f + float(rng.uniform_int(0, 3000));
    new_civ.tech.era        = TechEra::METALLURGY;
    new_civ.government      = GovForm::MONARCHY;

    civs.push_back(new_civ);

    AeonRulerAI new_ai(new_id);
    new_ai.primary_goal = "Build and defend " + candidate_name;
    new_ai.model_name   = "rule_based";
    ai_controllers.push_back(new_ai);

    auto ruler = make_ruler(next_char_id++, "", new_id, current_year - 30);
    civs.back().ruler_id = ruler.id;
    civs.back().character_ids.push_back(ruler.id);
    characters.push_back(ruler);

    world_map.set_city(new_civ.capital_x, new_civ.capital_y, 0, '@');

    std::cout << "\n[YEAR " << current_year << "] 🌟 EMERGENCE: A new civilization rises: "
              << new_civ.name << " (ID:" << new_id << ") ruled by " << ruler.name << "!\n" << std::endl;

    history.record(current_year, month, "CIVILIZATION",
        "New realm rises: " + new_civ.name,
        new_civ.name + " establishes sovereignty on the world stage under " + ruler.name + ".",
        new_id, -1, {"emergence"}, 0.80f);
}

} // namespace Aeon
