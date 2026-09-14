#include "aeon_rebellion.h"
#include "aeon_engine.h"
#include "aeon_military.h"
#include "aeon_ruler_ai.h"
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace Aeon {

AeonRebellionEngine::AeonRebellionEngine() {
}

void AeonRebellionEngine::trigger_secession(int civ_id, AeonEngine& engine) {
    trigger_state_splinter(civ_id, engine);
}

std::string AeonRebellionEngine::generate_unique_successor_name(
    const AeonCivilization& parent,
    const std::string& prov_name,
    IdeologyType ideology,
    GovForm gov,
    const std::vector<AeonCivilization>& all_civs) {

    auto is_taken = [&](const std::string& candidate) {
        for (const auto& c : all_civs) {
            if (c.name == candidate) return true;
        }
        return false;
    };

    std::vector<std::string> candidates;

    if (gov == GovForm::MILITARY_JUNTA || ideology == IdeologyType::MILITARISM) {
        if (!prov_name.empty()) {
            candidates.push_back(prov_name + " Military Directorate");
            candidates.push_back(prov_name + " Defense Command");
            candidates.push_back("Martial Republic of " + prov_name);
            candidates.push_back(prov_name + " Revolutionary Front");
        }
        candidates.push_back(parent.name + " Military Directorate");
        candidates.push_back(parent.name + " Military Council");
        candidates.push_back("National Salvation Front of " + parent.name);
        candidates.push_back("Armed Forces of " + parent.name);
        candidates.push_back(parent.name + " Revolutionary Command");
    } else if (gov == GovForm::FEDERATION || ideology == IdeologyType::FEDERALISM) {
        if (!prov_name.empty()) {
            candidates.push_back(prov_name + " Federation");
            candidates.push_back("Confederation of " + prov_name);
            candidates.push_back("Autonomous State of " + prov_name);
            candidates.push_back("Free State of " + prov_name);
            candidates.push_back("League of " + prov_name);
        }
        candidates.push_back("Free Provinces of " + parent.name);
        candidates.push_back("United Provinces of " + parent.name);
        candidates.push_back("Confederate Provinces of " + parent.name);
        candidates.push_back(parent.name + " Democratic Union");
    } else {
        // Republican / Democratic
        if (!prov_name.empty()) {
            candidates.push_back("Republic of " + prov_name);
            candidates.push_back("Democratic Republic of " + prov_name);
            candidates.push_back(prov_name + " Free State");
            candidates.push_back("Commonwealth of " + prov_name);
            candidates.push_back("Independent Republic of " + prov_name);
        }
        candidates.push_back("Northern Republic of " + parent.name);
        candidates.push_back("Southern Republic of " + parent.name);
        candidates.push_back("Eastern Republic of " + parent.name);
        candidates.push_back("Western Republic of " + parent.name);
        candidates.push_back("Democratic Republic of " + parent.name);
        candidates.push_back("Federal Republic of " + parent.name);
        candidates.push_back("People's Republic of " + parent.name);
        candidates.push_back("Republic of " + parent.name);
    }

    for (const auto& cand : candidates) {
        if (!is_taken(cand)) {
            return cand;
        }
    }

    // If all base candidates are taken, append Roman numerals or ordinal suffix
    std::string base = candidates.empty() ? ("Republic of " + parent.name) : candidates[0];
    const char* numerals[] = { " II", " III", " IV", " V", " VI", " VII", " VIII" };
    for (const char* num : numerals) {
        std::string cand = base + num;
        if (!is_taken(cand)) {
            return cand;
        }
    }

    int suffix = 1;
    while (true) {
        std::string cand = base + " (" + std::to_string(suffix++) + ")";
        if (!is_taken(cand)) return cand;
    }
}

void AeonRebellionEngine::trigger_state_splinter(int civ_id, AeonEngine& engine) {
    if (civ_id < 0 || civ_id >= (int)engine.civs.size()) return;
    auto& parent = engine.civs[civ_id];
    if (parent.is_commons || parent.is_alive <= 0.0f) return;

    // Cooldown: At least 20 years between civil war splinters for any realm
    auto it_last = civ_last_crisis_year.find(civ_id);
    if (it_last != civ_last_crisis_year.end() && (engine.year - it_last->second < 20)) {
        return;
    }

    // Do not splinter if parent is already fighting an active rebellion
    for (const auto& r : rebellions) {
        if (r.active && (r.parent_civ_id == civ_id || r.rebel_civ_id == civ_id)) {
            return;
        }
    }

    // Initialize provinces if empty
    if (parent.provinces.empty()) {
        parent.init_default_provinces();
    }

    // Cannot splinter if realm has only 1 province
    if (parent.provinces.size() <= 1) {
        return;
    }

    // 1. Identify defecting generals based on loyalty, ambition, and traits
    std::vector<int> defecting_gen_ids;
    int prime_rebel_gen_id = -1;
    float highest_ambition = -1.0f;

    for (int gid : parent.general_character_ids) {
        for (auto& ch : engine.characters) {
            if (ch.id == gid && ch.is_alive) {
                bool will_defect = false;
                if (ch.loyalty_to_ruler < 0.40f) will_defect = true;
                if (ch.political_ambition > 0.65f && parent.stability < 25.0f) will_defect = true;
                if (ch.will_refuse_order(parent.stability, parent.public_support)) will_defect = true;
                if (ch.has_trait("Politically Ambitious") && parent.stability < 35.0f) will_defect = true;

                if (will_defect) {
                    defecting_gen_ids.push_back(gid);
                    if (ch.political_ambition > highest_ambition) {
                        highest_ambition = ch.political_ambition;
                        prime_rebel_gen_id = gid;
                    }
                }
                break;
            }
        }
    }

    // If no general met defection criteria but stability is catastrophic (<15%), pick least loyal general
    if (defecting_gen_ids.empty() && !parent.general_character_ids.empty()) {
        float min_loyalty = 999.0f;
        int least_loyal_id = parent.general_character_ids.front();
        for (int gid : parent.general_character_ids) {
            for (auto& ch : engine.characters) {
                if (ch.id == gid && ch.is_alive) {
                    if (ch.loyalty_to_ruler < min_loyalty) {
                        min_loyalty = ch.loyalty_to_ruler;
                        least_loyal_id = gid;
                    }
                    break;
                }
            }
        }
        defecting_gen_ids.push_back(least_loyal_id);
        prime_rebel_gen_id = least_loyal_id;
    }

    // 2. Physical Province Partition
    int new_civ_id = static_cast<int>(engine.civs.size());
    std::vector<Province> kept_provinces;
    std::vector<Province> seceded_provinces;
    std::vector<std::string> seceded_names;

    for (size_t pidx = 0; pidx < parent.provinces.size(); ++pidx) {
        auto prov = parent.provinces[pidx];
        // Capital province (index 0) remains with parent; secede 1 or 2 other provinces
        if (pidx > 0 && seceded_provinces.size() < 2) {
            prov.civ_id = new_civ_id;
            prov.occupier_civ_id = -1;
            prov.is_occupied = false;
            prov.occupation_resistance = 0.0f;
            seceded_names.push_back(prov.name);
            seceded_provinces.push_back(prov);
        } else {
            kept_provinces.push_back(prov);
        }
    }

    if (seceded_provinces.empty()) {
        return;
    }
    parent.provinces = kept_provinces;
    parent.calculate_provincial_yields();

    std::string primary_prov_name = seceded_names.empty() ? "" : seceded_names.front();

    // 3. Determine Rebel Faction Ideology, Government Form, and Globally Unique Name
    IdeologyType rebel_ideology = IdeologyType::REPUBLICANISM;
    GovForm rebel_gov = GovForm::REPUBLIC;
    std::string ideology_str = "Democratic Resistance";

    if (prime_rebel_gen_id >= 0) {
        for (const auto& ch : engine.characters) {
            if (ch.id == prime_rebel_gen_id) {
                if (ch.political_ambition > 0.65f || ch.has_trait("Brilliant Strategist") || ch.has_trait("Politically Ambitious")) {
                    rebel_ideology = IdeologyType::MILITARISM;
                    rebel_gov = GovForm::MILITARY_JUNTA;
                    ideology_str = "Military Faction";
                }
                break;
            }
        }
    }

    if (rebel_ideology != IdeologyType::MILITARISM) {
        if (parent.government == GovForm::MONARCHY || parent.government == GovForm::EMPIRE || parent.government == GovForm::DICTATORSHIP) {
            rebel_ideology = IdeologyType::REPUBLICANISM;
            rebel_gov = GovForm::REPUBLIC;
            ideology_str = "Democratic Separatists";
        } else {
            rebel_ideology = IdeologyType::FEDERALISM;
            rebel_gov = GovForm::FEDERATION;
            ideology_str = "Regional Separatists";
        }
    }

    std::string faction_name = generate_unique_successor_name(parent, primary_prov_name, rebel_ideology, rebel_gov, engine.civs);

    // 4. Military Division Splitting & Concrete Division Instantiation
    float defected_personnel = 0.0f;
    int defected_div_count = 0;

    for (auto& div : engine.military_engine.divisions) {
        if (div.civ_id == parent.id && div.general_character_id >= 0) {
            bool gen_defected = false;
            for (int d_gid : defecting_gen_ids) {
                if (div.general_character_id == d_gid) {
                    gen_defected = true;
                    break;
                }
            }
            if (gen_defected) {
                div.civ_id = new_civ_id;
                defected_personnel += div.personnel;
                defected_div_count++;
            }
        }
    }

    // Ensure at least 30% of parent's military forces defect to rebel state
    for (auto& div : engine.military_engine.divisions) {
        if (div.civ_id == parent.id && defected_personnel < parent.army_size * 0.35f) {
            div.civ_id = new_civ_id;
            defected_personnel += div.personnel;
            defected_div_count++;
        }
    }

    if (defected_personnel < 1500.0f) {
        defected_personnel = std::max(2500.0f, parent.army_size * 0.35f);
    }

    // Concrete MilitaryDivision instantiation so division count matches troop count
    if (defected_div_count == 0) {
        MilitaryDivision new_div;
        new_div.id = static_cast<int>(engine.military_engine.divisions.size()) + 1;
        new_div.civ_id = new_civ_id;
        new_div.name = faction_name + " 1st Revolutionary Brigade";
        new_div.type = UnitType::Infantry;
        new_div.x = seceded_provinces.front().map_x;
        new_div.y = seceded_provinces.front().map_y;
        new_div.personnel = defected_personnel;
        new_div.max_personnel = defected_personnel;
        new_div.supply_level = 0.90f;
        new_div.fuel_ammo = 0.90f;
        new_div.combat_experience = 25.0f;
        new_div.general_character_id = prime_rebel_gen_id;
        engine.military_engine.divisions.push_back(new_div);
        defected_div_count = 1;
    } else {
        float total_div_personnel = 0.0f;
        for (const auto& div : engine.military_engine.divisions) {
            if (div.civ_id == new_civ_id) total_div_personnel += div.personnel;
        }
        if (total_div_personnel < defected_personnel) {
            float missing = defected_personnel - total_div_personnel;
            MilitaryDivision new_div;
            new_div.id = static_cast<int>(engine.military_engine.divisions.size()) + 1;
            new_div.civ_id = new_civ_id;
            new_div.name = faction_name + " Rebel Volunteers";
            new_div.type = UnitType::Infantry;
            new_div.x = seceded_provinces.front().map_x;
            new_div.y = seceded_provinces.front().map_y;
            new_div.personnel = missing;
            new_div.max_personnel = missing;
            new_div.supply_level = 0.85f;
            new_div.fuel_ammo = 0.85f;
            new_div.combat_experience = 15.0f;
            new_div.general_character_id = prime_rebel_gen_id;
            engine.military_engine.divisions.push_back(new_div);
            defected_div_count++;
        }
    }

    parent.army_size = std::max(1000.0f, parent.army_size - defected_personnel);
    parent.standing_army = std::max(1000.0f, parent.standing_army - defected_personnel);

    // 5. Leadership Assignment
    std::string leader_name;
    int rebel_ruler_id = -1;

    if (prime_rebel_gen_id >= 0) {
        rebel_ruler_id = prime_rebel_gen_id;
        for (auto& ch : engine.characters) {
            if (ch.id == prime_rebel_gen_id) {
                ch.civ_id = new_civ_id;
                ch.title = (rebel_gov == GovForm::MILITARY_JUNTA) ? "Lord General" : "President";
                leader_name = ch.name;
                break;
            }
        }
    } else {
        auto ruler = engine.make_ruler(engine.next_char_id++, "", new_civ_id, engine.year - 40);
        rebel_ruler_id = ruler.id;
        leader_name = ruler.name;
        engine.characters.push_back(ruler);
    }

    // Clean parent general list
    std::vector<int> parent_remaining_gens;
    for (int gid : parent.general_character_ids) {
        bool defected = false;
        for (int d_gid : defecting_gen_ids) {
            if (gid == d_gid) { defected = true; break; }
        }
        if (!defected) {
            parent_remaining_gens.push_back(gid);
        }
    }
    parent.general_character_ids = parent_remaining_gens;

    // 6. Build the New Breakaway Civilization
    AeonCivilization new_civ;
    new_civ.id = new_civ_id;
    new_civ.name = faction_name;
    new_civ.government = rebel_gov;
    new_civ.national_ideology = rebel_ideology;
    new_civ.ideology = ideology_str;
    new_civ.ruler_id = rebel_ruler_id;
    new_civ.character_ids.push_back(rebel_ruler_id);

    for (int d_gid : defecting_gen_ids) {
        new_civ.general_character_ids.push_back(d_gid);
        new_civ.character_ids.push_back(d_gid);
        for (auto& ch : engine.characters) {
            if (ch.id == d_gid) {
                ch.civ_id = new_civ_id;
                break;
            }
        }
    }

    new_civ.provinces = seceded_provinces;
    new_civ.capital_x = seceded_provinces.front().map_x;
    new_civ.capital_y = seceded_provinces.front().map_y;
    new_civ.calculate_provincial_yields();

    new_civ.army_size = defected_personnel;
    new_civ.standing_army = defected_personnel;
    new_civ.military_power = defected_personnel * 0.12f;
    new_civ.morale = 80.0f;
    new_civ.stability = 65.0f;
    new_civ.unrest = 15.0f;
    new_civ.is_alive = 1.0f;
    new_civ.is_commons = false;
    new_civ.tech = parent.tech;

    // GDP & Economy partitioning
    float gdp_share = parent.economy.gdp * 0.35f;
    parent.economy.gdp = std::max(50.0f, parent.economy.gdp - gdp_share);
    new_civ.economy.gdp = gdp_share + 50.0f;

    // 7. Foreign Recognition and Financial Sponsorship (Deduplicated by Realm ID)
    std::vector<int> recognized_by;
    std::vector<int> funded_by;
    std::unordered_map<int, float> aid_by_civ;
    float total_foreign_aid = 0.0f;

    for (size_t i = 0; i < engine.civs.size(); ++i) {
        auto& other = engine.civs[i];
        if (other.is_commons || other.is_alive <= 0.0f || other.id == parent.id || other.id == new_civ_id) continue;

        auto it_rel = other.relations.find(parent.id);
        DiplomacyStatus p_rel = (it_rel != other.relations.end()) ? it_rel->second : DiplomacyStatus::NEUTRAL;

        if (p_rel == DiplomacyStatus::ALLY) {
            // Allies of parent recognize the loyalist regime, hostile to rebels
            other.relations[new_civ_id] = DiplomacyStatus::HOSTILE;
            new_civ.relations[other.id] = DiplomacyStatus::HOSTILE;
        } else if (p_rel == DiplomacyStatus::RIVAL || p_rel == DiplomacyStatus::AT_WAR || other.bilateral_relations[parent.id].hatred > 30.0f) {
            // Rivals recognize the breakaway state and finance it
            if (std::find(recognized_by.begin(), recognized_by.end(), other.id) == recognized_by.end()) {
                recognized_by.push_back(other.id);
            }
            other.relations[new_civ_id] = DiplomacyStatus::TRADE_PARTNER;
            new_civ.relations[other.id] = DiplomacyStatus::TRADE_PARTNER;

            float aid = std::min(120.0f, other.economy.gdp * 0.12f);
            if (aid > 10.0f) {
                if (std::find(funded_by.begin(), funded_by.end(), other.id) == funded_by.end()) {
                    other.economy.gdp = std::max(50.0f, other.economy.gdp - aid);
                    new_civ.economy.gdp += aid;
                    total_foreign_aid += aid;
                    funded_by.push_back(other.id);
                    aid_by_civ[other.id] = aid;
                    new_civ.bilateral_relations[other.id].gratitude = 50.0f;
                    new_civ.bilateral_relations[other.id].trust = 40.0f;
                }
            }
        } else {
            other.relations[new_civ_id] = DiplomacyStatus::NEUTRAL;
            new_civ.relations[other.id] = DiplomacyStatus::NEUTRAL;
        }
    }

    // 8. Civil War State: Parent and Rebel Civ are locked in mortal conflict
    parent.at_war = true;
    parent.war_with_civ = new_civ_id;
    parent.war_year_start = engine.year;
    parent.relations[new_civ_id] = DiplomacyStatus::AT_WAR;
    parent.bilateral_relations[new_civ_id].hatred = 95.0f;
    parent.bilateral_relations[new_civ_id].border_claim_score = 100.0f;

    new_civ.at_war = true;
    new_civ.war_with_civ = parent.id;
    new_civ.war_year_start = engine.year;
    new_civ.relations[parent.id] = DiplomacyStatus::AT_WAR;
    new_civ.bilateral_relations[parent.id].hatred = 95.0f;
    new_civ.bilateral_relations[parent.id].border_claim_score = 100.0f;

    // Defense-in-depth: Strict clean isolated diplomatic relations (no self-entries)
    new_civ.relations.erase(new_civ_id);
    new_civ.bilateral_relations.erase(new_civ_id);
    parent.relations.erase(parent.id);
    parent.bilateral_relations.erase(parent.id);

    // 9. Register AI controller, Map City, and History
    engine.civs.push_back(new_civ);

    AeonRulerAI new_ai(new_civ_id);
    new_ai.primary_goal = "Crush loyalists of " + parent.name + " and secure sovereignty";
    new_ai.model_name = "rule_based";
    engine.ai_controllers.push_back(new_ai);

    engine.world_map.set_city(new_civ.capital_x, new_civ.capital_y, 0, '@');

    // 10. Record rebellion entry and update cooldowns
    RebelFaction rf;
    rf.id = static_cast<int>(rebellions.size()) + 1;
    rf.parent_civ_id = parent.id;
    rf.rebel_civ_id = new_civ_id;
    rf.name = faction_name;
    rf.ideology = ideology_str;
    rf.ideology_type = rebel_ideology;
    rf.strength = defected_personnel * 0.01f;
    rf.active = true;
    rf.leader_character_id = rebel_ruler_id;
    rf.leader_name = leader_name;
    rf.defecting_general_ids = defecting_gen_ids;
    rf.defected_army_size = defected_personnel;
    rf.defected_division_count = defected_div_count;
    rf.seceded_provinces = seceded_names;
    rf.recognized_by_civ_ids = recognized_by;
    rf.funded_by_civ_ids = funded_by;
    rf.foreign_financial_aid = total_foreign_aid;
    rebellions.push_back(rf);

    // Update cooldown timestamps
    civ_last_crisis_year[parent.id] = engine.year;
    civ_last_crisis_year[new_civ_id] = engine.year;
    last_crisis_year = engine.year;

    // History log
    engine.history.record(engine.year, engine.month, "CIVIL_WAR",
        "THE " + parent.name + " CRISIS: State Fracture!",
        faction_name + " [ID:" + std::to_string(new_civ_id) + "] secedes from " +
        parent.name + " [ID:" + std::to_string(parent.id) + "] under " + leader_name +
        " seizing " + std::to_string(seceded_names.size()) + " province(s) and " +
        std::to_string(static_cast<int>(defected_personnel)) + " soldiers.", parent.id, new_civ_id);

    // Terminal Announcement with permanent realm IDs
    std::cout << "\n"
              << "================================================================================\n"
              << "🔥 CIVIL WAR BREAKOUT: THE " << parent.name << " CRISIS! (Year " << engine.year << ")\n"
              << "--------------------------------------------------------------------------------\n"
              << "  Stability Collapse: " << parent.name << " [ID:" << parent.id << "] fractures into civil war!\n"
              << "  Breakaway State:    " << faction_name << " [ID:" << new_civ_id << "] [" << ideology_str << "]\n"
              << "  Rebel Commander:    " << leader_name << " (" << defecting_gen_ids.size() << " general(s) defected)\n"
              << "  Military Split:     " << defected_div_count << " division(s) (" << static_cast<int>(defected_personnel) << " troops) turned on the regime!\n"
              << "  Seceded Territory:  ";
    for (size_t s = 0; s < seceded_names.size(); ++s) {
        std::cout << seceded_names[s] << (s + 1 < seceded_names.size() ? ", " : "");
    }
    std::cout << "\n";
    if (!funded_by.empty()) {
        std::cout << "  Foreign Backing:    ";
        for (size_t f = 0; f < funded_by.size(); ++f) {
            int fid = funded_by[f];
            std::cout << engine.civs[fid].name << " [ID:" << fid << "] (provided $" << static_cast<int>(aid_by_civ[fid]) << "B aid)"
                      << (f + 1 < funded_by.size() ? ", " : "");
        }
        std::cout << "\n";
    }
    std::cout << "================================================================================\n" << std::endl;
}

void AeonRebellionEngine::update_rebellions_tick(AeonEngine& engine) {
    // 1. Clean up inactive rebellions
    for (auto& r : rebellions) {
        if (!r.active) continue;
        if (r.parent_civ_id >= 0 && r.parent_civ_id < static_cast<int>(engine.civs.size()) &&
            r.rebel_civ_id >= 0 && r.rebel_civ_id < static_cast<int>(engine.civs.size())) {
            const auto& p = engine.civs[r.parent_civ_id];
            const auto& reb = engine.civs[r.rebel_civ_id];
            if (p.is_alive <= 0.0f || reb.is_alive <= 0.0f || !p.at_war || p.war_with_civ != reb.id) {
                r.active = false;
            }
        }
    }

    // Global Cooldown: at most one major state fracture every 5 years across the entire world
    if (engine.year - last_crisis_year < 5) return;

    size_t count = engine.civs.size();
    for (size_t i = 0; i < count; ++i) {
        auto& civ = engine.civs[i];
        if (civ.is_commons || civ.is_alive <= 0.0f) continue;
        if (civ.provinces.size() <= 1) continue;

        // Per-realm Cooldown: At least 20 years between fractures for the same realm
        auto it_last = civ_last_crisis_year.find(civ.id);
        if (it_last != civ_last_crisis_year.end() && (engine.year - it_last->second < 20)) {
            continue;
        }

        // Do not fracture if this civ is currently an active belligerent in a rebellion
        bool already_rebelling = false;
        for (const auto& r : rebellions) {
            if (r.active && (r.parent_civ_id == civ.id || r.rebel_civ_id == civ.id)) {
                already_rebelling = true;
                break;
            }
        }
        if (already_rebelling) continue;

        if (civ.check_civil_war(engine.year)) {
            trigger_state_splinter(static_cast<int>(i), engine);
            break;
        }
    }
}

} // namespace Aeon
