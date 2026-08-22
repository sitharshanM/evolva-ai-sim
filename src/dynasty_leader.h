#pragma once
#include <string>
#include <vector>
#include "config.h"

struct LeaderPersonality {
    int         faction_id   = 0;
    std::string name         = "Unknown Ruler";
    std::string title        = "Emperor";
    std::string faction_name = "Tribe";
    std::string model        = "llama3.1";
    std::string system_prompt;
    std::string current_quote;
    float       aggression   = 0.5f;
    float       ruler_age    = 25.0f;
    std::string heir_name    = "Prince Valen";

    std::vector<std::string> relational_memory;
    void add_grievance(const std::string& g) {
        if (relational_memory.size() >= 5) relational_memory.erase(relational_memory.begin());
        relational_memory.push_back(g);
    }

    static LeaderPersonality get_preset(int faction_id);
};

struct LeaderTurnResult {
    int         faction_id     = 0;
    std::string leader_name;
    std::string speech;
    std::string action_type;    // DECLARE_WAR, FORM_ALLIANCE, PEACE_TREATY, CIVIL_WAR, MOBILIZE, HARVEST, NONE
    int         target_faction = -1;
    std::string treaty_name;
    std::string declaration;
};
