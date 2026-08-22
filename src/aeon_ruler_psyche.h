#pragma once
#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct RulerPsychProfile {
    int civ_id = 0;
    std::string ruler_name;
    float paranoia = 15.0f;       // 0..100%
    float megalomania = 20.0f;    // 0..100%
    float ptsd_trauma = 0.0f;     // 0..100%
    float pragmatism = 60.0f;     // 0..100%
    float narcissism = 25.0f;     // 0..100%
    bool breakdown_active = false;
    std::string mental_state_status = "Stable Mind";
};

class AeonRulerPsycheEngine {
public:
    AeonRulerPsycheEngine() = default;

    void init(const AeonEngine& engine);
    void tick_year(AeonEngine& engine);
    void trigger_war_trauma(int civ_id, float severity, AeonEngine& engine);
    void trigger_cabinet_purge(int civ_id, AeonEngine& engine);

    std::vector<RulerPsychProfile> profiles;
};

} // namespace Aeon
