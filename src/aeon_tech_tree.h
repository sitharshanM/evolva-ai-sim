#pragma once
#include "aeon_world_types.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace Aeon {

class AeonEngine;

enum class TechBranch {
    MILITARY,
    ECONOMICS,
    SCIENCE,
    MEDICINE,
    ENERGY,
    AI,
    INDUSTRY,
    AGRICULTURE,
    NAVAL,
    AEROSPACE
};

inline const char* tech_branch_name(TechBranch b) {
    switch (b) {
        case TechBranch::MILITARY:    return "Military";
        case TechBranch::ECONOMICS:   return "Economics & Commerce";
        case TechBranch::SCIENCE:     return "Fundamental Science";
        case TechBranch::MEDICINE:    return "Medicine & Healthcare";
        case TechBranch::ENERGY:      return "Energy & Power";
        case TechBranch::AI:          return "Artificial Intelligence & Cybernetics";
        case TechBranch::INDUSTRY:    return "Heavy Industry & Automation";
        case TechBranch::AGRICULTURE: return "Agronomy & Biotechnology";
        case TechBranch::NAVAL:       return "Naval & Maritime Systems";
        case TechBranch::AEROSPACE:   return "Aerospace & Orbital Engineering";
    }
    return "Science";
}

struct TechNode {
    std::string id;
    std::string name;
    std::string description;
    TechEra era = TechEra::AGRICULTURE;
    TechBranch branch = TechBranch::SCIENCE;
    float research_cost = 100.0f;
    std::vector<std::string> prerequisites;
    bool unlocked = false;
    
    // Mechanical Modifiers
    float food_yield_mult = 1.0f;
    float industry_mult   = 1.0f;
    float military_mult   = 1.0f;
    float trade_mult      = 1.0f;
    float science_mult    = 1.0f;
    float health_mult     = 1.0f;
    float energy_mult     = 1.0f;
    bool  unlocks_orbital = false;
    bool  unlocks_nukes   = false;
};

enum class IdeologyTree { MILITARISM, SCIENTISM, MERCANTILISM, DIVINE_EMPIRE };

inline const char* ideology_tree_name(IdeologyTree i) {
    switch (i) {
        case IdeologyTree::MILITARISM:    return "Militarism & Conquest";
        case IdeologyTree::SCIENTISM:     return "Scientism & Innovation";
        case IdeologyTree::MERCANTILISM:  return "Mercantilism & Commerce";
        case IdeologyTree::DIVINE_EMPIRE: return "Divine Imperialism";
    }
    return "Neutral";
}

struct IdeologyPolicy {
    std::string id;
    std::string name;
    IdeologyTree tree;
    int tier = 1;
    bool active = false;
    std::string bonus_desc;
};

class TechTreeEngine {
public:
    TechTreeEngine();

    std::vector<TechNode> nodes;
    std::vector<IdeologyPolicy> policies;

    void init_default_tree();
    
    bool can_research(int civ_tech_pts, const std::string& tech_id, const std::vector<std::string>& unlocked_list) const;
    const TechNode* find_node(const std::string& tech_id) const;
    
    void apply_tech_effects(struct AeonCivilization& civ);
    void process_ideology_cold_wars(AeonEngine& engine);
    void process_brain_drain_tick(AeonEngine& engine);
};

} // namespace Aeon

