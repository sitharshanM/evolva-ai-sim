#ifndef AEON_NAVAL_H
#define AEON_NAVAL_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct NavalFleet {
    int id = 0;
    int civ_id = 0;
    int destroyers = 0;
    int submarines = 0;
    int aircraft_carriers = 0;
    bool is_blockading = false;
    int blockaded_civ_id = -1;
};

class AeonNavalEngine {
public:
    std::vector<NavalFleet> fleets;
    int offshore_oil_rigs = 0;

    void init();
    void tick_year(AeonEngine& engine);
    bool build_warship(AeonEngine& engine, const std::string& ship_type);
    bool enact_sea_blockade(AeonEngine& engine, int target_civ_id);
    bool construct_offshore_rig(AeonEngine& engine);
};

} // namespace Aeon

#endif // AEON_NAVAL_H
