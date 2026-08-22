#pragma once
#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

enum class ShadowSociety {
    OBSIDIAN_CITADEL = 0, // Shadow Military-Industrial Complex
    TECHNOCRATIC_APEX = 1,// Surveillance & Tech Oligarchy
    SOLAR_SYNDICATE  = 2  // Black-Market Banking & Syndicate
};

const char* shadow_society_name(ShadowSociety soc);

struct DeepStateCell {
    ShadowSociety society = ShadowSociety::OBSIDIAN_CITADEL;
    int civ_id = 0;
    std::string civ_name;
    float infiltration_pct = 25.0f; // 0..100%
    float shadow_funds_gold = 500.0f;
    bool coup_plot_active = false;
};

class AeonDeepStateEngine {
public:
    AeonDeepStateEngine() = default;

    void init(const AeonEngine& engine);
    void tick_year(AeonEngine& engine);
    bool launch_shadow_coup(int civ_id, ShadowSociety soc, AeonEngine& engine);
    void siphoning_treasury(int civ_id, float amount, AeonEngine& engine);

    std::vector<DeepStateCell> cells;
};

} // namespace Aeon
