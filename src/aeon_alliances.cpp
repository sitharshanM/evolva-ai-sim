#include "aeon_alliances.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

void AeonAllianceEngine::init() {
    blocs.clear();

    AllianceBloc b1;
    b1.id = 1;
    b1.name = "Global Atlantic Defense Pact (NATO)";
    b1.type = "NATO";
    b1.alliance_type = AllianceType::DEFENSIVE_PACT;
    b1.member_civ_ids = { 0, 1, 2 };
    b1.article_5_active = true;
    b1.founded_year = 2026;
    blocs.push_back(b1);

    AllianceBloc b2;
    b2.id = 2;
    b2.name = "Organization of Energy Exporting Nations (OPEC)";
    b2.type = "OPEC";
    b2.alliance_type = AllianceType::ECONOMIC_UNION;
    b2.member_civ_ids = { 0, 3, 4 };
    b2.article_5_active = false;
    b2.founded_year = 2026;
    blocs.push_back(b2);
}

AllyWarResponse AeonAllianceEngine::evaluate_war_response(int ally_id, int attacked_civ_id, int aggressor_civ_id, AeonEngine& engine) {
    if (ally_id < 0 || ally_id >= (int)engine.civs.size()) return AllyWarResponse::REMAIN_NEUTRAL;
    auto& ally = engine.civs[ally_id];
    if (ally.is_alive <= 0.0f || ally.at_war) return AllyWarResponse::REMAIN_NEUTRAL;

    const auto* rel_victim = engine.history.relation_view(ally_id, attacked_civ_id);
    const auto* rel_aggressor = engine.history.relation_view(ally_id, aggressor_civ_id);

    float trust_to_victim = rel_victim ? rel_victim->trust : 0.0f;
    float fear_of_aggressor = rel_aggressor ? rel_aggressor->fear : 0.0f;
    float debt_to_victim = rel_victim ? rel_victim->diplomatic_debt : 0.0f;

    // Check ally ruler personality if available
    const AeonCharacter* ruler = nullptr;
    if (ally.ruler_id >= 0) {
        for (const auto& ch : engine.characters) {
            if (ch.id == ally.ruler_id && ch.is_alive) { ruler = &ch; break; }
        }
    }
    float honor = ruler ? (ruler->loyalty * 0.6f + ruler->morality * 0.4f) : 0.6f;
    float cowardice = ruler ? ruler->paranoia : 0.2f;

    // High trust + high honor + high debt -> join war
    float join_score = (trust_to_victim / 100.0f) * 0.40f + (honor * 0.35f) + (debt_to_victim / 100.0f) * 0.25f - (fear_of_aggressor / 100.0f) * 0.20f;

    if (ally.stability < 30.0f) join_score -= 0.30f;
    if (ally.army_size < 1000.0f) join_score -= 0.30f;

    if (join_score > 0.45f) {
        return AllyWarResponse::JOIN_WAR;
    } else if (join_score > 0.15f) {
        return AllyWarResponse::SEND_MILITARY_AID;
    } else if (join_score > -0.10f) {
        return AllyWarResponse::SEND_MONETARY_AID;
    } else if (join_score < -0.40f && cowardice > 0.60f) {
        return AllyWarResponse::BETRAY_AND_ABANDON;
    }
    return AllyWarResponse::REMAIN_NEUTRAL;
}

