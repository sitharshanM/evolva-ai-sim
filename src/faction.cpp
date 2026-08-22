#include "faction.h"
#include <array>
#include <random>
#include <cmath>

static const std::array<const char*, 8> PREFIXES = {
    "Iron", "Verdant", "Shadow", "Sun", "Crimson", "Azure", "Golden", "Frost"
};

static const std::array<const char*, 8> SUFFIXES = {
    "Clan", "Hive", "Covenant", "Empire", "Dynasty", "Republic", "Dominion", "Tribe"
};

static const std::array<const char*, 5> IDEOLOGIES = {
    "Militarist", "Pacifist", "Expansionist", "Isolationist", "Mercantile"
};

std::string generate_faction_name(int id) {
    size_t p = static_cast<size_t>(id) % PREFIXES.size();
    size_t s = static_cast<size_t>(id * 3 + 1) % SUFFIXES.size();
    return std::string(PREFIXES[p]) + " " + SUFFIXES[s];
}

Faction Faction::create_random(int id, const std::string& name, glm::vec3 col) {
    Faction f;
    f.id = id;
    f.leader = LeaderPersonality::get_preset(id);
    f.name = name.empty() ? f.leader.faction_name : name;
    f.ideology = IDEOLOGIES[static_cast<size_t>(id) % IDEOLOGIES.size()];
    f.color = col;
    return f;
}
