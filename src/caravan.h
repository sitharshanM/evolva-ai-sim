#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

struct MerchantCaravan {
    uint64_t  id                     = 0;
    int       faction_id             = 0;  // Home faction
    int       destination_faction_id = 0;  // Allied destination faction
    glm::vec2 pos                    {0.0f, 0.0f};
    glm::vec2 target_pos             {0.0f, 0.0f};
    float     energy_cargo           = 150.0f;
    float     speed                  = 95.0f;
    bool      alive                  = true;
    bool      returning              = false;
};
