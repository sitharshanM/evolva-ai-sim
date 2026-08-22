#include "aeon_religion.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

void AeonReligionEngine::init() {
    religions.clear();
    holy_relics_collected.clear();
    player_faith_points = 500.0f;
    active_missionaries = 2;

    // Seed major world religions
    WorldReligion r1;
    r1.id = 1;
    r1.name = "Christianity";
    r1.holy_city = "Rome & Jerusalem";
    r1.founder_civ_id = 2; // Valoria Realm
    r1.global_followers_pct = 31.0f;
    r1.faith_power = 140.0f;
    r1.sacred_relic = "Holy Grail of Antiquity";
    religions.push_back(r1);

    WorldReligion r2;
    r2.id = 2;
    r2.name = "Islam";
    r2.holy_city = "Mecca & Medina";
    r2.founder_civ_id = 5; // The Commons Free Territories
    r2.global_followers_pct = 25.0f;
    r2.faith_power = 135.0f;
    r2.sacred_relic = "Sacred Kaaba Relic";
    religions.push_back(r2);

    WorldReligion r3;
    r3.id = 3;
    r3.name = "Dharmic Traditions (Hinduism & Buddhism)";
    r3.holy_city = "Varanasi & Bodh Gaya";
    r3.founder_civ_id = 4; // Solaria Dominion
    r3.global_followers_pct = 20.0f;
    r3.faith_power = 125.0f;
    r3.sacred_relic = "Vedic Scrolls of Dharma";
    religions.push_back(r3);

    WorldReligion r4;
    r4.id = 4;
    r4.name = "Secular Humanism & Rationalism";
    r4.holy_city = "Geneva & Silicon Valley";
    r4.founder_civ_id = 0; // Nordra Realm
    r4.global_followers_pct = 18.0f;
    r4.faith_power = 110.0f;
    r4.sacred_relic = "Charter of Universal Rights";
    religions.push_back(r4);

}


void AeonReligionEngine::tick_year(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (!p.active || p.player_civ_id < 0 || p.player_civ_id >= (int)engine.civs.size()) return;

    auto& civ = engine.civs[p.player_civ_id];

    // Faith points accumulation
    float faith_income = civ.stability * 0.15f + static_cast<float>(holy_relics_collected.size()) * 25.0f + 10.0f;
    player_faith_points += faith_income;

    // Process Holy Wars / Crusades
    for (auto& r : religions) {
        if (r.holy_war_active && r.crusade_target_civ_id >= 0 && r.crusade_target_civ_id < (int)engine.civs.size()) {
            auto& target = engine.civs[r.crusade_target_civ_id];
            target.stability = std::max(0.0f, target.stability - 3.0f);
            target.military_power *= 0.98f;
        }
    }
    
    check_religious_schism(engine);
    resolve_holy_war_tick(engine);
}

bool AeonReligionEngine::found_religion(AeonEngine& engine, const std::string& name, const std::string& relic_name) {
    auto& p = engine.president_game;
    if (player_faith_points < 300.0f) return false;

    player_faith_points -= 300.0f;

    WorldReligion nr;
    nr.id = static_cast<int>(religions.size() + 1);
    nr.name = name.empty() ? "Sacred Path of Light" : name;
    nr.holy_city = engine.civs[p.player_civ_id].name + " Holy Seat";
    nr.founder_civ_id = p.player_civ_id;
    nr.global_followers_pct = 15.0f;
    nr.faith_power = 100.0f;
    nr.sacred_relic = relic_name.empty() ? "Holy Ark of Covenant" : relic_name;

    religions.push_back(nr);
    holy_relics_collected.push_back(nr.sacred_relic);

    p.approval_rating += 12.0f;
    p.last_news_headline = "FAITH TRIUMPH: President Sterling officially founds new World Religion '" + nr.name + "'!";

    engine.history.record(engine.year, engine.month, "RELIGION",
        "Founding of " + nr.name,
        "Sacred Relic: " + nr.sacred_relic, p.player_civ_id);
    return true;
}

