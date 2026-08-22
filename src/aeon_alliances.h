#ifndef AEON_ALLIANCES_H
#define AEON_ALLIANCES_H

#include "aeon_world_types.h"
#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct AllianceBloc {
    int id = 0;
    std::string name;
    AllianceType alliance_type = AllianceType::DEFENSIVE_PACT;
    std::string type; // NATO, OPEC, EU, RESEARCH_LEAGUE, FEDERATION
    std::vector<int> member_civ_ids;
    bool article_5_active = true;
    int  founded_year = 2026;
    float cumulative_trade_bonus = 0.0f;
    float cumulative_science_bonus = 0.0f;
};

enum class AllyWarResponse {
    JOIN_WAR,
    SEND_MILITARY_AID,
    SEND_MONETARY_AID,
    REMAIN_NEUTRAL,
    BETRAY_AND_ABANDON
};

class AeonAllianceEngine {
public:
    std::vector<AllianceBloc> blocs;

    void init();
    void tick_year(AeonEngine& engine);
    
    AllyWarResponse evaluate_war_response(int ally_id, int attacked_civ_id, int aggressor_civ_id, AeonEngine& engine);
    bool form_alliance(int civ1_id, int civ2_id, AllianceType type, AeonEngine& engine);
    bool betray_alliance(int betrayer_civ_id, int victim_civ_id, AeonEngine& engine);

    bool sign_nato_treaty(AeonEngine& engine, int partner_civ_id);
    bool form_opec_cartel(AeonEngine& engine);
};

} // namespace Aeon

#endif // AEON_ALLIANCES_H

