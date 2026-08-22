#include "citizen.h"
#include <array>
#include <random>

static const std::array<const char*, 10> FIRST_NAMES = {
    "Kael", "Lyra", "Thorne", "Vesper", "Boran", "Zephyr", "Rhea", "Darius", "Elora", "Marek"
};

static const std::array<const char*, 8> TITLES = {
    "Blood-Sworn", "the Swift", "of the Glade", "Iron-Bound", "the Vigilant", "Sun-Touched", "the Unyielding", "Shadow-Walker"
};

static const std::array<const char*, 6> PROFESSIONS = {
    "Vanguard Warrior", "Scout", "Herb Harvester", "Chieftain's Guard", "Frontier Defender", "Paladin"
};

static const std::array<const char*, 5> GOALS = {
    "Protect the regional borders from invaders.",
    "Gather food reserves for the upcoming winter.",
    "Avenge falling comrades in the ongoing war.",
    "Explore uncharted wilderness territory.",
    "Serve the Dynasty Leader with honor."
};

CitizenProfile CitizenProfile::generate_random(uint64_t org_id, int faction_id, const RegionInfo& region) {
    CitizenProfile p;
    p.organism_id = org_id;

    size_t f = static_cast<size_t>(org_id) % FIRST_NAMES.size();
    size_t t = static_cast<size_t>(org_id * 7 + faction_id) % TITLES.size();
    size_t pr = static_cast<size_t>(org_id * 3 + region.id) % PROFESSIONS.size();
    size_t g = static_cast<size_t>(org_id * 11) % GOALS.size();

    p.name         = std::string(FIRST_NAMES[f]) + " " + TITLES[t];
    p.profession   = PROFESSIONS[pr];
    p.personal_goal= GOALS[g];
    p.backstory    = "Born in " + region.name + " under the law of " + region.cultural_trait + ".";
    p.live_thought = "Patrolling " + region.name + " for food and enemy threats.";
    return p;
}
