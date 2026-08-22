#ifndef AEON_UN_COUNCIL_H
#define AEON_UN_COUNCIL_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct UNResolution {
    int id = 0;
    std::string title;
    std::string description;
    bool passed = false;
    int votes_in_favor = 0;
    int votes_against = 0;
    bool vetoed_by_p5 = false;
};

class AeonUNCouncilEngine {
public:
    int peacekeeper_divisions = 4;
    std::vector<UNResolution> active_resolutions;

    void init();
    void tick_year(AeonEngine& engine);
    bool propose_resolution(AeonEngine& engine, const std::string& title, const std::string& desc);
    bool deploy_peacekeepers(AeonEngine& engine, int target_civ_id);
    bool enact_un_sanctions(AeonEngine& engine, int target_civ_id);
};

} // namespace Aeon

#endif // AEON_UN_COUNCIL_H
