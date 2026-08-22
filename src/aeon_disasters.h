#ifndef AEON_DISASTERS_H
#define AEON_DISASTERS_H

#include <string>
#include <vector>
#include <memory>

namespace Aeon {

class AeonEngine;

enum class DisasterType {
    VOLCANIC_ERUPTION,
    METEOR_STRIKE,
    BIO_PLAGUE,
    SOLAR_FLARE,
    MEGA_EARTHQUAKE,
    LOCUST_SWARM
};

struct ActiveDisaster {
    int id = 0;
    DisasterType type = DisasterType::VOLCANIC_ERUPTION;
    std::string name;
    std::string location_name;
    int target_x = 0;
    int target_y = 0;
    int duration_years = 3;
    float severity = 1.0f; // 0.1 to 1.0
    int start_year = 0;
    bool is_active = true;
    bool triggered_cascade = false;
};

class AeonDisasterEngine {
public:
    std::vector<ActiveDisaster> active_disasters;
    std::vector<std::string> disaster_history_log;
    float international_relief_fund_gold = 0.0f;

    void init();
    void tick_year(AeonEngine& engine);
    void trigger_disaster(AeonEngine& engine, DisasterType type, int target_x = -1, int target_y = -1);
    void check_disaster_cascades(AeonEngine& engine);
    void contribute_to_relief_fund(int donor_civ_id, float amount, AeonEngine& engine);

    // Crisis intervention decrees
    bool enact_quarantine(AeonEngine& engine);
    bool research_plague_vaccine(AeonEngine& engine);
    bool deploy_disaster_relief(AeonEngine& engine);
};

} // namespace Aeon

#endif // AEON_DISASTERS_H

