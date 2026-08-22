#ifndef AEON_WONDERS_H
#define AEON_WONDERS_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct WorldWonder {
    int id = 0;
    std::string name;
    std::string description;
    float cost_gold = 100000.0f;
    float invested_gold = 0.0f;
    bool is_built = false;
    int built_year = -1;
    int builder_civ_id = -1;
    std::string passive_perk_text;
};

class AeonWonderEngine {
public:
    std::vector<WorldWonder> wonders;

    void init();
    void tick_year(AeonEngine& engine);
    bool invest_in_wonder(AeonEngine& engine, int wonder_id, float amount);
};

} // namespace Aeon

#endif // AEON_WONDERS_H
