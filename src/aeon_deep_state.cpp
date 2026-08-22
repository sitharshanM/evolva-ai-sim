#include "aeon_deep_state.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

const char* shadow_society_name(ShadowSociety soc) {
    switch (soc) {
        case ShadowSociety::OBSIDIAN_CITADEL: return "🗡️ The Obsidian Citadel (Shadow Military-Industrial)";
        case ShadowSociety::TECHNOCRATIC_APEX: return "👁️ The Technocratic Apex (AI & Corporate Surveillance)";
        case ShadowSociety::SOLAR_SYNDICATE:   return "💼 The Solar Syndicate (Shadow Banking & Cartel)";
    }
    return "Secret Society";
}

void AeonDeepStateEngine::init(const AeonEngine& engine) {
    cells.clear();
    for (const auto& civ : engine.civs) {
        if (civ.is_commons) continue;
        DeepStateCell cell;
        cell.society = static_cast<ShadowSociety>(civ.id % 3);
        cell.civ_id = civ.id;
        cell.civ_name = civ.name;
        cell.infiltration_pct = 20.0f + (civ.id * 5.0f);
        cell.shadow_funds_gold = 1000.0f;
        cell.coup_plot_active = false;
        cells.push_back(cell);
    }
}

void AeonDeepStateEngine::tick_year(AeonEngine& engine) {
    for (auto& cell : cells) {
        if (cell.civ_id < 0 || cell.civ_id >= (int)engine.civs.size()) continue;
        auto& civ = engine.civs[cell.civ_id];
        if (civ.is_alive <= 0.0f) continue;

        // Infiltration creeps up when stability drops
        if (civ.stability < 50.0f) {
            cell.infiltration_pct = std::min(100.0f, cell.infiltration_pct + 2.0f);
        }

        // Siphon treasury to shadow funds
        float siphon = civ.economy.gdp * 0.01f;
        cell.shadow_funds_gold += siphon;

        // Automatic coup trigger if infiltration > 80%
        if (cell.infiltration_pct >= 80.0f && !cell.coup_plot_active) {
            cell.coup_plot_active = true;
            launch_shadow_coup(cell.civ_id, cell.society, engine);
        }
    }
}

bool AeonDeepStateEngine::launch_shadow_coup(int civ_id, ShadowSociety soc, AeonEngine& engine) {
    for (auto& cell : cells) {
        if (cell.civ_id == civ_id) {
            auto& civ = engine.civs[civ_id];
            civ.stability = std::max(5.0f, civ.stability - 30.0f);
            engine.history.record(engine.year, engine.month, "DEEP_STATE",
                std::string(shadow_society_name(soc)) + " Executes Covert Coup in " + civ.name,
                "Shadow council alters state policy and installs puppet cabinet ministers.", civ_id);

            cell.infiltration_pct = 40.0f; // Reset after coup
            cell.coup_plot_active = false;
            return true;
        }
    }
    return false;
}

void AeonDeepStateEngine::siphoning_treasury(int civ_id, float amount, AeonEngine& engine) {
    for (auto& cell : cells) {
        if (cell.civ_id == civ_id) {
            cell.shadow_funds_gold += amount;
            engine.civs[civ_id].economy.gdp = std::max(10.0f, engine.civs[civ_id].economy.gdp - amount);
        }
    }
}

} // namespace Aeon
