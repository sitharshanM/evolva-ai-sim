#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "config.h"

enum class BiomeType { PLAINS, DESERT, TUNDRA, SWAMP };

inline const char* biome_type_str(BiomeType b) {
    switch (b) {
        case BiomeType::PLAINS: return "Lush Plains";
        case BiomeType::DESERT: return "Arid Desert";
        case BiomeType::TUNDRA: return "Snowy Tundra";
        case BiomeType::SWAMP:  return "Dark Swamp";
    }
    return "Wildlands";
}

struct RegionInfo {
    int         id             = 0;
    std::string name           = "Wildlands";
    std::string description    = "Untamed wilderness.";
    std::string cultural_trait = "Hardy Survival";
    BiomeType   biome          = BiomeType::PLAINS;
    glm::vec2   center_pos     {0.0f, 0.0f};
    glm::vec3   color          {1.0f, 1.0f, 1.0f};
    glm::vec3   ground_color   {0.1f, 0.12f, 0.1f};

    float speed_mod      = 1.0f;
    float aggression_mod = 1.0f;
    float metabolism_mod = 1.0f;

    static RegionInfo get_region_for_pos(glm::vec2 pos);
};
