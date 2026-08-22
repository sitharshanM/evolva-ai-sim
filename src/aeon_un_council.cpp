#include "aeon_un_council.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

void AeonUNCouncilEngine::init() {
    peacekeeper_divisions = 4;
    active_resolutions.clear();

    UNResolution r1{ 1, "Global Climate Accord", "Enforce 20% carbon reduction across all industrial member states.", true, 4, 1, false };
    UNResolution r2{ 2, "Nuclear Non-Proliferation Treaty", "Prohibit non-nuclear nations from building ICBM silos.", true, 5, 0, false };
    active_resolutions.push_back(r1);
    active_resolutions.push_back(r2);
}

void AeonUNCouncilEngine::tick_year(AeonEngine& engine) {
    // UN Peacekeeping resolution effects
    for (auto& civ : engine.civs) {
        if (civ.at_war && peacekeeper_divisions > 0) {
            civ.stability = std::min(100.0f, civ.stability + 1.0f);
        }
    }
}

bool AeonUNCouncilEngine::propose_resolution(AeonEngine& engine, const std::string& title, const std::string& desc) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 15000.0f) return false;

    p.treasury_gold -= 15000.0f;

    UNResolution res;
    res.id = static_cast<int>(active_resolutions.size() + 1);
    res.title = title;
    res.description = desc;
    res.votes_in_favor = 4;
    res.votes_against = 1;
    res.passed = true;

    active_resolutions.push_back(res);

    p.approval_rating += 8.0f;
    p.last_news_headline = "UN GENERAL ASSEMBLY: Passed Resolution '" + title + "' (" + std::to_string(res.votes_in_favor) + " votes in favor)!";

    engine.history.record(engine.year, engine.month, "DIPLOMACY", "UN Resolution Passed", title, p.player_civ_id);
    return true;
}

bool AeonUNCouncilEngine::deploy_peacekeepers(AeonEngine& engine, int target_civ_id) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 20000.0f) return false;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;

    p.treasury_gold -= 20000.0f;
    peacekeeper_divisions++;

    auto& target = engine.civs[target_civ_id];
    if (target.at_war && target.war_with_civ >= 0 && target.war_with_civ < (int)engine.civs.size()) {
        auto& enemy = engine.civs[target.war_with_civ];
        enemy.at_war = false;
        enemy.war_with_civ = -1;
    }
    target.at_war = false;
    target.war_with_civ = -1;
    target.stability = std::min(100.0f, target.stability + 20.0f);

    p.last_news_headline = "UN PEACEKEEPERS: Deployed Blue Helmet division to ceasefire zone in " + target.name + "!";
    return true;
}

bool AeonUNCouncilEngine::enact_un_sanctions(AeonEngine& engine, int target_civ_id) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 12000.0f) return false;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;

    p.treasury_gold -= 12000.0f;
    auto& target = engine.civs[target_civ_id];
    target.economy.gdp *= 0.85f; // 15% GDP hit from international sanctions
    target.stability = std::max(0.0f, target.stability - 10.0f);

    p.last_news_headline = "UN SECURITY COUNCIL: Imposed multilateral economic sanctions against " + target.name + "!";
    return true;
}

} // namespace Aeon
