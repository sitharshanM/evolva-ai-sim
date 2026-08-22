#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "config.h"

#include "dynasty_leader.h"

struct Outpost {
    glm::vec2 pos{0.0f, 0.0f};
    float health     = 100.0f;
    float max_health = 100.0f;
};

enum class SettlementTier { CAMPSITE, VILLAGE, TOWN, METROPOLIS };
enum class GovernmentType { MONARCHY, REPUBLIC, THEOCRACY, MARTIAL_STATE };

inline const char* settlement_tier_str(SettlementTier s) {
    switch (s) {
        case SettlementTier::CAMPSITE:   return "Campsite";
        case SettlementTier::VILLAGE:    return "Village";
        case SettlementTier::TOWN:       return "Town";
        case SettlementTier::METROPOLIS: return "Metropolis 🏰";
    }
    return "Campsite";
}

inline const char* government_type_str(GovernmentType g) {
    switch (g) {
        case GovernmentType::MONARCHY:      return "Absolute Monarchy 👑";
        case GovernmentType::REPUBLIC:      return "Merchant Republic 🏛️";
        case GovernmentType::THEOCRACY:      return "Divine Theocracy 🔮";
        case GovernmentType::MARTIAL_STATE:  return "Martial Autocracy ⚔️";
    }
    return "Monarchy";
}

struct Faction {
    int            id             = 0;
    std::string    name           = "Unnamed Tribe";
    std::string    ideology       = "Expansionist";
    glm::vec3      color          {1.0f, 1.0f, 1.0f};
    glm::vec2      capital_pos    {0.0f, 0.0f};
    
    SettlementTier tier           = SettlementTier::CAMPSITE;
    GovernmentType government     = GovernmentType::MONARCHY;

    // Leader persona
    LeaderPersonality leader;

    int   member_count         = 0;
    float territory_pct        = 0.0f;  // 0 to 100
    float military_power       = 0.0f;
    
    // Multi-resource economy stockpiles
    float treasury_food        = 200.0f;
    float resource_wood        = 50.0f;
    float resource_iron        = 20.0f;
    float resource_gold        = 10.0f;
    float faith_points         = 30.0f;

    // Tech Tree Research Levels (0 to 5)
    int tech_metallurgy        = 1;
    int tech_agriculture       = 1;
    int tech_fortification     = 1;
    int tech_navigation        = 1;

    float tax_rate             = 0.20f; // 20% default tax
    float border_radius        = 250.0f;

    std::vector<Outpost> outposts;

    // Modifiers set by political decrees/ideology
    float aggression_mod       = 1.0f;
    float speed_mod            = 1.0f;
    float defense_mod          = 1.0f;

    static Faction create_random(int id, const std::string& name = "", glm::vec3 col = {1,1,1});
};

// Generates cool faction names like "Iron Blood Clan", "Verdant Empire", "Shadow Covenant"
std::string generate_faction_name(int id);
