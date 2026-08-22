#include "aeon_space_race.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

AeonSpaceRaceEngine::AeonSpaceRaceEngine() {
}

void AeonSpaceRaceEngine::init_programs(const AeonEngine& engine) {
    programs.clear();
    for (const auto& civ : engine.civs) {
        if (civ.is_commons) continue;
        EmpireSpaceProgram sp;
        sp.civ_id = civ.id;
        sp.civ_name = civ.name;
        sp.highest_milestone = (civ.id == 0 || civ.id == 1) ? SpaceMilestone::LUNAR_BASE : SpaceMilestone::ORBITAL_SATELLITE; // Nordra & Eldoria space pioneers

        sp.space_budget_millions = 1200.0f + (civ.id * 300.0f);
        sp.lunar_colonists = (sp.highest_milestone >= SpaceMilestone::LUNAR_BASE) ? 450 : 0;

        sp.mars_colonists = 0;
        sp.helium3_mined_tons = (sp.highest_milestone >= SpaceMilestone::LUNAR_BASE) ? 120.0 : 0.0;
        sp.titanium_mined_tons = 350.0;
        programs.push_back(sp);
    }
}

void AeonSpaceRaceEngine::update_space_race_tick(AeonEngine& engine) {
    for (auto& sp : programs) {
        // Mining payload growth
        if (sp.highest_milestone >= SpaceMilestone::LUNAR_BASE) {
            sp.helium3_mined_tons += 45.0;
            sp.lunar_colonists += 35;
        }
        if (sp.highest_milestone >= SpaceMilestone::MARS_COLONY) {
            sp.titanium_mined_tons += 180.0;
            sp.mars_colonists += 15;
        }

        // Advance milestones if budget and tech are high
        for (auto& civ : engine.civs) {
            if (civ.id == sp.civ_id) {
                if (civ.tech.progress > 80.0f && sp.highest_milestone == SpaceMilestone::ORBITAL_SATELLITE) {
                    sp.highest_milestone = SpaceMilestone::LUNAR_BASE;
                    sp.lunar_colonists = 100;
                    engine.history.record(engine.year, engine.month, "SPACE",
                        civ.name + " Establishes Lunar Colony!",
                        "First permanent off-world habitat founded on Lunar Surface.", civ.id);
                } else if (civ.tech.progress > 95.0f && sp.highest_milestone == SpaceMilestone::LUNAR_BASE) {
                    sp.highest_milestone = SpaceMilestone::MARS_COLONY;
                    sp.mars_colonists = 50;
                    engine.history.record(engine.year, engine.month, "SPACE",
                        civ.name + " Lands on Mars!",
                        "Red Planet colony established with orbital payload transport.", civ.id);
                }
                break;
            }
        }
    }
}

const char* AeonSpaceRaceEngine::get_milestone_name(SpaceMilestone m) const {
    switch (m) {
        case SpaceMilestone::ORBITAL_SATELLITE: return "Orbital Satellite";
        case SpaceMilestone::LUNAR_BASE:        return "Lunar Colony";
        case SpaceMilestone::MARS_COLONY:       return "Mars Habitat";
        case SpaceMilestone::ASTEROID_MINING:   return "Asteroid Mining Grid";
        default: return "Space Program";
    }
}

bool AeonSpaceRaceEngine::launch_space_mission(int civ_id, SpaceMilestone target_milestone, AeonEngine& engine) {
    for (auto& sp : programs) {
        if (sp.civ_id == civ_id) {
            if (sp.space_budget_millions >= 2000.0f) {
                sp.space_budget_millions -= 2000.0f;
                sp.highest_milestone = target_milestone;
                if (target_milestone == SpaceMilestone::LUNAR_BASE) sp.lunar_colonists += 200;
                if (target_milestone == SpaceMilestone::MARS_COLONY) sp.mars_colonists += 150;
                if (target_milestone == SpaceMilestone::ASTEROID_MINING) sp.titanium_mined_tons += 1000.0;
                engine.history.record(engine.year, engine.month, "SPACE_MISSION",
                    sp.civ_name + " Launched Mission: " + get_milestone_name(target_milestone),
                    "Space launch successfully reached orbit and achieved operational status.", civ_id);
                return true;
            }
            break;
        }
    }
    return false;
}

void AeonSpaceRaceEngine::fund_space_program(int civ_id, float added_budget_millions) {
    for (auto& sp : programs) {
        if (sp.civ_id == civ_id) {
            sp.space_budget_millions += added_budget_millions;
            break;
        }
    }
}

void AeonSpaceRaceEngine::check_kessler_cascade(AeonEngine& engine) {
    // Each active satellite adds minor debris over time
    for (const auto& sp : programs) {
        orbital_debris_count += sp.satellites_in_orbit * 0.5f;
    }

    if (orbital_debris_count > 150.0f && !kessler_syndrome_active) {
        kessler_syndrome_active = true;
        for (auto& sp : programs) {
            sp.satellites_in_orbit = 0; // Collisions wipe out satellite constellations
        }
        for (auto& civ : engine.civs) {
            civ.tech.research_pts *= 0.85f; // Communications disrupted
            civ.stability = std::max(10.0f, civ.stability - 8.0f);
        }
        engine.history.record(engine.year, engine.month, "DISASTER",
            "Kessler Syndrome Triggered!",
            "Orbital space debris chain reaction destroys global satellite networks!", -1);
    }
}

void AeonSpaceRaceEngine::process_asteroid_commodity_shock(AeonEngine& engine) {
    for (const auto& sp : programs) {
        if (sp.highest_milestone == SpaceMilestone::ASTEROID_MINING) {
            // Floods global supply of metals and rare earths
            if (sp.civ_id >= 0 && sp.civ_id < (int)engine.civs.size()) {
                auto& civ = engine.civs[sp.civ_id];
                civ.resources.iron += 200.0f;
                civ.resources.rare += 50.0f;
                civ.economy.gdp += 500.0f;
            }
        }
    }
}

} // namespace Aeon

