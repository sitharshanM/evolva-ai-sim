#ifndef AEON_DEMOGRAPHICS_H
#define AEON_DEMOGRAPHICS_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct PopulationPyramid {
    int civ_id = 0;
    std::string civ_name = "Global Civ";
    long long youth_count = 1200000;      // Age 0-18
    long long working_count = 3500000;    // Age 19-64
    long long senior_count = 800000;      // Age 65+
    float birth_rate_per_1000 = 14.5f;
    float death_rate_per_1000 = 8.2f;
    float dependency_ratio = 0.57f;        // (Youth + Senior) / Working
};

struct RefugeeWave {
    int origin_civ_id = -1;
    int target_civ_id = -1;
    long long displacement_count = 0;
    std::string reason; // WAR, DISASTER, FAMINE
};

struct PandemicStatus {
    std::string pathogen_name = "None";
    bool active = false;
    float R0 = 2.4f;                      // Basic Reproduction Number
    long long susceptible = 0;
    long long exposed = 0;
    long long infectious = 0;
    long long recovered = 0;
    float mortality_rate = 0.02f;         // 2% mortality
    bool quarantine_active = false;
    float vaccine_progress = 0.0f;        // 0.0 to 1.0
    // Accuracy additions
    float seasonal_spread_mult = 1.0f;    // set by climate engine each season
    float underreporting_factor = 2.5f;   // true cases = reported * this
    int   origin_civ_id = -1;
    int   origin_year   = 0;
};

struct EthnicGroup {
    int         civ_id   = -1;
    std::string name;
    long long   population = 0;
    float       autonomy_demand = 0.0f; // 0-100; high → separatist pressure
    float       cultural_integration = 50.0f; // 0-100
    bool        has_separatist_movement = false;
};

class AeonDemographicsEngine {
public:
    std::vector<PopulationPyramid> pyramids;
    std::vector<RefugeeWave>       refugees;
    std::vector<EthnicGroup>       ethnic_groups;
    PandemicStatus                 global_pandemic;

    AeonDemographicsEngine();
    void init_default_demographics();
    void update_demographics_year(AeonEngine& engine);
    void trigger_outbreak(const std::string& pathogen, long long initial_cases);
    float calculate_transit_delay_hours(float distance_km, float terrain_cost_mult) const;
    // New accuracy methods
    void update_urbanization(AeonEngine& engine);
    void apply_inequality_feedback(AeonEngine& engine);
    void apply_generational_education(AeonEngine& engine);
    void process_refugee_intake(AeonEngine& engine);
    void advance_pandemic_season(AeonEngine& engine, int month);
};

} // namespace Aeon

#endif // AEON_DEMOGRAPHICS_H
