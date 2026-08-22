#include "aeon_rebellion.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

AeonRebellionEngine::AeonRebellionEngine() {
}

void AeonRebellionEngine::trigger_secession(int civ_id, AeonEngine& engine) {
    if (civ_id < 0 || civ_id >= (int)engine.civs.size()) return;
    auto& parent = engine.civs[civ_id];

    RebelFaction rf;
    rf.id = (int)rebellions.size() + 1;
    rf.parent_civ_id = civ_id;
    rf.name = "Free " + parent.name + " Front";
    rf.ideology = "Separatist Movement";
    rf.strength = 75.0f;
    rf.active = true;

    // Find target insurgent / commons civ
    int target_war_id = -1;
    for (size_t i = 0; i < engine.civs.size(); ++i) {
        if (engine.civs[i].is_commons) {
            target_war_id = static_cast<int>(i);
            break;
        }
    }
    if (target_war_id < 0 && !engine.civs.empty()) {
        target_war_id = (civ_id + 1) % static_cast<int>(engine.civs.size());
    }

    // Set parent civ into war and lower stability
    parent.stability = std::max(10.0f, parent.stability - 35.0f);
    if (target_war_id >= 0 && target_war_id < static_cast<int>(engine.civs.size()) && target_war_id != civ_id) {
        parent.at_war = true;
        parent.war_with_civ = target_war_id;
        parent.relations[target_war_id] = DiplomacyStatus::AT_WAR;

        auto& target_civ = engine.civs[target_war_id];
        target_civ.at_war = true;
        target_civ.war_with_civ = civ_id;
        target_civ.relations[civ_id] = DiplomacyStatus::AT_WAR;
    }

    rebellions.push_back(rf);

    engine.history.record(engine.year, engine.month, "CIVIL_WAR",
        parent.name + " Fractures in Civil War!",
        rf.name + " secedes following widespread internal unrest.", civ_id);

    std::cout << "[YEAR " << engine.year << "] ⚔️ CIVIL WAR: " << rf.name
              << " secedes from " << parent.name << "!" << std::endl;
}

void AeonRebellionEngine::update_rebellions_tick(AeonEngine& engine) {
    for (auto& civ : engine.civs) {
        if (civ.is_commons || civ.is_alive <= 0.0f) continue;

        // Check if low stability triggers secession
        if (civ.stability < 30.0f && (rand() % 100) < 25) {
            bool already_rebelling = false;
            for (const auto& r : rebellions) {
                if (r.parent_civ_id == civ.id && r.active) {
                    already_rebelling = true;
                    break;
                }
            }
            if (!already_rebelling) {
                trigger_secession(civ.id, engine);
            }
        }
    }
}

} // namespace Aeon
