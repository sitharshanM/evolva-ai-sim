#ifndef AEON_REBELLION_H
#define AEON_REBELLION_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct RebelFaction {
    int id = 0;
    int parent_civ_id = -1;
    int rebel_civ_id = -1;
    std::string name;             // e.g. "Free Nordra Republic"
    std::string ideology;         // e.g. "Libertarian Separatists"
    float strength = 50.0f;
    bool active = false;
};

class AeonRebellionEngine {
public:
    std::vector<RebelFaction> rebellions;

    AeonRebellionEngine();
    void update_rebellions_tick(AeonEngine& engine);
    void trigger_secession(int civ_id, AeonEngine& engine);
};

} // namespace Aeon

#endif // AEON_REBELLION_H
