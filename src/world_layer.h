#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "config.h"

// 13 Strategic Resource Types
enum class ResourceType {
    FOOD, WATER, WOOD, STONE, IRON, COAL, OIL, GOLD, URANIUM, ENERGY, RARE_MINERALS, TECH_PARTS, LABOR
};

inline const char* resource_type_str(ResourceType r) {
    switch (r) {
        case ResourceType::FOOD:          return "Grain & Food 🌾";
        case ResourceType::WATER:         return "Fresh Water 💧";
        case ResourceType::WOOD:          return "Timber Wood 🪵";
        case ResourceType::STONE:         return "Quarry Stone 🪨";
        case ResourceType::IRON:          return "Iron Ore ⛏️";
        case ResourceType::COAL:          return "Coal Deposit ⬛";
        case ResourceType::OIL:           return "Crude Oil 🛢️";
        case ResourceType::GOLD:          return "Gold Bullion 🪙";
        case ResourceType::URANIUM:       return "Uranium Ore ☢️";
        case ResourceType::ENERGY:        return "Power Grid Energy ⚡";
        case ResourceType::RARE_MINERALS: return "Rare Minerals 💎";
        case ResourceType::TECH_PARTS:    return "Electronics Tech 🔌";
        case ResourceType::LABOR:         return "Human Labor 👷";
    }
    return "Resource";
}

struct InfrastructureNode {
    glm::vec2   pos{0.0f, 0.0f};
    std::string type = "Road"; // Road, Bridge, Port, PowerPlant, Base, University
    int         faction_id = -1;
    float       level      = 1.0f;
};

struct WorldDisaster {
    glm::vec2   pos{0.0f, 0.0f};
    std::string type = "Drought"; // Earthquake, Drought, Flood, Pandemic, Wildfire
    float       radius   = 200.0f;
    float       duration = 45.0f;
};

class WorldLayer {
public:
    WorldLayer() = default;

    void init();
    void update(float dt);

    std::vector<InfrastructureNode> infrastructure;
    std::vector<WorldDisaster>     disasters;

    float global_temperature = 22.0f; // Celsius
    float global_pollution   = 0.05f;
};
