#include "aeon_wonders.h"
#include "aeon_engine.h"
#include <iostream>
#include <algorithm>

namespace Aeon {

void AeonWonderEngine::init() {
    wonders.clear();

    WorldWonder w1;
    w1.id = 1;
    w1.name = "Great Pyramids of Sol";
    w1.description = "Monumental stone pyramids honoring the sun god.";
    w1.cost_gold = 150000.0f;
    w1.invested_gold = 0.0f;
    w1.is_built = false;
    w1.passive_perk_text = "+25% National Stability & +20% Population Growth";
    wonders.push_back(w1);

    WorldWonder w2;
    w2.id = 2;
    w2.name = "Great Library of Aeon";
    w2.description = "Universal archive of ancient and futuristic knowledge.";
    w2.cost_gold = 200000.0f;
    w2.invested_gold = 0.0f;
    w2.is_built = false;
    w2.passive_perk_text = "+35% Global Technology Research Rate";
    wonders.push_back(w2);

    WorldWonder w3;
    w3.id = 3;
    w3.name = "Hadron Particle Supercollider";
    w3.description = "Subatomic particle accelerator discovering zero-point energy.";
    w3.cost_gold = 300000.0f;
    w3.invested_gold = 0.0f;
    w3.is_built = false;
    w3.passive_perk_text = "Unlocks Fusion Tech & Antimatter Weaponry";
    wonders.push_back(w3);

    WorldWonder w4;
    w4.id = 4;
    w4.name = "Global Orbital Defense Shield";
    w4.description = "Space laser constellation capable of stopping 100% of ICBM strikes.";
    w4.cost_gold = 400000.0f;
    w4.invested_gold = 0.0f;
    w4.is_built = false;
    w4.passive_perk_text = "Immunity to incoming Nuclear ICBM Strikes";
    wonders.push_back(w4);
}

void AeonWonderEngine::tick_year(AeonEngine& engine) {
    // Apply passive perks of constructed wonders
    for (const auto& w : wonders) {
        if (w.is_built && w.builder_civ_id >= 0 && w.builder_civ_id < (int)engine.civs.size()) {
            auto& civ = engine.civs[w.builder_civ_id];
            if (w.id == 1) { // Pyramids
                civ.stability = std::min(100.0f, civ.stability + 0.5f);
                civ.population.total = static_cast<long long>(civ.population.total * 1.005f);
            } else if (w.id == 2) { // Library
                civ.tech.progress += 25.0f;
            } else if (w.id == 3) { // Supercollider
                civ.economy.gdp += 1000.0f;
            } else if (w.id == 4) { // Orbital Shield
                engine.space_espionage.space.orbital_defense_active = true;
            }
        }
    }
}

bool AeonWonderEngine::invest_in_wonder(AeonEngine& engine, int wonder_id, float amount) {
    auto& p = engine.president_game;
    if (p.treasury_gold < amount) return false;

    for (auto& w : wonders) {
        if (w.id == wonder_id && !w.is_built) {
            p.treasury_gold -= amount;
            w.invested_gold += amount;

            if (w.invested_gold >= w.cost_gold) {
                w.is_built = true;
                w.built_year = engine.year;
                w.builder_civ_id = p.player_civ_id;

                p.approval_rating += 30.0f;
                p.last_news_headline = "WORLD WONDER COMPLETED: President Sterling unveils the " + w.name + "!";

                engine.history.record(engine.year, engine.month, "WONDER",
                    "Completion of " + w.name,
                    w.passive_perk_text, p.player_civ_id);
            } else {
                p.last_news_headline = "WONDER FUNDING: $" + std::to_string((int)amount) + " invested in " + w.name + ". Progress: " + std::to_string((int)(w.invested_gold / w.cost_gold * 100.0f)) + "%";
            }
            return true;
        }
    }
    return false;
}

} // namespace Aeon
