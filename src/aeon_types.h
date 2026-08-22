#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "aeon_config.h"

namespace Aeon {

// ─── Biome Types ──────────────────────────────────────────────────────────────
enum class Biome {
    OCEAN, SHALLOW_SEA, LAKE, RIVER,
    COAST, BEACH,
    PLAINS, GRASSLAND, FERTILE_VALLEY,
    FOREST, JUNGLE,
    DESERT, SCRUBLAND,
    TUNDRA, SNOW, ICE,
    MOUNTAIN, PEAK,
    SWAMP, MARSH,
    VOLCANO, CANYON
};

// ASCII symbol & colour for each biome
inline char biome_char(Biome b) {
    switch (b) {
        case Biome::OCEAN:          return '~';
        case Biome::SHALLOW_SEA:    return '~';
        case Biome::LAKE:           return 'O';
        case Biome::RIVER:          return 'o';
        case Biome::COAST:          return '=';
        case Biome::BEACH:          return '_';
        case Biome::PLAINS:         return '.';
        case Biome::GRASSLAND:      return ',';
        case Biome::FERTILE_VALLEY: return '"';
        case Biome::FOREST:         return '|';
        case Biome::JUNGLE:         return '#';
        case Biome::DESERT:         return ':';
        case Biome::SCRUBLAND:      return ';';
        case Biome::TUNDRA:         return '-';
        case Biome::SNOW:           return '*';
        case Biome::ICE:            return '+';
        case Biome::MOUNTAIN:       return '^';
        case Biome::PEAK:           return 'A';
        case Biome::SWAMP:          return '%';
        case Biome::MARSH:          return '&';
        case Biome::VOLCANO:        return '!';
        case Biome::CANYON:         return 'V';
    }
    return '?';
}

inline const char* biome_name(Biome b) {
    switch (b) {
        case Biome::OCEAN:          return "Ocean";
        case Biome::SHALLOW_SEA:    return "Shallow Sea";
        case Biome::LAKE:           return "Lake";
        case Biome::RIVER:          return "River";
        case Biome::COAST:          return "Coastline";
        case Biome::BEACH:          return "Beach";
        case Biome::PLAINS:         return "Plains";
        case Biome::GRASSLAND:      return "Grassland";
        case Biome::FERTILE_VALLEY: return "Fertile Valley";
        case Biome::FOREST:         return "Forest";
        case Biome::JUNGLE:         return "Jungle";
        case Biome::DESERT:         return "Desert";
        case Biome::SCRUBLAND:      return "Scrubland";
        case Biome::TUNDRA:         return "Tundra";
        case Biome::SNOW:           return "Snowfield";
        case Biome::ICE:            return "Ice Sheet";
        case Biome::MOUNTAIN:       return "Mountains";
        case Biome::PEAK:           return "Mountain Peak";
        case Biome::SWAMP:          return "Swamp";
        case Biome::MARSH:          return "Marshland";
        case Biome::VOLCANO:        return "Volcano";
        case Biome::CANYON:         return "Canyon";
    }
    return "Unknown";
}

// ─── Resource Deposits ────────────────────────────────────────────────────────────
enum class ResourceKind {
    FOOD, WATER, WOOD, STONE, IRON, COPPER, COAL, OIL, NATURAL_GAS,
    RARE_MINERALS, ENERGY, KNOWLEDGE, TECH, GOLD_ORE,
    SILICON, LITHIUM, HELIUM3
};

inline const char* resource_name(ResourceKind r) {
    switch (r) {
        case ResourceKind::FOOD:         return "Food";
        case ResourceKind::WATER:        return "Water";
        case ResourceKind::WOOD:         return "Wood";
        case ResourceKind::STONE:        return "Stone";
        case ResourceKind::IRON:         return "Iron";
        case ResourceKind::COPPER:       return "Copper";
        case ResourceKind::COAL:         return "Coal";
        case ResourceKind::OIL:          return "Oil";
        case ResourceKind::NATURAL_GAS:  return "Natural Gas";
        case ResourceKind::RARE_MINERALS:return "Rare Minerals";
        case ResourceKind::ENERGY:       return "Energy";
        case ResourceKind::KNOWLEDGE:    return "Knowledge";
        case ResourceKind::TECH:         return "Technology";
        case ResourceKind::GOLD_ORE:     return "Gold Ore";
        case ResourceKind::SILICON:      return "Silicon";
        case ResourceKind::LITHIUM:      return "Lithium";
        case ResourceKind::HELIUM3:      return "Helium-3";
    }
    return "Resource";
}

// ─── Map Tile ──────────────────────────────────────────────────────────────────────
struct MapTile {
    Biome   biome       = Biome::PLAINS;
    float   elevation   = 0.0f;     // 0.0 = sea level, 1.0 = peak
    float   moisture    = 0.5f;     // 0.0 = arid, 1.0 = swamp
    float   fertility   = 0.5f;
    bool    has_river   = false;
    bool    strategic   = false;    // mountain pass, natural harbor, etc.
    int     owner_civ   = -1;       // -1 = unclaimed / Commons
    int     city_id     = -1;
    std::vector<ResourceKind> resources;
    // Simulation accuracy additions
    float   soil_depletion    = 0.0f;  // 0-1; rises with over-farming
    float   co2_contribution  = 0.0f;  // CO2 emitted by industry on this tile
    float   fortification_level = 0.0f;// 0-5; built by military
    bool    is_contested      = false; // two civs fighting over it
};

// ─── Strategic Location Types ───────────────────────────────────────────────────────
enum class LandmarkType {
    MOUNTAIN_PASS, NATURAL_HARBOR, ANCIENT_RUINS,
    TRADE_JUNCTION, FERTILE_CROSSROADS, VOLCANIC_HOTSPOT,
    STRAIT_CHOKEPOINT, ANCIENT_LIBRARY, NATURAL_SPRING
};

struct Landmark {
    int           x = 0, y = 0;
    LandmarkType  type;
    std::string   name;
    std::string   description;
};

} // namespace Aeon
