#include "aeon_parliament.h"
#include "aeon_engine.h"
#include <cstdlib>
#include <algorithm>

namespace Aeon {

void AeonParliamentEngine::init() {
    seats_liberal = 30;
    seats_conservative = 32;
    seats_security = 18;
    seats_technocrat = 12;
    seats_green = 8;
    bill_history.clear();
}

void AeonParliamentEngine::tick_year(AeonEngine& engine) {
    // Shifts in parliamentary seat distribution based on presidential approval & stability
    auto& p = engine.president_game;
    if (!p.active || p.player_civ_id < 0 || p.player_civ_id >= (int)engine.civs.size()) return;

    if (p.years_in_office % 4 == 0) { // General Election Year
        if (p.approval_rating > 60.0f) {
            seats_liberal = std::min(45, seats_liberal + 3);
            seats_conservative = std::max(20, seats_conservative - 2);
        } else {
            seats_conservative = std::min(45, seats_conservative + 3);
            seats_liberal = std::max(15, seats_liberal - 3);
        }
    }
}

bool AeonParliamentEngine::submit_bill_to_vote(AeonEngine& engine, const std::string& title, const std::string& summary, float cost) {
    auto& p = engine.president_game;
    if (p.treasury_gold < cost) return false;

    LegislativeBill bill;
    bill.id = static_cast<int>(bill_history.size() + 1);
    bill.title = title;
    bill.summary = summary;
    bill.cost_gold = cost;

    // Calculate parliamentary votes based on party seat stances
    int aye = seats_liberal + (rand() % 10);
    if (cost > 30000.0f) {
        aye += seats_technocrat;
    } else {
        aye += seats_conservative / 2;
    }
    int nay = 100 - aye;

    bill.votes_for = aye;
    bill.votes_against = nay;
    bill.passed = (aye >= 51);

    if (bill.passed) {
        p.treasury_gold -= cost;
        p.approval_rating += 5.0f;
        p.last_news_headline = "CONGRESS PASSES BILL: '" + title + "' (" + std::to_string(aye) + " Ayes vs " + std::to_string(nay) + " Nays)!";
    } else {
        p.last_news_headline = "PARLIAMENT REJECTS BILL: '" + title + "' defeated (" + std::to_string(nay) + " Nays vs " + std::to_string(aye) + " Ayes).";
    }

    bill_history.push_back(bill);
    engine.history.record(engine.year, engine.month, "POLITICS", "Parliamentary Vote on " + title,
        bill.passed ? "PASSED by Majority" : "REJECTED by Congress", p.player_civ_id);
    return bill.passed;
}

bool AeonParliamentEngine::execute_campaign_rally(AeonEngine& engine, PoliticalParty party) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 8000.0f) return false;

    p.treasury_gold -= 8000.0f;
    p.approval_rating = std::min(100.0f, p.approval_rating + 6.0f);
    p.last_news_headline = "CAMPAIGN RALLY: President Sterling addresses nationwide rally for " + std::string(party_name(party)) + "!";
    return true;
}

bool AeonParliamentEngine::pass_executive_order(AeonEngine& engine, const std::string& title, float cost) {
    auto& p = engine.president_game;
    if (p.treasury_gold < cost) return false;

    p.treasury_gold -= cost;
    p.approval_rating = std::max(0.0f, p.approval_rating - 4.0f); // Executive bypass angers congress
    p.last_news_headline = "EXECUTIVE DECREE: President bypasses Congress to sign " + title + "!";
    engine.history.record(engine.year, engine.month, "POLITICS", "Executive Order Enacted", title, p.player_civ_id);
    return true;
}

void AeonParliamentEngine::dissolve_parliament(AeonEngine& engine) {
    auto& p = engine.president_game;
    seats_liberal = 20;
    seats_conservative = 20;
    seats_security = 40;
    seats_technocrat = 10;
    seats_green = 10;
    p.approval_rating = std::max(0.0f, p.approval_rating - 15.0f);
    p.last_news_headline = "PARLIAMENT DISSOLVED: Emergency Decree issued to call snap elections!";
    engine.history.record(engine.year, engine.month, "POLITICS", "Parliament Dissolved", "Snap elections ordered under martial law.", p.player_civ_id);
}

