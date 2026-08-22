#pragma once
#include <string>
#include <cstdint>
#include "region_ai.h"

struct CitizenProfile {
    uint64_t    organism_id  = 0;
    std::string name         = "Citizen";
    std::string profession   = "Vanguard Warrior";
    std::string personal_goal= "Survive and protect the tribe.";
    std::string backstory    = "Born under a blood-red sky.";
    std::string live_thought = "Searching for food...";

    int kills = 0;

    static CitizenProfile generate_random(uint64_t org_id, int faction_id, const RegionInfo& region);
};
