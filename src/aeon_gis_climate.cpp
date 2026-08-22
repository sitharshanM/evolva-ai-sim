#include "aeon_gis_climate.h"
#include "aeon_engine.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

#include "aeon_config.h"

namespace Aeon {

AeonGISClimateEngine::AeonGISClimateEngine() {
    init_map(MAP_WIDTH, MAP_HEIGHT);
}

void AeonGISClimateEngine::init_map(int width, int height) {
    map_width = width;
    map_height = height;
    grid.clear();
    grid.reserve(width * height);

    for (int y = 0; y < height; ++y) {
        float lat = 90.0f - ((float)y / (float)height) * 180.0f; // +90N to -90S
        for (int x = 0; x < width; ++x) {
            float lon = ((float)x / (float)width) * 360.0f - 180.0f; // -180W to +180E
            GISTileCell cell;
            cell.x = x;
            cell.y = y;

            // Accurate Real-World Earth Elevation in meters
            float elev = 50.0f; // Sea level / coastal baseline

            // Himalayas & Tibetan Plateau (lon 75..102E, lat 27..38N)
            if (lon >= 75.0f && lon <= 102.0f && lat >= 27.0f && lat <= 38.0f) {
                elev = 4800.0f;
            }
            // Andes Mountains (lon -80..-65W, lat -55..10S)
            else if (lon >= -80.0f && lon <= -65.0f && lat >= -55.0f && lat <= 10.0f) {
                elev = 3900.0f;
            }
            // Rocky Mountains (lon -122..-104W, lat 32..60N)
            else if (lon >= -122.0f && lon <= -104.0f && lat >= 32.0f && lat <= 60.0f) {
                elev = 3100.0f;
            }
            // European Alps (lon 6..15E, lat 45..48N)
            else if (lon >= 6.0f && lon <= 15.0f && lat >= 45.0f && lat <= 48.0f) {
                elev = 2800.0f;
            }
            // Ural Mountains (lon 58..65E, lat 50..68N)
            else if (lon >= 58.0f && lon <= 65.0f && lat >= 50.0f && lat <= 68.0f) {
                elev = 1600.0f;
            }
            // Ethiopian Highlands & East African Rift
            else if (lon >= 35.0f && lon <= 42.0f && lat >= 5.0f && lat <= 15.0f) {
                elev = 2400.0f;
            }
            // Great Plains / European Lowlands / Amazon / Siberian Plains
            else if ((lon >= -104.0f && lon <= -88.0f && lat >= 30.0f && lat <= 50.0f) ||
                     (lon >= 0.0f && lon <= 50.0f && lat >= 48.0f && lat <= 60.0f) ||
                     (lon >= -75.0f && lon <= -50.0f && lat >= -15.0f && lat <= 5.0f)) {
                elev = 250.0f;
            } else {
                elev = 400.0f;
            }

            cell.elevation_m = elev;
            cell.co2_level   = 280.0f;

            if (cell.elevation_m > 3500.0f) cell.terrain_type = "Himalayan/Andean Peak";
            else if (cell.elevation_m > 1800.0f) cell.terrain_type = "Alpine Highland";
            else if (cell.elevation_m > 500.0f) cell.terrain_type = "Continental Plateau";
            else cell.terrain_type = "Fertile Lowlands";

            // Prevailing Trade Winds & Westerlies (Hadley & Ferrel Cells)
            if (std::abs(lat) < 30.0f) {
                cell.wind_direction = (lat >= 0) ? 70.0f : 110.0f; // Northeast / Southeast Trade Winds
            } else {
                cell.wind_direction = 270.0f; // Mid-Latitude Westerlies
            }

            cell.cloud_cover     = 0.35f + 0.25f * std::cos((lat / 180.0f) * 3.14159f);
            cell.soil_moisture   = 0.50f;
            cell.base_fertility  = 0.4f + 0.4f * std::max(0.0f, 1.0f - (cell.elevation_m / 4500.0f));
            cell.current_fertility = cell.base_fertility;

            grid.push_back(cell);
        }
    }
    update_climate_physics(0.0f);
    simulate_wind_patterns();
    apply_rainfall_shadow();
}


// ─── update_climate_physics ───────────────────────────────────────────────────
void AeonGISClimateEngine::update_climate_physics(float season_factor) {
    for (auto& cell : grid) {
        // Equator latitude effect (middle of map is warmest)
        float lat_dist = std::abs((float)cell.y / (float)map_height - 0.5f) * 2.0f;
        float base_temp = 32.0f - (lat_dist * 40.0f);

        // Elevation environmental lapse rate (-6.5°C per 1000m)
        float lapse_cooling = (cell.elevation_m / 1000.0f) * 6.5f;

        // Seasonal variation (-8°C winter to +8°C summer), amplified near poles
        float season_amplitude = 8.0f + lat_dist * 12.0f;
        float seasonal_delta = std::sin(season_factor) * season_amplitude;

        // CO2 warming: +0.01°C per ppm above 280
        float co2_warming = (global_co2_ppm - 280.0f) * 0.01f;

        cell.temperature_c = base_temp - lapse_cooling + seasonal_delta + co2_warming;
        cell.rainfall_mm   = std::max(80.0f,
            1200.0f - (lat_dist * 800.0f) - (cell.elevation_m * 0.1f) + (cell.cloud_cover * 200.0f));

        // Update seasonal food multiplier for this cell (used by civ tick)
        cell.seasonal_food_mult = get_seasonal_food_mult(
            static_cast<int>(std::fmod(season_factor / 6.28f * 12.0f + 12.0f, 12.0f)));
    }
}

// ─── update_seasonal_yield ────────────────────────────────────────────────────
void AeonGISClimateEngine::update_seasonal_yield(int month) {
    // month: 1-12
    for (auto& cell : grid) {
        cell.seasonal_food_mult = get_seasonal_food_mult(month - 1);
        // Drought effect: if rainfall_mm < 200 → fertility penalty
        if (cell.rainfall_mm < 200.0f) {
            cell.current_fertility = std::max(0.05f, cell.current_fertility - 0.002f);
        }
    }
}

// ─── get_seasonal_food_mult ───────────────────────────────────────────────────
float AeonGISClimateEngine::get_seasonal_food_mult(int month) const {
    // month: 0-11
    int season = (month / 3) % 4; // 0=Spring 1=Summer 2=Autumn 3=Winter
    return season_mods.food_mult[season];
}

// ─── apply_deforestation_effect ───────────────────────────────────────────────
void AeonGISClimateEngine::apply_deforestation_effect(int x, int y) {
    if (x < 0 || x >= map_width || y < 0 || y >= map_height) return;
    auto& cell = grid[y * map_width + x];
    // Deforestation reduces soil moisture and fertility, raises CO2
    cell.soil_moisture      = std::max(0.1f, cell.soil_moisture - 0.05f);
    cell.current_fertility  = std::max(0.05f, cell.current_fertility - 0.03f);
    cell.co2_level         += 2.0f;
    global_co2_ppm         += 0.1f;
}

// ─── tick_year_climate ────────────────────────────────────────────────────────
void AeonGISClimateEngine::tick_year_climate(AeonEngine& engine) {
    // Industrial civs emit CO2
    for (const auto& civ : engine.civs) {
        if (civ.is_alive <= 0.0f) continue;
        if (civ.tech.era >= TechEra::INDUSTRIALIZATION) {
            float co2_added = civ.territory_tiles * 0.02f;
            global_co2_ppm += co2_added;
        }
    }
    global_co2_ppm = std::min(global_co2_ppm, 1200.0f); // hard cap
    global_temp_delta = (global_co2_ppm - 280.0f) * 0.01f;

    apply_co2_climate_feedback();
}

// ─── simulate_wind_patterns ───────────────────────────────────────────────────
void AeonGISClimateEngine::simulate_wind_patterns() {
    for (auto& cell : grid) {
        // Trade winds blow east near equator, westerlies blow west in mid-latitudes
        float lat = (float)cell.y / (float)map_height; // 0=north, 1=south
        if (lat > 0.3f && lat < 0.7f) {
            cell.wind_direction = 270.0f; // westerlies
        } else {
            cell.wind_direction = 90.0f;  // trade winds eastward
        }
        // Polar easterlies
        if (lat < 0.15f || lat > 0.85f) {
            cell.wind_direction = 90.0f;
        }
    }
}

// ─── apply_rainfall_shadow ───────────────────────────────────────────────────
void AeonGISClimateEngine::apply_rainfall_shadow() {
    // Tiles directly east of high mountains get 40% less rainfall (rain shadow)
    for (int y = 0; y < map_height; ++y) {
        for (int x = 1; x < map_width; ++x) {
            const auto& west_cell = grid[y * map_width + (x - 1)];
            auto& cell            = grid[y * map_width + x];
            if (west_cell.elevation_m > 2500.0f) {
                cell.rainfall_mm *= 0.60f; // 40% reduction leeward of mountains
                cell.soil_moisture = std::max(0.05f, cell.soil_moisture - 0.15f);
            }
        }
    }
}

// ─── apply_co2_climate_feedback ──────────────────────────────────────────────
void AeonGISClimateEngine::apply_co2_climate_feedback() {
    if (global_co2_ppm <= 400.0f) return;
    float excess = global_co2_ppm - 400.0f;
    for (auto& cell : grid) {
        // Arid tiles become more arid; fertile coastal zones flood risk rises
        if (cell.rainfall_mm < 300.0f) {
            // Desert expansion: reduce fertility in already dry tiles
            cell.current_fertility -= excess * 0.0001f;
            cell.current_fertility  = std::max(0.01f, cell.current_fertility);
        }
        if (cell.elevation_m < 50.0f && cell.terrain_type == "Coastal Lowlands") {
            // Sea level rise damages coastal fertility
            cell.current_fertility -= excess * 0.00005f;
            cell.current_fertility  = std::max(0.01f, cell.current_fertility);
        }
    }
}

float AeonGISClimateEngine::get_temperature_at(int x, int y) const {
    if (x < 0 || x >= map_width || y < 0 || y >= map_height) return 15.0f;
    return grid[y * map_width + x].temperature_c;
}

float AeonGISClimateEngine::get_elevation_at(int x, int y) const {
    if (x < 0 || x >= map_width || y < 0 || y >= map_height) return 100.0f;
    return grid[y * map_width + x].elevation_m;
}

} // namespace Aeon
