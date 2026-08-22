#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "config.h"

struct MemoryEntry {
    std::string event_desc;
    int         source_faction = -1;
    float       timestamp     = 0.0f;
    float       trust_impact  = 0.0f; // -100 to +100
    float       hatred_impact = 0.0f; // 0 to 100
    float       fear_impact   = 0.0f; // 0 to 100
};

struct ImperfectBeliefState {
    int   perceived_enemy_army = 45000;
    float perceived_threat_level = 0.40f;
    std::string secret_goal    = "Secretly construct nuclear/advanced deterrence shield";
};

class RulerAI {
public:
    int         faction_id   = 0;
    std::string ruler_name   = "Archon Thaelon";
    std::string title        = "High Emperor";

    // Personality Trait Vectors (0.0 to 1.0)
    float ambition       = 0.85f;
    float intelligence   = 0.90f;
    float patience       = 0.60f;
    float risk_tolerance = 0.45f;
    float morality       = 0.50f;
    float ego            = 0.80f;
    float fear           = 0.30f;
    float greed          = 0.70f;
    float loyalty        = 0.65f;

    // Primary & Secondary Goals
    std::string primary_goal   = "Become Richest Scientific Empire";
    std::vector<std::string> secondary_goals;

    // Memory Decay Matrix
    std::vector<MemoryEntry> memory_matrix;
    ImperfectBeliefState     belief;

    void update_memory_decay(float dt);
    void add_memory(const std::string& desc, int source_f, float trust, float hatred, float fear);
};
