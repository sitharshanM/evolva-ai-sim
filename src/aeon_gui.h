#pragma once
#include "aeon_engine.h"
#include <string>

struct GLFWwindow;

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  AeonGUI  —  Graphical User Interface for AEON Simulator (ImGui + GLFW)
// ─────────────────────────────────────────────────────────────────────────────
class AeonGUI {
public:
    AeonGUI() = default;
    ~AeonGUI();

    bool init(const char* title = "AEON -- Emergent AI Civilization Simulator", int width = 1280, int height = 800);
    void shutdown();

    bool should_close() const;
    void render_frame(AeonEngine& engine);

private:
    GLFWwindow* window_ = nullptr;
    void apply_modern_theme();

    int selected_civ_id_ = 0;
    int selected_tile_x_ = -1;
    int selected_tile_y_ = -1;
    int active_category_ = 0;

    // Interactive world map state
    float map_zoom_   = 4.0f;   // pixels per tile (adjusted for 360x180)
    float map_pan_x_  = 0.0f;
    float map_pan_y_  = 0.0f;
    bool  map_dragging_ = false;
    float map_drag_start_x_ = 0.0f;
    float map_drag_start_y_ = 0.0f;
    float map_pan_start_x_ = 0.0f;
    float map_pan_start_y_ = 0.0f;

    // Real OpenStreetMap & Satellite Map API GPU Texture Handles
    unsigned int osm_dark_texture_id_      = 0;
    unsigned int osm_street_texture_id_    = 0;
    unsigned int osm_satellite_texture_id_ = 0;
    int osm_map_provider_ = 0; // 0: CartoDB Dark OpenStreetMap, 1: Standard OpenStreetMap, 2: Esri World Satellite
    void load_osm_map_picture_textures();

    // Map Overlays: 0: Sovereign Borders, 1: Climate/Temp, 2: Demographics/Refugees, 3: Military/Frontlines, 4: Trade Routes
    int map_overlay_mode_ = 0;



    char save_filename_buf_[128] = "my_aeon_universe";
    char model_change_buf_[64]   = "llama3.1";
    char custom_law_buf_[256]    = "";
    char ruler_chat_buf_[256]    = "";
    char god_custom_decree_buf_[512] = "Trigger a massive gold rush across all empires!";
    std::string ruler_chat_history_;

    void draw_control_bar(AeonEngine& engine);
    void draw_president_tab(AeonEngine& engine);
    void draw_space_espionage_tab(AeonEngine& engine);
    void draw_ruler_chat_tab(AeonEngine& engine);
    void draw_tech_tree_tab(AeonEngine& engine);
    void draw_coalitions_tab(AeonEngine& engine);
    void draw_trade_routes_tab(AeonEngine& engine);
    void draw_analytics_tab(AeonEngine& engine);
    void draw_policies_tab(AeonEngine& engine);
    void draw_world_map_tab(AeonEngine& engine);
    void draw_empires_tab(AeonEngine& engine);
    void draw_economy_tab(AeonEngine& engine);
    void draw_factions_tab(AeonEngine& engine);
    void draw_religion_tab(AeonEngine& engine);
    void draw_chronicle_tab(AeonEngine& engine);
    void draw_disasters_tab(AeonEngine& engine);
    void draw_wonders_tab(AeonEngine& engine);
    void draw_stock_market_tab(AeonEngine& engine);
    void draw_naval_tab(AeonEngine& engine);
    void draw_parliament_tab(AeonEngine& engine);
    void draw_un_council_tab(AeonEngine& engine);
    void draw_alliances_tab(AeonEngine& engine);
    void draw_persistence_tab(AeonEngine& engine);
    void draw_gis_climate_tab(AeonEngine& engine);
    void draw_supply_demand_tab(AeonEngine& engine);
    void draw_demographics_tab(AeonEngine& engine);
    void draw_military_logistics_tab(AeonEngine& engine);
    void draw_central_banking_tab(AeonEngine& engine);
    void draw_agent_control_tab(AeonEngine& engine);
    void draw_rebellion_tab(AeonEngine& engine);
    void draw_nuclear_tab(AeonEngine& engine);
    void draw_space_race_tab(AeonEngine& engine);
    void draw_dynasty_tab(AeonEngine& engine);
    void draw_megawonders_tab(AeonEngine& engine);
    void draw_maritime_tab(AeonEngine& engine);
    void draw_citizens_tab(AeonEngine& engine);
    void draw_god_mode_tab(AeonEngine& engine);
    void draw_interactive_event_modal(AeonEngine& engine);
};

} // namespace Aeon