void AeonAllianceEngine::tick_year(AeonEngine& engine) {
    // Check mutual defense & alliance obligations
    for (auto& b : blocs) {
        // Apply alliance bonuses
        if (b.alliance_type == AllianceType::ECONOMIC_UNION) {
            for (int cid : b.member_civ_ids) {
                if (cid >= 0 && cid < (int)engine.civs.size()) {
                    engine.civs[cid].economy.gdp *= 1.008f; // +0.8% annual trade bonus
                }
            }
        } else if (b.alliance_type == AllianceType::RESEARCH_PACT) {
            for (int cid : b.member_civ_ids) {
                if (cid >= 0 && cid < (int)engine.civs.size()) {
                    engine.civs[cid].tech.research_pts += 15.0f;
                }
            }
        }

        // Mutual defense evaluation during active wars
        if (b.article_5_active) {
            for (int civ_id : b.member_civ_ids) {
                if (civ_id < 0 || civ_id >= (int)engine.civs.size()) continue;
                auto& civ = engine.civs[civ_id];
                if (civ.at_war && civ.war_with_civ >= 0) {
                    int enemy_id = civ.war_with_civ;

                    for (int ally_id : b.member_civ_ids) {
                        if (ally_id == civ_id || ally_id < 0 || ally_id >= (int)engine.civs.size()) continue;
                        auto& ally = engine.civs[ally_id];
                        if (ally.at_war) continue; // Already engaged

                        AllyWarResponse resp = evaluate_war_response(ally_id, civ_id, enemy_id, engine);

                        if (resp == AllyWarResponse::JOIN_WAR) {
                            ally.at_war = true;
                            ally.war_with_civ = enemy_id;
                            ally.war_year_start = engine.year;
                            ally.relations[enemy_id] = DiplomacyStatus::AT_WAR;
                            engine.history.relation(ally_id, civ_id).record_aid_given();

                            std::cout << "[YEAR " << engine.year << "] 🤝 ALLIANCE HONOR: "
                                      << ally.name << " honors mutual defense treaty and enters the war defending "
                                      << civ.name << " against " << engine.civs[enemy_id].name << "!" << std::endl;
                            engine.history.record(engine.year, engine.month, "DIPLOMACY",
                                ally.name + " joins war honoring alliance with " + civ.name,
                                ally.name + " mobilizes troops in support of " + civ.name + " against " + engine.civs[enemy_id].name,
                                ally_id, enemy_id, {"alliance_obligation"}, 0.75f);
                        } else if (resp == AllyWarResponse::SEND_MILITARY_AID) {
                            float aid_soldiers = ally.army_size * 0.15f;
                            ally.army_size -= aid_soldiers;
                            civ.army_size += aid_soldiers;
                            engine.history.relation(ally_id, civ_id).record_aid_given();
                            std::cout << "[YEAR " << engine.year << "] 🛡️ MILITARY AID: "
                                      << ally.name << " sends " << int(aid_soldiers) << " expeditionary troops to assist "
                                      << civ.name << "." << std::endl;
                        } else if (resp == AllyWarResponse::SEND_MONETARY_AID) {
                            float gold_aid = std::min(ally.economy.gdp * 0.05f, 200.0f);
                            civ.economy.gdp += gold_aid;
                            engine.history.relation(ally_id, civ_id).record_aid_given();
                        } else if (resp == AllyWarResponse::BETRAY_AND_ABANDON) {
                            betray_alliance(ally_id, civ_id, engine);
                        }
                    }
                }
            }
        }
    }
}

bool AeonAllianceEngine::form_alliance(int civ1_id, int civ2_id, AllianceType type, AeonEngine& engine) {
    if (civ1_id < 0 || civ2_id < 0 || civ1_id == civ2_id) return false;
    if (civ1_id >= (int)engine.civs.size() || civ2_id >= (int)engine.civs.size()) return false;

    AllianceBloc b;
    b.id = (int)blocs.size() + 1;
    b.alliance_type = type;
    b.name = engine.civs[civ1_id].name + "-" + engine.civs[civ2_id].name + " " + alliance_type_name(type);
    b.member_civ_ids = { civ1_id, civ2_id };
    b.article_5_active = (type == AllianceType::DEFENSIVE_PACT || type == AllianceType::MILITARY_ALLIANCE || type == AllianceType::FEDERATION);
    b.founded_year = engine.year;
    blocs.push_back(b);

    engine.civs[civ1_id].relations[civ2_id] = DiplomacyStatus::ALLY;
    engine.civs[civ2_id].relations[civ1_id] = DiplomacyStatus::ALLY;

    engine.history.relation(civ1_id, civ2_id).record_treaty();

    std::cout << "[YEAR " << engine.year << "] 📜 DIPLOMATIC TREATY: "
              << engine.civs[civ1_id].name << " and " << engine.civs[civ2_id].name
              << " establish a " << alliance_type_name(type) << "!" << std::endl;

    engine.history.record(engine.year, engine.month, "DIPLOMACY",
        engine.civs[civ1_id].name + " & " + engine.civs[civ2_id].name + " form " + alliance_type_name(type),
        "Bilateral treaty signed creating new geopolitical alignment.",
        civ1_id, civ2_id, {"diplomatic_alignment"}, 0.70f);

    return true;
}

