#include "aeon_nuclear.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

AeonNuclearEngine::AeonNuclearEngine() {
}

void AeonNuclearEngine::init_arsenals(const AeonEngine& engine) {
    nuclear_arsenals.clear();
    for (const auto& civ : engine.civs) {
        if (civ.is_commons) continue;
        TriadNuclearState ns;
        ns.civ_id = civ.id;
        ns.civ_name = civ.name;
        ns.icbm_silos = 12 + (civ.id * 4);
        ns.stealth_bombers = 6 + (civ.id * 2);
        ns.ssbn_submarines = 4 + civ.id;
        ns.warheads_total = (ns.icbm_silos * 4) + (ns.stealth_bombers * 2) + (ns.ssbn_submarines * 8);
        ns.triad_complete = (ns.icbm_silos > 0 && ns.stealth_bombers > 0 && ns.ssbn_submarines > 0);
        nuclear_arsenals.push_back(ns);
    }
}

void AeonNuclearEngine::launch_nuclear_strike(int attacker_civ_id, int target_civ_id, AeonEngine& engine) {
    if (attacker_civ_id < 0 || attacker_civ_id >= (int)engine.civs.size()) return;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return;

    auto& attacker = engine.civs[attacker_civ_id];
    auto& target = engine.civs[target_civ_id];

    // Trigger Nuclear Winter
    global_nuclear_winter = true;
    nuclear_winter_years_left = 5;

    // Destroy target capital area stability & population
    target.stability = std::max(5.0f, target.stability - 60.0f);
    target.army_size = std::max(500.0f, target.army_size * 0.40f);

    engine.history.record(engine.year, engine.month, "NUCLEAR_STRIKE",
        attacker.name + " Launches Nuclear Strike on " + target.name + "!",
        "Thermonuclear warheads detonate over capital sectors, triggering Global Nuclear Winter.", attacker_civ_id);

    std::cout << "[YEAR " << engine.year << "] ☢️ NUCLEAR DETONATION: " << attacker.name
              << " strikes " << target.name << "! Global Nuclear Winter initiated!" << std::endl;

    // Trigger Second Strike MAD retaliation if target possesses SSBN nuclear submarines
    const auto* target_arsenal = get_arsenal(target_civ_id);
    if (target_arsenal && target_arsenal->ssbn_submarines > 0) {
        execute_mad_retaliation(target_civ_id, attacker_civ_id, engine);
    }
}

void AeonNuclearEngine::execute_mad_retaliation(int defender_civ_id, int original_attacker_id, AeonEngine& engine) {
    if (defender_civ_id < 0 || defender_civ_id >= (int)engine.civs.size()) return;
    if (original_attacker_id < 0 || original_attacker_id >= (int)engine.civs.size()) return;

    auto& defender = engine.civs[defender_civ_id];
    auto& attacker = engine.civs[original_attacker_id];

    // Attacker takes severe retaliatory counter-value damage
    attacker.stability = std::max(5.0f, attacker.stability - 50.0f);
    attacker.army_size = std::max(500.0f, attacker.army_size * 0.45f);

    engine.history.record(engine.year, engine.month, "NUCLEAR_RETALIATION",
        "M.A.D. Second-Strike Retaliation!",
        defender.name + " SSBN submarines launch retaliatory thermonuclear salvo into " + attacker.name + "!", defender_civ_id);
}

void AeonNuclearEngine::update_nuclear_tick(AeonEngine& engine) {
    // Tick down nuclear winter
    if (global_nuclear_winter) {
        if (nuclear_winter_years_left > 0) {
            nuclear_winter_years_left--;
            // Lower global temperature in climate engine for this active winter year
            for (auto& cell : engine.gis_climate_engine.grid) {
                cell.temperature_c -= 12.0f; // 12°C cooling
            }
        } else {
            global_nuclear_winter = false;
            // Restore climate baseline temperature when nuclear winter clears
            for (auto& cell : engine.gis_climate_engine.grid) {
                cell.temperature_c += 60.0f;
            }
        }
    }
}

const TriadNuclearState* AeonNuclearEngine::get_arsenal(int civ_id) const {
    for (const auto& ns : nuclear_arsenals) {
        if (ns.civ_id == civ_id) return &ns;
    }
    return nullptr;
}

} // namespace Aeon

