#ifndef AEON_GIS_CLIMATE_H
#define AEON_GIS_CLIMATE_H

#include <vector>
#include <string>
#include <cmath>

namespace Aeon {

class AeonEngine;

// Per-season food and movement multipliers (Spring/Summer/Autumn/Winter)
struct SeasonalModifiers {
    float food_mult[4]     = { 1.1f, 1.2f, 0.9f, 0.6f };  // Spring Summer Autumn Winter
    float movement_mult[4] = { 1.0f, 1.0f, 0.9f, 0.7f };  // Winter mud/snow slows armies
    float disease_mult[4]  = { 0.9f, 0.8f, 1.0f, 1.3f };  // Pandemic spreads faster in winter
};

struct GISTileCell {
    int x = 0;
    int y = 0;
    float elevation_m    = 100.0f; // Elevation in meters (0m to 8848m)
    float temperature_c  = 15.0f; // Temperature in degrees Celsius
    float rainfall_mm    = 500.0f; // Annual rainfall in mm
    std::string terrain_type = "Plains";
    // Accuracy additions
    float wind_direction  = 0.0f;  // 0-360 degrees prevailing wind
    float cloud_cover     = 0.3f;  // 0-1; affects temperature swings
    float soil_moisture   = 0.5f;  // 0-1; drives fertility
    float co2_level       = 280.0f;// local CO2 ppm
    float base_fertility  = 0.5f;  // baseline before depletion
    float current_fertility = 0.5f;// after soil depletion / climate shifts
    float seasonal_food_mult = 1.0f;// updated each month by seasonal calc
};

class AeonGISClimateEngine {
public:
    int map_width = 32;
    int map_height = 32;
    std::vector<GISTileCell> grid;
    SeasonalModifiers season_mods;
    float global_co2_ppm = 280.0f; // shared world CO2 level
    float global_temp_delta = 0.0f;// warming above baseline

    AeonGISClimateEngine();
    void init_map(int width, int height);
    void update_climate_physics(float season_factor);
    void update_seasonal_yield(int month);          // modulates food per season
    void apply_deforestation_effect(int x, int y);  // reduces local fertility & moisture
    void tick_year_climate(AeonEngine& engine);     // CO2 accumulation + climate drift

    float get_temperature_at(int x, int y) const;
    float get_elevation_at(int x, int y) const;
    float get_seasonal_food_mult(int month) const;  // 0-based month 0-11

private:
    void simulate_wind_patterns();
    void apply_rainfall_shadow();  // leeward of mountains get less rain
    void apply_co2_climate_feedback();
};

} // namespace Aeon

#endif // AEON_GIS_CLIMATE_H
