#pragma once
#include <glm/glm.hpp>
#include <vector>

enum class GodPowerType {
    NONE,
    SMITE,       // Key 1: Smite lightning
    GENESIS,     // Key 2: Paint food clusters
    STONE_WALL,  // Key 3: Raise stone barrier
    RALLY        // Key 4: Rally horn
};

struct StoneWall {
    glm::vec2 pos;
    float     radius   = 45.0f;
    float     lifetime = 25.0f; // seconds
    float     max_life = 25.0f;
};

struct SmiteEffect {
    glm::vec2 pos;
    float     radius   = 120.0f;
    float     lifetime = 1.2f;
    float     max_life = 1.2f;
};

class GodPowerManager {
public:
    GodPowerType active_power = GodPowerType::NONE;
    std::vector<StoneWall>   stone_walls;
    std::vector<SmiteEffect> smite_effects;

    void update(float dt) {
        for (auto& w : stone_walls) w.lifetime -= dt;
        stone_walls.erase(
            std::remove_if(stone_walls.begin(), stone_walls.end(),
                [](const StoneWall& w) { return w.lifetime <= 0.0f; }),
            stone_walls.end());

        for (auto& s : smite_effects) s.lifetime -= dt;
        smite_effects.erase(
            std::remove_if(smite_effects.begin(), smite_effects.end(),
                [](const SmiteEffect& s) { return s.lifetime <= 0.0f; }),
            smite_effects.end());
    }
};
