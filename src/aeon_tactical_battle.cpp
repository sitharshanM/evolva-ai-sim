#include "aeon_tactical_battle.h"
#include "aeon_engine.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace Aeon {

const char* tactical_unit_name(TacticalUnitType type) {
    switch (type) {
        case TacticalUnitType::INFANTRY:   return "⚔️ Infantry Brigade";
        case TacticalUnitType::ARMOR:      return "🛡️ Armor Division";
        case TacticalUnitType::ARTILLERY:  return "💥 Heavy Artillery";
        case TacticalUnitType::AIR_SUPPORT:return "✈️ Air Support Squadron";
    }
    return "Regiment";
}

void AeonTacticalBattleEngine::init() {
    active_battle = false;
    units.clear();
    victor_civ_id = -1;
}

void AeonTacticalBattleEngine::start_battle(int attacker_civ, int defender_civ, const std::string& location_name, AeonEngine& engine) {
    active_battle = true;
    attacker_civ_id = attacker_civ;
    defender_civ_id = defender_civ;
    battle_location = location_name;
    current_turn_civ_id = attacker_civ;
    turn_number = 1;
    victor_civ_id = -1;
    units.clear();

    int unit_id_counter = 1;
    // Spawn Attacker forces on West side (x: 1..3)
    units.push_back({unit_id_counter++, "Attacker Infantry A", TacticalUnitType::INFANTRY,   attacker_civ, 1, 5, 100.0f, 100.0f, 30.0f, 15.0f, 1, false});
    units.push_back({unit_id_counter++, "Attacker Infantry B", TacticalUnitType::INFANTRY,   attacker_civ, 1, 10, 100.0f, 100.0f, 30.0f, 15.0f, 1, false});
    units.push_back({unit_id_counter++, "Attacker Armor Column",TacticalUnitType::ARMOR,      attacker_civ, 2, 8, 150.0f, 150.0f, 50.0f, 35.0f, 1, false});
    units.push_back({unit_id_counter++, "Attacker Battery",    TacticalUnitType::ARTILLERY,  attacker_civ, 0, 8, 80.0f,  80.0f,  45.0f, 10.0f, 3, false});
    units.push_back({unit_id_counter++, "Attacker Air Squadron",TacticalUnitType::AIR_SUPPORT,attacker_civ, 0, 2, 70.0f,  70.0f,  60.0f, 5.0f,  4, false});

    // Spawn Defender forces on East side (x: 16..18)
    units.push_back({unit_id_counter++, "Defender Infantry A", TacticalUnitType::INFANTRY,   defender_civ, 18, 5, 100.0f, 100.0f, 30.0f, 18.0f, 1, false});
    units.push_back({unit_id_counter++, "Defender Infantry B", TacticalUnitType::INFANTRY,   defender_civ, 18, 10, 100.0f, 100.0f, 30.0f, 18.0f, 1, false});
    units.push_back({unit_id_counter++, "Defender Armor Column",TacticalUnitType::ARMOR,      defender_civ, 17, 8, 150.0f, 150.0f, 50.0f, 38.0f, 1, false});
    units.push_back({unit_id_counter++, "Defender Battery",    TacticalUnitType::ARTILLERY,  defender_civ, 19, 8, 80.0f,  80.0f,  45.0f, 10.0f, 3, false});
    units.push_back({unit_id_counter++, "Defender Air Squadron",TacticalUnitType::AIR_SUPPORT,defender_civ, 19, 15, 70.0f, 70.0f,  60.0f, 5.0f,  4, false});

    engine.history.record(engine.year, engine.month, "WAR",
        "Tactical Battle Engaged at " + location_name,
        engine.civs[attacker_civ].name + " vs " + engine.civs[defender_civ].name + " on 20x20 battlefield grid.", attacker_civ, defender_civ);

    std::cout << "[TACTICAL BATTLE] Commenced at " << location_name << std::endl;
}

