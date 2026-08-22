#include "aeon_military.h"
#include "aeon_engine.h"
#include <algorithm>
#include <cmath>

namespace Aeon {

AeonMilitaryEngine::AeonMilitaryEngine() {
}

void AeonMilitaryEngine::init_default_forces(const AeonEngine& engine) {
    divisions.clear();
    frontlines.clear();

    int next_id = 1;
    for (const auto& civ : engine.civs) {
        if (civ.is_commons) continue;

        MilitaryDivision div1;
        div1.id = next_id++;
        div1.civ_id = civ.id;
        div1.name = civ.name + " 1st Infantry Division";
        div1.type = UnitType::Infantry;
        div1.x = civ.capital_x;
        div1.y = civ.capital_y;
        div1.personnel = 15000.0f;
        div1.max_personnel = 15000.0f;
        div1.supply_level = 1.0f;
        div1.fuel_ammo = 1.0f;
        divisions.push_back(div1);

        MilitaryDivision div2;
        div2.id = next_id++;
        div2.civ_id = civ.id;
        div2.name = civ.name + " Armor Corps";
        div2.type = UnitType::ArmoredBrigade;
        div2.x = std::max(0, civ.capital_x + 1);
        div2.y = civ.capital_y;
        div2.personnel = 8000.0f;
        div2.max_personnel = 8000.0f;
        div2.supply_level = 0.95f;
        div2.fuel_ammo = 0.90f;
        divisions.push_back(div2);

        MilitaryDivision div3;
        div3.id = next_id++;
        div3.civ_id = civ.id;
        div3.name = civ.name + " Air Wing 1";
        div3.type = UnitType::AirSquadron;
        div3.x = civ.capital_x;
        div3.y = std::max(0, civ.capital_y + 1);
        div3.personnel = 3000.0f;
        div3.max_personnel = 3000.0f;
        div3.supply_level = 1.0f;
        div3.fuel_ammo = 1.0f;
        divisions.push_back(div3);
    }
}

void AeonMilitaryEngine::update_military_tick(AeonEngine& engine) {
    frontlines.clear();

    // 1. Supply & Fuel Consumption / Attrition
    for (auto& div : divisions) {
        // Distance to capital
        int cap_x = 16, cap_y = 16;
        for (const auto& civ : engine.civs) {
            if (civ.id == div.civ_id) {
                cap_x = civ.capital_x;
                cap_y = civ.capital_y;
                break;
            }
        }

        float dist = (float)(std::abs(div.x - cap_x) + std::abs(div.y - cap_y));
        float max_supply_range = 15.0f;

        if (dist > max_supply_range) {
            // Cut off from main supply lines -> Attrition
            div.supply_level = std::max(0.1f, div.supply_level - 0.15f);
            div.fuel_ammo = std::max(0.1f, div.fuel_ammo - 0.20f);
            div.personnel = std::max(100.0f, div.personnel * 0.95f);
        } else {
            // Replenish
            div.supply_level = std::min(1.0f, div.supply_level + 0.10f);
            div.fuel_ammo = std::min(1.0f, div.fuel_ammo + 0.10f);
            div.personnel = std::min(div.max_personnel, div.personnel + 200.0f);
            div.entrenchment = std::min(100.0f, div.entrenchment + 5.0f);
        }
    }

    // 2. Identify Frontline Zones between warring civs & resolve engagement damage
    for (size_t i = 0; i < engine.civs.size(); ++i) {
        for (size_t j = i + 1; j < engine.civs.size(); ++j) {
            const auto& c1 = engine.civs[i];
            const auto& c2 = engine.civs[j];
            if (c1.is_commons || c2.is_commons) continue;

            if (c1.at_war && c1.war_with_civ == c2.id) {
                FrontlineZone fz;
                fz.civ1_id = c1.id;
                fz.civ2_id = c2.id;
                fz.center_x = (c1.capital_x + c2.capital_x) / 2;
                fz.center_y = (c1.capital_y + c2.capital_y) / 2;
                fz.intensity = 0.85f;
                frontlines.push_back(fz);

                // Combat damage between nearby opposing divisions
                for (auto& d1 : divisions) {
                    if (d1.civ_id != c1.id) continue;
                    for (auto& d2 : divisions) {
                        if (d2.civ_id != c2.id) continue;

                        int dx = std::abs(d1.x - d2.x);
                        int dy = std::abs(d1.y - d2.y);
                        if (dx <= 2 && dy <= 2) {
                            d1.in_combat = true;
                            d2.in_combat = true;
                            float dmg1 = (d2.personnel * 0.05f) * (1.0f - (d1.entrenchment / 200.0f));
                            float dmg2 = (d1.personnel * 0.05f) * (1.0f - (d2.entrenchment / 200.0f));
                            d1.personnel = std::max(0.0f, d1.personnel - dmg1);
                            d2.personnel = std::max(0.0f, d2.personnel - dmg2);
                            d1.combat_experience = std::min(100.0f, d1.combat_experience + 2.0f);
                            d2.combat_experience = std::min(100.0f, d2.combat_experience + 2.0f);
                        }
                    }
                }
            }
        }
    }
}

const char* AeonMilitaryEngine::get_unit_type_name(UnitType type) const {
    switch (type) {
        case UnitType::Infantry:       return "Infantry Division";
        case UnitType::ArmoredBrigade: return "Armored Brigade";
        case UnitType::AirSquadron:    return "Air Squadron";
        case UnitType::NavalFleet:     return "Naval Fleet";
        default: return "Military Unit";
    }
}

float AeonMilitaryEngine::get_civ_total_military_power(int civ_id) const {
    float total = 0.0f;
    for (const auto& div : divisions) {
        if (div.civ_id == civ_id) {
            total += div.personnel * div.supply_level * div.fuel_ammo;
        }
    }
    return total;
}

void AeonMilitaryEngine::execute_airstrike(int division_id, int target_x, int target_y, AeonEngine& engine) {
    for (auto& div : divisions) {
        if (div.id == division_id && div.type == UnitType::AirSquadron && div.fuel_ammo >= 0.3f) {
            div.fuel_ammo -= 0.3f;
            for (auto& target : divisions) {
                if (target.civ_id != div.civ_id && std::abs(target.x - target_x) <= 1 && std::abs(target.y - target_y) <= 1) {
                    float strike_dmg = 2000.0f * (1.0f + div.combat_experience / 50.0f);
                    target.personnel = std::max(0.0f, target.personnel - strike_dmg);
                    target.entrenchment = std::max(0.0f, target.entrenchment - 30.0f);
                    engine.history.record(engine.year, engine.month, "AIR_STRIKE",
                        "Air Strike Executed by " + div.name,
                        "Heavy tactical bombing raid conducted on enemy positions at (" + std::to_string(target_x) + ", " + std::to_string(target_y) + ").", div.civ_id);
                }
            }
            break;
        }
    }
}

void AeonMilitaryEngine::relocate_division(int division_id, int new_x, int new_y, AeonEngine& engine) {
    (void)engine;
    for (auto& div : divisions) {
        if (div.id == division_id) {
            div.x = std::max(0, std::min(MAP_WIDTH - 1, new_x));
            div.y = std::max(0, std::min(MAP_HEIGHT - 1, new_y));
            div.entrenchment = 0.0f; // reset entrenchment on relocation
            break;
        }
    }
}

// ─── get_terrain_modifiers ────────────────────────────────────────────────────
TerrainCombatModifiers get_terrain_modifiers(const std::string& terrain_type) {
    TerrainCombatModifiers m;
    if (terrain_type == "Alpine Peak") {
        m.attacker_mult = 0.60f; m.defender_mult = 1.50f;
        m.supply_cost   = 2.00f; m.siege_speed   = 0.50f;
    } else if (terrain_type == "Highland Hills") {
        m.attacker_mult = 0.80f; m.defender_mult = 1.25f;
        m.supply_cost   = 1.50f; m.siege_speed   = 0.75f;
    } else if (terrain_type == "Coastal Lowlands") {
        m.attacker_mult = 1.10f; m.defender_mult = 0.90f;
        m.supply_cost   = 0.80f; m.siege_speed   = 1.20f;
    } else if (terrain_type == "Forest" || terrain_type == "Jungle") {
        m.attacker_mult = 0.70f; m.defender_mult = 1.40f;
        m.supply_cost   = 1.80f; m.siege_speed   = 0.60f;
    } else if (terrain_type == "Desert" || terrain_type == "Steppe") {
        m.attacker_mult = 0.90f; m.defender_mult = 0.85f;
        m.supply_cost   = 1.60f; m.siege_speed   = 1.10f;
    }
    // Plains / default: all 1.0f
    return m;
}

// ─── calculate_tech_era_combat_bonus ─────────────────────────────────────────
float AeonMilitaryEngine::calculate_tech_era_combat_bonus(TechEra attacker_era, TechEra defender_era) const {
    int diff = (int)attacker_era - (int)defender_era;
    // Each era gap is worth +20% combat efficiency
    return 1.0f + (float)std::max(-2, std::min(3, diff)) * 0.20f;
}

// ─── simulate_war_engagement ──────────────────────────────────────────────────
void AeonMilitaryEngine::simulate_war_engagement(AeonCivilization& attacker, AeonCivilization& defender, AeonEngine& engine) {
    // Terrain: use defender's capital tile from the climate grid
    std::string terrain = "Plains";
    if (!engine.gis_climate_engine.grid.empty()) {
        int idx = defender.capital_y * engine.gis_climate_engine.map_width + defender.capital_x;
        if (idx >= 0 && idx < (int)engine.gis_climate_engine.grid.size())
            terrain = engine.gis_climate_engine.grid[idx].terrain_type;
    }
    TerrainCombatModifiers tcm = get_terrain_modifiers(terrain);

    float tech_bonus = calculate_tech_era_combat_bonus(attacker.tech.era, defender.tech.era);

    float att_power = attacker.military_power * tcm.attacker_mult * tech_bonus;
    float def_power = defender.military_power * tcm.defender_mult;

    float att_loss_ratio = def_power / (att_power + def_power + 1.0f);
    float def_loss_ratio = att_power / (att_power + def_power + 1.0f);

    attacker.army_size *= (1.0f - att_loss_ratio * 0.12f);
    defender.army_size *= (1.0f - def_loss_ratio * 0.12f);
    attacker.military_power *= (1.0f - att_loss_ratio * 0.08f);
    defender.military_power *= (1.0f - def_loss_ratio * 0.08f);

    // War exhaustion accumulates per year of engagement
    attacker.war_exhaustion += 3.0f + att_loss_ratio * 8.0f;
    defender.war_exhaustion += 2.0f + def_loss_ratio * 8.0f;
    attacker.war_exhaustion = std::min(100.0f, attacker.war_exhaustion);
    defender.war_exhaustion = std::min(100.0f, defender.war_exhaustion);

    // High exhaustion collapses morale and stability
    if (attacker.war_exhaustion > 75.0f) {
        attacker.morale   -= 5.0f;
        attacker.stability -= 3.0f;
        attacker.population.happiness -= 4.0f;
    }
    if (defender.war_exhaustion > 75.0f) {
        defender.morale   -= 5.0f;
        defender.stability -= 3.0f;
        defender.population.happiness -= 4.0f;
    }
}

// ─── tick_supply_lines ────────────────────────────────────────────────────────
void AeonMilitaryEngine::tick_supply_lines(AeonEngine& engine) {
    for (auto& div : divisions) {
        if (!div.deployed) continue;
        // Overextended supply: distance from home capital
        AeonCivilization* civ_ptr = nullptr;
        for (auto& civ : engine.civs) {
            if (civ.id == div.civ_id) { civ_ptr = &civ; break; }
        }
        if (!civ_ptr) continue;

        int dx = div.x - civ_ptr->capital_x;
        int dy = div.y - civ_ptr->capital_y;
        float dist = std::sqrt((float)(dx*dx + dy*dy));

        // Supply stress grows with distance; war exhaustion amplifies it
        float stress = (dist / 20.0f) + (civ_ptr->war_exhaustion / 200.0f);
        div.supply_level = std::max(0.1f, div.supply_level - stress * 0.01f);

        // Guerrilla sabotage burns extra supply
        for (const auto& cell : guerrilla_cells) {
            if (cell.occupier_civ_id == div.civ_id) {
                div.supply_level -= cell.sabotage_rate;
                div.supply_level = std::max(0.05f, div.supply_level);
            }
        }

        // Low supply means attrition even without combat
        if (div.supply_level < 0.3f) {
            div.personnel *= 0.98f; // 2% attrition per year
        }
    }
}

// ─── resolve_sieges ───────────────────────────────────────────────────────────
void AeonMilitaryEngine::resolve_sieges(AeonEngine& engine) {
    for (auto& div : divisions) {
        if (div.siege_target_city_id < 0) continue;
        // Cities are tracked by ID on the civilization; apply siege pressure as
        // a stability/food penalty on the defending civ until it falls
        for (auto& civ : engine.civs) {
            if (civ.id == div.civ_id) continue; // own civ — not besieging yourself
            // Check if this civ owns the target city
            bool owns = false;
            for (int cid : civ.city_ids) {
                if (cid == div.siege_target_city_id) { owns = true; break; }
            }
            if (!owns) continue;
            // Apply annual siege pressure
            civ.population.food_security -= 6.0f;
            civ.stability               -= 2.0f;
            // City captured when food_security exhausted
            if (civ.population.food_security < 5.0f) {
                // Transfer city to besieger by removing from defender's city_ids
                civ.city_ids.erase(
                    std::remove(civ.city_ids.begin(), civ.city_ids.end(), div.siege_target_city_id),
                    civ.city_ids.end());
                // Add to attacker's city_ids
                for (auto& attacker : engine.civs) {
                    if (attacker.id == div.civ_id) {
                        attacker.city_ids.push_back(div.siege_target_city_id);
                        break;
                    }
                }
                engine.history.record(engine.year, engine.month, "MILITARY",
                    "City #" + std::to_string(div.siege_target_city_id) + " falls by siege",
                    civ.name + " starved into surrender.", div.civ_id);
                div.siege_target_city_id = -1;
            }
            break;
        }
    }
}

// ─── check_for_guerrilla_uprising ────────────────────────────────────────────
void AeonMilitaryEngine::check_for_guerrilla_uprising(int occupied_civ_id, int occupier_civ_id, AeonEngine& engine) {
    // Look for existing cell
    for (auto& cell : guerrilla_cells) {
        if (cell.native_civ_id == occupied_civ_id && cell.occupier_civ_id == occupier_civ_id) {
            cell.strength   += 50; // grows over time under occupation
            cell.active_years++;
            return;
        }
    }
    // Spawn new cell if occupation has lasted 2+ years
    // (checked externally by engine)
    // Use active_events to check for ongoing WAR events
    for (const auto& ev : engine.active_events) {
        if (ev.type == "WAR" &&
            (ev.civ_id == occupier_civ_id || ev.civ2_id == occupier_civ_id)) {
            GuerrillaCell cell;
            cell.native_civ_id   = occupied_civ_id;
            cell.occupier_civ_id = occupier_civ_id;
            cell.strength        = 150;
            cell.sabotage_rate   = 0.04f;
            guerrilla_cells.push_back(cell);
            if (occupied_civ_id >= 0 && occupied_civ_id < (int)engine.civs.size()) {
                engine.history.record(engine.year, engine.month, "MILITARY",
                    engine.civs[occupied_civ_id].name + " resistance forms",
                    "Guerrilla cells begin sabotaging occupier supply lines.", occupied_civ_id);
            }
            break;
        }
    }
}

} // namespace Aeon
