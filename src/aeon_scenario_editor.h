#pragma once
#include "aeon_types.h"
#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

enum class EditorBrushMode {
    PAINT_BIOME        = 0,
    PAINT_SOVEREIGNTY  = 1,
    PLACE_RESOURCE     = 2,
    PLACE_LANDMARK     = 3
};

const char* brush_mode_name(EditorBrushMode mode);

class AeonScenarioEditorEngine {
public:
    AeonScenarioEditorEngine() = default;

    void init();
    void apply_brush(AeonEngine& engine, int tile_x, int tile_y);
    bool save_scenario(const AeonEngine& engine, const std::string& scenario_name);
    bool load_scenario(AeonEngine& engine, const std::string& scenario_name);

    EditorBrushMode current_brush = EditorBrushMode::PAINT_BIOME;
    Biome selected_biome = Biome::PLAINS;
    int selected_civ_owner = 0;
    ResourceKind selected_resource = ResourceKind::GOLD_ORE;
    int brush_radius = 1;
};

} // namespace Aeon
