#include "aeon_ruler_psyche.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

void AeonRulerPsycheEngine::init(const AeonEngine& engine) {
    profiles.clear();
    for (const auto& civ : engine.civs) {
        if (civ.is_commons) continue;
        RulerPsychProfile prof;
        prof.civ_id = civ.id;
        prof.ruler_name = "High Sovereign of " + civ.name;
        prof.paranoia = 10.0f + (civ.id * 8.0f);
        prof.megalomania = 15.0f + (civ.id * 10.0f);
        prof.ptsd_trauma = 0.0f;
        prof.pragmatism = 70.0f - (civ.id * 5.0f);
        prof.narcissism = 20.0f + (civ.id * 5.0f);
        prof.mental_state_status = "Rational & Stable";
        profiles.push_back(prof);
    }
}

void AeonRulerPsycheEngine::tick_year(AeonEngine& engine) {
    for (auto& prof : profiles) {
        if (prof.civ_id < 0 || prof.civ_id >= (int)engine.civs.size()) continue;
        auto& civ = engine.civs[prof.civ_id];
        if (civ.is_alive <= 0.0f) continue;

        // Increase paranoia if at war
        if (civ.at_war) {
            prof.paranoia = std::min(100.0f, prof.paranoia + 4.5f);
            prof.ptsd_trauma = std::min(100.0f, prof.ptsd_trauma + 3.0f);
        } else {
            prof.ptsd_trauma = std::max(0.0f, prof.ptsd_trauma - 2.0f);
        }

        // Increase megalomania if military power > 800
        if (civ.military_power > 800.0f) {
            prof.megalomania = std::min(100.0f, prof.megalomania + 3.5f);
        }

        // Check mental breakdown thresholds
        if (prof.paranoia > 85.0f && !prof.breakdown_active) {
            prof.breakdown_active = true;
            prof.mental_state_status = "⚠️ Severe Paranoia & Delusions";
            trigger_cabinet_purge(prof.civ_id, engine);
        } else if (prof.megalomania > 85.0f && !prof.breakdown_active) {
            prof.breakdown_active = true;
            prof.mental_state_status = "👑 Megalomanic Conqueror";
            civ.aggression = std::min(1.0f, civ.aggression + 0.25f);
            engine.history.record(engine.year, engine.month, "PSYCHE",
                civ.name + " Ruler Enters Megalomanic State",
                "Ruler demands grand monuments and pre-emptive conquest of rival nations.", prof.civ_id);
        } else if (prof.ptsd_trauma > 75.0f) {
            prof.mental_state_status = "💔 Shell-Shocked War Trauma";
            civ.stability = std::max(10.0f, civ.stability - 5.0f);
        } else {
            prof.breakdown_active = false;
            prof.mental_state_status = "Rational & Stable";
        }
    }
}

void AeonRulerPsycheEngine::trigger_war_trauma(int civ_id, float severity, AeonEngine& engine) {
    for (auto& prof : profiles) {
        if (prof.civ_id == civ_id) {
            prof.ptsd_trauma = std::min(100.0f, prof.ptsd_trauma + severity);
            prof.paranoia = std::min(100.0f, prof.paranoia + severity * 0.8f);
            engine.history.record(engine.year, engine.month, "PSYCHE",
                prof.ruler_name + " Suffers War Trauma Shock",
                "Catastrophic battle losses deeply alter the ruler's psychological state.", civ_id);
        }
    }
}

void AeonRulerPsycheEngine::trigger_cabinet_purge(int civ_id, AeonEngine& engine) {
    for (auto& prof : profiles) {
        if (prof.civ_id == civ_id) {
            engine.history.record(engine.year, engine.month, "PSYCHE",
                engine.civs[civ_id].name + " Ruler Executes Cabinet Purge",
                "Paranoid ruler orders arrest of senior generals and ministers due to suspected treason.", civ_id);
            engine.civs[civ_id].stability = std::max(10.0f, engine.civs[civ_id].stability - 15.0f);
            prof.paranoia = 40.0f; // Reset paranoia after purge
        }
    }
}

} // namespace Aeon
