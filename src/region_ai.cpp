#include "region_ai.h"

RegionInfo RegionInfo::get_region_for_pos(glm::vec2 pos) {
    RegionInfo r;
    bool north = (pos.y >= Config::WORLD_HEIGHT * 0.5f);
    bool east  = (pos.x >= Config::WORLD_WIDTH * 0.5f);

    if (north && !east) {
        r.id = 0;
        r.name = "Snowy Tundra";
        r.description = "Harsh frozen steppes with icy winds.";
        r.cultural_trait = "Glacial Fortitude (+Speed)";
        r.biome = BiomeType::TUNDRA;
        r.center_pos = {Config::WORLD_WIDTH * 0.25f, Config::WORLD_HEIGHT * 0.75f};
        r.color = {0.4f, 0.8f, 1.0f};
        r.ground_color = {0.08f, 0.14f, 0.20f};
        r.speed_mod = 1.15f;
        r.metabolism_mod = 1.25f;
    }
    else if (!north && !east) {
        r.id = 1;
        r.name = "Golden Plains";
        r.description = "Fertile plains with abundant crops and peaceful farms.";
        r.cultural_trait = "Harvest Bounty (-Aggression)";
        r.biome = BiomeType::PLAINS;
        r.center_pos = {Config::WORLD_WIDTH * 0.25f, Config::WORLD_HEIGHT * 0.25f};
        r.color = {1.0f, 0.85f, 0.3f};
        r.ground_color = {0.06f, 0.12f, 0.06f};
        r.aggression_mod = 0.85f;
    }
    else if (north && east) {
        r.id = 2;
        r.name = "Arid Desert";
        r.description = "Scorched sands rich in iron and ancient gold mines.";
        r.cultural_trait = "Desert Wrath (+Aggression)";
        r.biome = BiomeType::DESERT;
        r.center_pos = {Config::WORLD_WIDTH * 0.75f, Config::WORLD_HEIGHT * 0.75f};
        r.color = {1.0f, 0.6f, 0.2f};
        r.ground_color = {0.18f, 0.14f, 0.05f};
        r.aggression_mod = 1.35f;
    }
    else {
        r.id = 3;
        r.name = "Dark Swamp";
        r.description = "Dense murky marshland favoring stealthy hunters.";
        r.cultural_trait = "Swamp Venom (+Vision)";
        r.biome = BiomeType::SWAMP;
        r.center_pos = {Config::WORLD_WIDTH * 0.75f, Config::WORLD_HEIGHT * 0.25f};
        r.color = {0.5f, 0.3f, 0.8f};
        r.ground_color = {0.10f, 0.06f, 0.14f};
        r.speed_mod = 0.9f;
    }
    return r;
}
