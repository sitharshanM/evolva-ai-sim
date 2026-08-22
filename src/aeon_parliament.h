#ifndef AEON_PARLIAMENT_H
#define AEON_PARLIAMENT_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

enum class PoliticalParty {
    LIBERAL_DEMOCRATS,
    CONSERVATIVE_UNION,
    NATIONAL_SECURITY,
    TECHNOCRAT_PROGRESS,
    GREEN_ECOLOGY
};

inline const char* party_name(PoliticalParty p) {
    switch (p) {
        case PoliticalParty::LIBERAL_DEMOCRATS:   return "Liberal Democrats 🔷";
        case PoliticalParty::CONSERVATIVE_UNION:  return "Conservative Union 🟥";
        case PoliticalParty::NATIONAL_SECURITY:   return "National Security Party 🦅";
        case PoliticalParty::TECHNOCRAT_PROGRESS: return "Technocrat Progress ⚛️";
        case PoliticalParty::GREEN_ECOLOGY:       return "Green Ecology 🌿";
    }
    return "Independent";
}

struct LegislativeBill {
    int id = 0;
    std::string title;
    std::string summary;
    float cost_gold = 0.0f;
    int votes_for = 0;
    int votes_against = 0;
    bool passed = false;
    bool vetoed_by_president = false;
};

struct LobbyingRecord {
    PoliticalParty party;
    std::string    interest_group;  // e.g. "Steel Magnates", "Tech Union"
    float          gold_donated     = 0.0f;
    float          seat_influence   = 0.0f;  // bonus seats this buys
    int            year_donated     = 0;
};

struct ProtestWave {
    int   civ_id        = -1;
    int   year_started  = 0;
    float size          = 0.0f;    // 0-100; grows if grievances unaddressed
    bool  violent       = false;   // escalates to riot / civil war
    std::string demand; // e.g. "Lower taxes", "Elections"
};

class AeonParliamentEngine {
public:
    int total_seats = 100;
    int seats_liberal = 30;
    int seats_conservative = 32;
    int seats_security = 18;
    int seats_technocrat = 12;
    int seats_green = 8;
    // Accuracy additions
    float political_momentum    = 0.0f;  // -100 (left) to +100 (right)
    int   election_cooldown_yrs = 4;     // years between elections
    int   last_election_year    = -999;
    int   term_years_served     = 0;     // ruling party years in power
    std::vector<LegislativeBill> bill_history;
    std::vector<LobbyingRecord>  lobby_records;
    std::vector<ProtestWave>     protest_waves;

    void init();
    void tick_year(AeonEngine& engine);
    bool submit_bill_to_vote(AeonEngine& engine, const std::string& title, const std::string& summary, float cost);
    bool execute_campaign_rally(AeonEngine& engine, PoliticalParty party);
    bool pass_executive_order(AeonEngine& engine, const std::string& title, float cost);
    void dissolve_parliament(AeonEngine& engine);
    // New accuracy methods
    void shift_seats_by_events(AeonEngine& engine);
    void process_lobbying(AeonEngine& engine);
    void check_protest_escalation(AeonEngine& engine);
    void enforce_term_limits(AeonEngine& engine);
};

} // namespace Aeon

#endif // AEON_PARLIAMENT_H
