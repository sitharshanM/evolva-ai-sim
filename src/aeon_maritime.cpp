#include "aeon_maritime.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

AeonMaritimeEngine::AeonMaritimeEngine() {
}

void AeonMaritimeEngine::init_maritime(const AeonEngine& engine) {
    cargo_ships.clear();
    active_blockades.clear();
    pirate_havens.clear();
    privateers.clear();

    // Spawn 3 historical & modern pirate havens along major maritime chokepoints
    pirate_havens.push_back({1, "Caribbean Tortuga Haven", 105, 72, 250, 8000.0f, false});
    pirate_havens.push_back({2, "Gulf of Aden Anchorage", 225, 80, 320, 14000.0f, false});
    pirate_havens.push_back({3, "Strait of Malacca Corsairs", 283, 88, 280, 11000.0f, false});

    for (const auto& civ : engine.civs) {
        if (civ.is_commons) continue;
        MaritimeCargoShip cs;
        cs.id = (int)cargo_ships.size() + 1;
        cs.owner_civ_id = civ.id;
        cs.origin_port = civ.name + " Deepwater Port";
        cs.dest_port = "Global Maritime Nexus (Rotterdam/Singapore)";
        cs.x = civ.capital_x;
        cs.y = civ.capital_y;
        cs.cargo_value_gold = 8500.0 + (civ.id * 1200.0);
        cs.intercepted_by_pirates = false;
        cs.sunk_by_storm = false;
        cargo_ships.push_back(cs);
    }

}

void AeonMaritimeEngine::enact_naval_blockade(int attacker_civ_id, int target_civ_id, AeonEngine& engine) {
    if (attacker_civ_id < 0 || attacker_civ_id >= (int)engine.civs.size()) return;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return;

    auto& attacker = engine.civs[attacker_civ_id];
    auto& target = engine.civs[target_civ_id];

    NavalBlockade nb;
    nb.attacker_civ_id = attacker_civ_id;
    nb.target_civ_id = target_civ_id;
    nb.port_name = target.name + " Port";
    nb.active = true;
    active_blockades.push_back(nb);

    engine.history.record(engine.year, engine.month, "BLOCKADE",
        attacker.name + " Enacts Naval Blockade on " + target.name + "!",
        "Warships blockade ocean trade lanes, choking off maritime commodity imports.", attacker_civ_id);

    std::cout << "[YEAR " << engine.year << "] ⛵ NAVAL BLOCKADE: " << attacker.name
              << " blockades " << target.name << " Port!" << std::endl;
}

void AeonMaritimeEngine::issue_letter_of_marque(int patron_civ, int target_civ) {
    for (auto& lm : privateers) {
        if (lm.patron_civ_id == patron_civ && lm.target_civ_id == target_civ) {
            lm.active = true;
            return;
        }
    }
    privateers.push_back({patron_civ, target_civ, 40.0f, true});
}

void AeonMaritimeEngine::launch_anti_piracy_expedition(int haven_id, int attacking_civ_id, AeonEngine& engine) {
    for (auto& h : pirate_havens) {
        if (h.id == haven_id && !h.destroyed) {
            h.destroyed = true;
            if (attacking_civ_id >= 0 && attacking_civ_id < (int)engine.civs.size()) {
                auto& civ = engine.civs[attacking_civ_id];
                civ.economy.annual_income += h.accumulated_loot;
                civ.cultural_prestige += 10.0f;
                engine.history.record(engine.year, engine.month, "PIRACY",
                    h.name + " Sacked & Destroyed!",
                    civ.name + " naval armada eradicated the pirate enclave and seized " + std::to_string((int)h.accumulated_loot) + " Gold loot.", attacking_civ_id);
            }
            break;
        }
    }
}

void AeonMaritimeEngine::update_maritime_tick(AeonEngine& engine) {
    for (auto& cs : cargo_ships) {
        cs.x = (cs.x + 2) % MAP_WIDTH;

        // Oceanic storm hazard (3% chance)
        if ((rand() % 100) < 3 && !cs.sunk_by_storm) {
            cs.sunk_by_storm = true;
            if (cs.owner_civ_id >= 0 && cs.owner_civ_id < (int)engine.civs.size()) {
                engine.history.record(engine.year, engine.month, "DISASTER",
                    "Maritime Vessel Lost in Storm",
                    "Merchant convoy was swallowed by an oceanic hurricane.", cs.owner_civ_id);
            }
            continue;
        }
        
        // Check for pirate or privateer raids
        if ((rand() % 100) < 10 && !cs.intercepted_by_pirates && !cs.sunk_by_storm) {
            cs.intercepted_by_pirates = true;
            
            // Check if a privateer state receives a cut
            for (const auto& pm : privateers) {
                if (pm.active && pm.target_civ_id == cs.owner_civ_id) {
                    if (pm.patron_civ_id >= 0 && pm.patron_civ_id < (int)engine.civs.size()) {
                        engine.civs[pm.patron_civ_id].economy.annual_income += cs.cargo_value_gold * (pm.bounty_cut_pct / 100.0f);
                    }
                    break;
                }
            }

            if (cs.owner_civ_id >= 0 && cs.owner_civ_id < (int)engine.civs.size()) {
                auto& owner = engine.civs[cs.owner_civ_id];
                engine.history.record(engine.year, engine.month, "PIRACY",
                    "Pirates Raid " + owner.name + " Cargo Vessel!",
                    "Merchant ship intercepted along oceanic shipping lane.", cs.owner_civ_id);
            }
        }
    }
}


} // namespace Aeon