bool AeonReligionEngine::dispatch_missionary(AeonEngine& engine, int target_civ_id) {
    if (player_faith_points < 150.0f) return false;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;

    player_faith_points -= 150.0f;
    active_missionaries++;

    auto& target = engine.civs[target_civ_id];
    target.relations[engine.president_game.player_civ_id] = DiplomacyStatus::ALLY;
    target.stability = std::min(100.0f, target.stability + 10.0f);

    engine.president_game.last_news_headline = "FAITH MISSION: Evangelical delegates sent to " + target.name + " to spread national faith.";
    return true;
}

bool AeonReligionEngine::declare_holy_war(AeonEngine& engine, int target_civ_id) {
    if (player_faith_points < 400.0f) return false;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;

    player_faith_points -= 400.0f;
    if (!religions.empty()) {
        religions[0].holy_war_active = true;
        religions[0].crusade_target_civ_id = target_civ_id;
    }

    auto& target = engine.civs[target_civ_id];
    int player_id = engine.president_game.player_civ_id;
    if (player_id >= 0 && player_id < (int)engine.civs.size() && player_id != target_civ_id) {
        auto& player_civ = engine.civs[player_id];

        target.at_war = true;
        target.war_with_civ = player_id;
        target.relations[player_id] = DiplomacyStatus::AT_WAR;

        player_civ.at_war = true;
        player_civ.war_with_civ = target_civ_id;
        player_civ.relations[target_civ_id] = DiplomacyStatus::AT_WAR;
    }

    engine.president_game.last_news_headline = "HOLY CRUSADE: Grand Religious War declared against " + target.name + "!";
    return true;
}

bool AeonReligionEngine::consecrate_shrine(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 12000.0f) return false;

    p.treasury_gold -= 12000.0f;
    player_faith_points += 500.0f;
    p.approval_rating += 5.0f;
    p.last_news_headline = "SACRED SHRINE CONSECRATED: Government builds Grand Cathedral ($12,000 cost).";
    return true;
}

void AeonReligionEngine::check_religious_schism(AeonEngine& engine) {
    for (size_t i = 0; i < religions.size(); ++i) {
        auto& r = religions[i];
        if (r.global_followers_pct > 35.0f) {
            r.schism_risk += 2.0f;
        }

        if (r.schism_risk >= 30.0f && (rand() % 100) < 25) {
            r.schism_risk = 5.0f;
            WorldReligion schism_faith;
            schism_faith.id = (int)religions.size() + 1;
            schism_faith.name = "Reformed " + r.name + " (Heresy)";
            schism_faith.holy_city = "Splinter Diocese";
            schism_faith.founder_civ_id = (r.founder_civ_id + 1) % std::max(1, (int)engine.civs.size());
            schism_faith.global_followers_pct = r.global_followers_pct * 0.4f;
            r.global_followers_pct *= 0.6f;
            schism_faith.faith_power = 90.0f;
            schism_faith.sacred_relic = "Reformed Scripture";
            schism_faith.is_heresy = true;
            schism_faith.parent_religion_id = r.id;
            religions.push_back(schism_faith);

            engine.history.record(engine.year, engine.month, "RELIGION",
                "Great Religious Schism!",
                schism_faith.name + " fractures away from " + r.name + " over doctrinal disputes.", schism_faith.founder_civ_id);
            break;
        }
    }
}

void AeonReligionEngine::resolve_holy_war_tick(AeonEngine& engine) {
    for (auto& r : religions) {
        if (!r.holy_war_active || r.crusade_target_civ_id < 0 || r.crusade_target_civ_id >= (int)engine.civs.size()) continue;

        auto& target = engine.civs[r.crusade_target_civ_id];
        float coalition_army = 0.0f;
        for (int cid : r.coalition_member_civ_ids) {
            if (cid >= 0 && cid < (int)engine.civs.size()) {
                coalition_army += engine.civs[cid].military_power * 0.2f;
            }
        }

        target.military_power = std::max(0.0f, target.military_power - coalition_army * 0.1f);
        target.stability = std::max(0.0f, target.stability - 4.0f);

        if (target.military_power < 100.0f || target.stability < 15.0f) {
            r.holy_war_active = false;
            engine.history.record(engine.year, engine.month, "RELIGION",
                "Crusade Concludes: Target Subdued",
                r.name + " coalition forces forced religious capitulation in " + target.name + ".", r.founder_civ_id);
        }
    }
}

} // namespace Aeon