bool AeonAllianceEngine::betray_alliance(int betrayer_civ_id, int victim_civ_id, AeonEngine& engine) {
    if (betrayer_civ_id < 0 || victim_civ_id < 0) return false;
    auto& betrayer = engine.civs[betrayer_civ_id];
    auto& victim = engine.civs[victim_civ_id];

    // Remove from blocs
    for (auto& b : blocs) {
        auto it = std::find(b.member_civ_ids.begin(), b.member_civ_ids.end(), betrayer_civ_id);
        if (it != b.member_civ_ids.end()) {
            b.member_civ_ids.erase(it);
        }
    }

    betrayer.relations[victim_civ_id] = DiplomacyStatus::HOSTILE;
    victim.relations[betrayer_civ_id] = DiplomacyStatus::HOSTILE;

    // Severe long-term consequences
    engine.history.relation(betrayer_civ_id, victim_civ_id).record_betrayal();

    // Global reputation hit: all neighbors lose trust and gain suspicion
    for (size_t i = 0; i < engine.civs.size(); ++i) {
        if ((int)i == betrayer_civ_id) continue;
        engine.history.relation(betrayer_civ_id, (int)i).trust -= 35.0f;
        engine.history.relation(betrayer_civ_id, (int)i).suspicion += 40.0f;
    }

    // Record greatest betrayal if first/biggest
    if (engine.history.stats.greatest_betrayal_year == 0) {
        engine.history.stats.greatest_betrayal_desc = betrayer.name + " abandoned " + victim.name + " in wartime";
        engine.history.stats.greatest_betrayal_year = engine.year;
    }

    std::cout << "\n[YEAR " << engine.year << "] 🗡️ TREASON & BETRAYAL: "
              << betrayer.name << " dishonors alliance and abandons " << victim.name
              << "! Diplomatic reputation destroyed across the continent." << std::endl;

    engine.history.record(engine.year, engine.month, "DIPLOMACY",
        betrayer.name + " BETRAYS alliance with " + victim.name,
        betrayer.name + " refused treaty obligations during crisis, sparking widespread international condemnation.",
        betrayer_civ_id, victim_civ_id, {"opportunistic_betrayal"}, 0.90f);

    return true;
}

bool AeonAllianceEngine::sign_nato_treaty(AeonEngine& engine, int partner_civ_id) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 25000.0f) return false;
    if (partner_civ_id < 0 || partner_civ_id >= (int)engine.civs.size()) return false;

    p.treasury_gold -= 25000.0f;
    if (!blocs.empty()) {
        auto& ids = blocs[0].member_civ_ids;
        if (std::find(ids.begin(), ids.end(), partner_civ_id) == ids.end()) {
            ids.push_back(partner_civ_id);
        }
    }

    auto& partner = engine.civs[partner_civ_id];
    partner.relations[p.player_civ_id] = DiplomacyStatus::ALLY;

    p.approval_rating += 10.0f;
    p.last_news_headline = "NATO DEFENSE TREATY: Signed Article 5 Mutual Defense Pact with " + partner.name + "!";
    return true;
}

bool AeonAllianceEngine::form_opec_cartel(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 30000.0f) return false;

    p.treasury_gold -= 30000.0f;
    p.approval_rating += 7.0f;
    p.last_news_headline = "OPEC CARTEL DIRECTIVE: Agreed on international oil production quotas ($30,000 cost). Boosted energy revenues!";
    return true;
}

} // namespace Aeon