bool AeonTacticalBattleEngine::move_unit(int unit_id, int new_x, int new_y) {
    if (!active_battle) return false;
    new_x = std::max(0, std::min(19, new_x));
    new_y = std::max(0, std::min(19, new_y));

    for (auto& u : units) {
        if (u.id == unit_id && u.hp > 0.0f && u.civ_id == current_turn_civ_id && !u.has_moved) {
            u.x = new_x;
            u.y = new_y;
            u.has_moved = true;
            return true;
        }
    }
    return false;
}

bool AeonTacticalBattleEngine::attack_unit(int attacker_unit_id, int target_unit_id, AeonEngine& engine) {
    if (!active_battle) return false;

    TacticalUnit* atk = nullptr;
    TacticalUnit* tgt = nullptr;

    for (auto& u : units) {
        if (u.id == attacker_unit_id) atk = &u;
        if (u.id == target_unit_id) tgt = &u;
    }

    if (!atk || !tgt || atk->hp <= 0.0f || tgt->hp <= 0.0f) return false;
    if (atk->civ_id != current_turn_civ_id || atk->civ_id == tgt->civ_id) return false;

    int dist = std::abs(atk->x - tgt->x) + std::abs(atk->y - tgt->y);
    if (dist > atk->attack_range) return false;

    float damage = std::max(5.0f, atk->attack_power - tgt->defense * 0.4f);
    tgt->hp -= damage;

    std::cout << "[TACTICAL BATTLE] " << atk->name << " dealt " << int(damage) << " DMG to " << tgt->name << std::endl;

    // Check if target eliminated
    if (tgt->hp <= 0.0f) {
        tgt->hp = 0.0f;
    }

    // Check battle victory conditions
    int atk_alive = 0, def_alive = 0;
    for (const auto& u : units) {
        if (u.hp > 0.0f) {
            if (u.civ_id == attacker_civ_id) atk_alive++;
            if (u.civ_id == defender_civ_id) def_alive++;
        }
    }

    if (atk_alive == 0 || def_alive == 0) {
        victor_civ_id = (atk_alive > 0) ? attacker_civ_id : defender_civ_id;
        active_battle = false;
        engine.history.record(engine.year, engine.month, "WAR",
            "Tactical Battle Decided at " + battle_location,
            engine.civs[victor_civ_id].name + " claims tactical battlefield victory!", victor_civ_id);
    }

    return true;
}

void AeonTacticalBattleEngine::resolve_turn(AeonEngine& engine) {
    if (!active_battle) return;

    // Toggle turn to opponent
    current_turn_civ_id = (current_turn_civ_id == attacker_civ_id) ? defender_civ_id : attacker_civ_id;
    if (current_turn_civ_id == attacker_civ_id) turn_number++;

    for (auto& u : units) {
        u.has_moved = false;
    }

    // AI automated move for non-player turn
    for (auto& u : units) {
        if (u.civ_id == current_turn_civ_id && u.hp > 0.0f) {
            // Find closest enemy
            TacticalUnit* closest_enemy = nullptr;
            int min_d = 999;
            for (auto& enemy : units) {
                if (enemy.civ_id != current_turn_civ_id && enemy.hp > 0.0f) {
                    int d = std::abs(u.x - enemy.x) + std::abs(u.y - enemy.y);
                    if (d < min_d) {
                        min_d = d;
                        closest_enemy = &enemy;
                    }
                }
            }

            if (closest_enemy) {
                if (min_d <= u.attack_range) {
                    attack_unit(u.id, closest_enemy->id, engine);
                } else {
                    int dx = (closest_enemy->x > u.x) ? 1 : (closest_enemy->x < u.x ? -1 : 0);
                    int dy = (closest_enemy->y > u.y) ? 1 : (closest_enemy->y < u.y ? -1 : 0);
                    move_unit(u.id, u.x + dx, u.y + dy);
                }
            }
        }
    }
}

} // namespace Aeon
