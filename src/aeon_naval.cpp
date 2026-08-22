#include "aeon_naval.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

void AeonNavalEngine::init() {
    fleets.clear();
    offshore_oil_rigs = 2;

    NavalFleet f1;
    f1.id = 1;
    f1.civ_id = 0;
    f1.destroyers = 4;
    f1.submarines = 2;
    f1.aircraft_carriers = 1;
    f1.is_blockading = false;
    fleets.push_back(f1);
}

void AeonNavalEngine::tick_year(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (!p.active || p.player_civ_id < 0 || p.player_civ_id >= (int)engine.civs.size()) return;

    auto& civ = engine.civs[p.player_civ_id];
    (void)civ;

    // Income from offshore oil rigs
    if (offshore_oil_rigs > 0) {
        float oil_revenue = offshore_oil_rigs * 8000.0f;
        p.treasury_gold += oil_revenue;
    }

    // Process Maritime Blockades
    for (auto& f : fleets) {
        if (f.is_blockading && f.blockaded_civ_id >= 0 && f.blockaded_civ_id < (int)engine.civs.size()) {
            auto& enemy = engine.civs[f.blockaded_civ_id];
            enemy.economy.gdp *= 0.95f; // 5% GDP reduction per year from trade embargo
            enemy.stability = std::max(0.0f, enemy.stability - 3.0f);
        }
    }
}

bool AeonNavalEngine::build_warship(AeonEngine& engine, const std::string& ship_type) {
    auto& p = engine.president_game;
    if (p.player_civ_id < 0 || p.player_civ_id >= (int)engine.civs.size()) return false;

    NavalFleet* player_fleet = nullptr;
    for (auto& f : fleets) {
        if (f.civ_id == p.player_civ_id) { player_fleet = &f; break; }
    }
    if (!player_fleet) {
        NavalFleet nf;
        nf.id = static_cast<int>(fleets.size() + 1);
        nf.civ_id = p.player_civ_id;
        fleets.push_back(nf);
        player_fleet = &fleets.back();
    }

    auto& fleet = *player_fleet;
    if (ship_type == "SUBMARINE") {
        if (p.treasury_gold < 25000.0f) return false;
        p.treasury_gold -= 25000.0f;
        fleet.submarines++;
        p.last_news_headline = "NAVAL EXPANSION: Commissioned Nuclear Attack Submarine ($25,000 cost).";
    } else if (ship_type == "CARRIER") {
        if (p.treasury_gold < 75000.0f) return false;
        p.treasury_gold -= 75000.0f;
        fleet.aircraft_carriers++;
        p.last_news_headline = "NAVAL EXPANSION: Supercarrier warship deployed to Fleet ($75,000 cost).";
    } else { // Destroyer
        if (p.treasury_gold < 15000.0f) return false;
        p.treasury_gold -= 15000.0f;
        fleet.destroyers++;
        p.last_news_headline = "NAVAL EXPANSION: Guided Missile Destroyer commissioned ($15,000 cost).";
    }

    auto& civ = engine.civs[p.player_civ_id];
    civ.military_power += 250.0f;
    return true;
}

bool AeonNavalEngine::enact_sea_blockade(AeonEngine& engine, int target_civ_id) {
    auto& p = engine.president_game;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;

    NavalFleet* player_fleet = nullptr;
    for (auto& f : fleets) {
        if (f.civ_id == p.player_civ_id) { player_fleet = &f; break; }
    }
    if (!player_fleet) {
        NavalFleet nf;
        nf.id = static_cast<int>(fleets.size() + 1);
        nf.civ_id = p.player_civ_id;
        fleets.push_back(nf);
        player_fleet = &fleets.back();
    }

    player_fleet->is_blockading = true;
    player_fleet->blockaded_civ_id = target_civ_id;

    auto& target = engine.civs[target_civ_id];
    engine.president_game.last_news_headline = "MARITIME BLOCKADE: Naval armada blockades foreign ports of " + target.name + "!";
    return true;
}

bool AeonNavalEngine::construct_offshore_rig(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 30000.0f) return false;

    p.treasury_gold -= 30000.0f;
    offshore_oil_rigs++;
    p.last_news_headline = "OFFSHORE ENERGY: Constructed Deep-Sea Oil Rig Complex #" + std::to_string(offshore_oil_rigs) + " ($30,000 cost).";
    return true;
}

} // namespace Aeon
