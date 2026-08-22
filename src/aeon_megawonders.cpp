#include "aeon_megawonders.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

AeonMegaWonderEngine::AeonMegaWonderEngine() {
}

void AeonMegaWonderEngine::init_megawonders(const AeonEngine& engine) {
    (void)engine;
    mega_wonders.clear();
    // Default starting mega wonder for Eldoria
    MegaWonder mw;
    mw.id = 1;
    mw.name = "Dyson Swarm Solar Collector Array";
    mw.type = "Space Megastructure";
    mw.owner_civ_id = 2; // Eldoria
    mw.tile_x = 300;
    mw.tile_y = 60;
    mw.construction_progress = 75.0f;
    mw.completed = false;
    mw.gdp_multiplier = 1.40f;
    mega_wonders.push_back(mw);
}

void AeonMegaWonderEngine::start_construction(int civ_id, const std::string& name, AeonEngine& engine) {
    if (civ_id < 0 || civ_id >= (int)engine.civs.size()) return;
    auto& civ = engine.civs[civ_id];

    MegaWonder mw;
    mw.id = (int)mega_wonders.size() + 1;
    mw.name = name;
    mw.type = "Mega-Engineering Wonder";
    mw.owner_civ_id = civ_id;
    mw.tile_x = civ.capital_x;
    mw.tile_y = civ.capital_y;
    mw.construction_progress = 10.0f;
    mw.completed = false;
    mw.gdp_multiplier = 1.30f;
    mega_wonders.push_back(mw);

    engine.history.record(engine.year, engine.month, "MEGAWONDER",
        civ.name + " Begins Mega-Project Construction!",
        "Construction initiated on " + name + ".", civ_id);

    std::cout << "[YEAR " << engine.year << "] 🏗️ MEGAWONDER: " << civ.name
              << " begins building " << name << "!" << std::endl;
}

void AeonMegaWonderEngine::update_megawonders_tick(AeonEngine& engine) {
    for (auto& mw : mega_wonders) {
        if (!mw.completed) {
            mw.construction_progress += 5.0f;
            if (mw.construction_progress >= 100.0f) {
                mw.construction_progress = 100.0f;
                mw.completed = true;
                if (mw.owner_civ_id >= 0 && mw.owner_civ_id < (int)engine.civs.size()) {
                    auto& owner = engine.civs[mw.owner_civ_id];
                    owner.economy.gdp *= mw.gdp_multiplier;
                    engine.history.record(engine.year, engine.month, "MEGAWONDER",
                        owner.name + " Completes " + mw.name + "!",
                        "Mega-structure online! Imperial GDP boosted by +30%.", mw.owner_civ_id);
                }
            }
        }
    }
}

} // namespace Aeon
