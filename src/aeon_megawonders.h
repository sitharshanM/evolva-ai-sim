#ifndef AEON_MEGAWONDERS_H
#define AEON_MEGAWONDERS_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct MegaWonder {
    int id = 0;
    std::string name;             // e.g. "Orbital Defense Ring"
    std::string type;             // e.g. "Space Megastructure"
    int owner_civ_id = -1;
    int tile_x = -1;
    int tile_y = -1;
    float construction_progress = 0.0f; // 0..100%
    bool completed = false;
    float gdp_multiplier = 1.25f;
};

class AeonMegaWonderEngine {
public:
    std::vector<MegaWonder> mega_wonders;

    AeonMegaWonderEngine();
    void init_megawonders(const AeonEngine& engine);
    void update_megawonders_tick(AeonEngine& engine);
    void start_construction(int civ_id, const std::string& name, AeonEngine& engine);
};

} // namespace Aeon

#endif // AEON_MEGAWONDERS_H
