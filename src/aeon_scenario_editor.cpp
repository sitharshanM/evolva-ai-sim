#include "aeon_scenario_editor.h"
#include "aeon_engine.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace Aeon {

const char* brush_mode_name(EditorBrushMode mode) {
    switch (mode) {
        case EditorBrushMode::PAINT_BIOME:       return "🖌️ Biome Painter";
        case EditorBrushMode::PAINT_SOVEREIGNTY: return "🚩 Sovereignty Border Painter";
        case EditorBrushMode::PLACE_RESOURCE:    return "💎 Strategic Resource Placer";
        case EditorBrushMode::PLACE_LANDMARK:    return "🗿 Landmark & Canal Placer";
    }
    return "Brush";
}

void AeonScenarioEditorEngine::init() {
    current_brush = EditorBrushMode::PAINT_BIOME;
    selected_biome = Biome::PLAINS;
    selected_civ_owner = 0;
    selected_resource = ResourceKind::GOLD_ORE;
    brush_radius = 1;
}

void AeonScenarioEditorEngine::apply_brush(AeonEngine& engine, int tile_x, int tile_y) {
    for (int dy = -brush_radius + 1; dy < brush_radius; ++dy) {
        for (int dx = -brush_radius + 1; dx < brush_radius; ++dx) {
            int tx = tile_x + dx;
            int ty = tile_y + dy;
            if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT) continue;

            auto& tile = engine.world_map.tile(tx, ty);

            switch (current_brush) {
                case EditorBrushMode::PAINT_BIOME:
                    tile.biome = selected_biome;
                    if (selected_biome == Biome::PEAK || selected_biome == Biome::MOUNTAIN) tile.elevation = 0.85f;
                    else if (selected_biome == Biome::OCEAN) tile.elevation = 0.10f;
                    break;

                case EditorBrushMode::PAINT_SOVEREIGNTY:
                    engine.world_map.set_owner(tx, ty, selected_civ_owner);
                    break;

                case EditorBrushMode::PLACE_RESOURCE:
                    tile.resources.push_back(selected_resource);
                    break;

                case EditorBrushMode::PLACE_LANDMARK:
                    tile.strategic = true;
                    Landmark lm;
                    lm.x = tx; lm.y = ty;
                    lm.type = LandmarkType::ANCIENT_RUINS;
                    lm.name = "Custom Wonder Landmark";
                    lm.description = "A player-created world wonder node.";
                    engine.world_map.landmarks.push_back(lm);
                    break;
            }
        }
    }
}

bool AeonScenarioEditorEngine::save_scenario(const AeonEngine& engine, const std::string& scenario_name) {
    nlohmann::json j;
    j["scenario_name"] = scenario_name;
    j["year"] = engine.year;

    nlohmann::json civs_j = nlohmann::json::array();
    for (const auto& c : engine.civs) {
        nlohmann::json c_obj;
        c_obj["id"] = c.id;
        c_obj["name"] = c.name;
        c_obj["capital_x"] = c.capital_x;
        c_obj["capital_y"] = c.capital_y;
        civs_j.push_back(c_obj);
    }
    j["civs"] = civs_j;

    std::string path = "scenarios/" + scenario_name + ".json";
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << j.dump(4);
    std::cout << "[SCENARIO EDITOR] Saved scenario to " << path << std::endl;
    return true;
}

bool AeonScenarioEditorEngine::load_scenario(AeonEngine& engine, const std::string& scenario_name) {
    std::string path = "scenarios/" + scenario_name + ".json";
    std::ifstream file(path);
    if (!file.is_open()) return false;

    nlohmann::json j;
    file >> j;
    if (j.contains("year")) engine.year = j["year"];

    std::cout << "[SCENARIO EDITOR] Loaded scenario from " << path << std::endl;
    return true;
}

} // namespace Aeon