// ─── shift_seats_by_events ────────────────────────────────────────────────────
void AeonParliamentEngine::shift_seats_by_events(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (!p.active) return;
    if (p.player_civ_id < 0 || p.player_civ_id >= (int)engine.civs.size()) return;
    auto& civ = engine.civs[p.player_civ_id];

    // Recession → left shift (people want welfare)
    if (civ.economy.business_cycle == BusinessCycle::RECESSION ||
        civ.economy.business_cycle == BusinessCycle::DEPRESSION) {
        seats_liberal = std::min(total_seats - 10, seats_liberal + 3);
        seats_conservative = std::max(5, seats_conservative - 2);
        political_momentum -= 5.0f;
    }
    // High war exhaustion → security party loses support
    if (civ.war_exhaustion > 60.0f) {
        seats_security = std::max(2, seats_security - 3);
        seats_liberal   = std::min(total_seats - 10, seats_liberal + 2);
    }
    // Normalize seats
    int total = seats_liberal + seats_conservative + seats_security + seats_technocrat + seats_green;
    if (total != total_seats) {
        int diff = total_seats - total;
        seats_technocrat += diff; // Absorb rounding into technocrats
        seats_technocrat = std::max(1, seats_technocrat);
    }
    term_years_served++;
    political_momentum = std::max(-100.0f, std::min(100.0f, political_momentum));
    last_election_year = engine.year;
}

// ─── process_lobbying ─────────────────────────────────────────────────────────
void AeonParliamentEngine::process_lobbying(AeonEngine& engine) {
    (void)engine;
    for (auto& rec : lobby_records) {
        // Lobbying buys seat influence that decays
        rec.seat_influence *= 0.85f; // Decay each year
        int seats_added = (int)(rec.seat_influence / 10.0f);
        // Apply seat shift for lobby's aligned party
        switch (rec.party) {
            case PoliticalParty::LIBERAL_DEMOCRATS:
                seats_liberal = std::min(50, seats_liberal + seats_added); break;
            case PoliticalParty::CONSERVATIVE_UNION:
                seats_conservative = std::min(50, seats_conservative + seats_added); break;
            case PoliticalParty::NATIONAL_SECURITY:
                seats_security = std::min(30, seats_security + seats_added); break;
            case PoliticalParty::TECHNOCRAT_PROGRESS:
                seats_technocrat = std::min(30, seats_technocrat + seats_added); break;
            case PoliticalParty::GREEN_ECOLOGY:
                seats_green = std::min(30, seats_green + seats_added); break;
        }
    }
    // Remove expired lobbying records
    lobby_records.erase(std::remove_if(lobby_records.begin(), lobby_records.end(),
        [](const LobbyingRecord& r){ return r.seat_influence < 0.5f; }), lobby_records.end());
}

// ─── check_protest_escalation ────────────────────────────────────────────────
void AeonParliamentEngine::check_protest_escalation(AeonEngine& engine) {
    for (auto& wave : protest_waves) {
        auto& civ = engine.civs[wave.civ_id];
        if (civ.is_alive <= 0.0f) continue;
        // Protest grows when happiness is low and not addressed
        if (civ.population.happiness < 40.0f) wave.size += 5.0f;
        if (civ.stability < 30.0f)            wave.size += 10.0f;
        wave.size = std::min(100.0f, wave.size);

        // Escalates to violence when size > 70 and stability < 40
        if (wave.size > 70.0f && civ.stability < 40.0f && !wave.violent) {
            wave.violent = true;
            civ.stability -= 15.0f;
            engine.history.record(engine.year, engine.month, "POLITICS",
                civ.name + " protests turn violent",
                wave.demand + " — rioting breaks out in cities.", wave.civ_id);
        }
        // Full civil war if violent + size maxed
        if (wave.violent && wave.size >= 100.0f) {
            civ.stability -= 25.0f;
            civ.at_war = true;
            engine.history.record(engine.year, engine.month, "POLITICS",
                civ.name + " civil war erupts",
                "Regime unable to suppress mass uprising.", wave.civ_id);
        }
    }
    // Remove resolved protests
    protest_waves.erase(std::remove_if(protest_waves.begin(), protest_waves.end(),
        [](const ProtestWave& w){ return w.size <= 0.0f; }), protest_waves.end());
}

// ─── enforce_term_limits ──────────────────────────────────────────────────────
void AeonParliamentEngine::enforce_term_limits(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (!p.active) return;
    // After 12 years ruling party loses 5 seats per year (incumbency fatigue)
    if (term_years_served >= 12) {
        int fatigue_loss = (term_years_served - 11) * 2;
        seats_conservative = std::max(5, seats_conservative - fatigue_loss);
        seats_liberal      = std::min(45, seats_liberal + fatigue_loss / 2);
        if (p.player_civ_id >= 0 && p.player_civ_id < (int)engine.civs.size()) {
            engine.civs[p.player_civ_id].population.happiness -= 1.0f;
        }
    }
}

} // namespace Aeon
