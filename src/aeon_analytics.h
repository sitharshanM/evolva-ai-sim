#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace Aeon {

class AeonEngine;

struct CivHistorySeries {
    int civ_id = 0;
    std::string civ_name;
    std::vector<float> years;
    std::vector<float> gdp;
    std::vector<float> treasury;
    std::vector<float> population;
    std::vector<float> military_power;
    std::vector<float> stability;
    std::vector<float> tech_progress;
};

class AeonAnalyticsEngine {
public:
    AeonAnalyticsEngine() = default;

    void init();
    void record_year(const AeonEngine& engine);

    std::vector<CivHistorySeries> series_data;
    std::vector<float> global_co2_history;
    std::vector<float> global_years;

    // Helper to fetch history for a civ
    const CivHistorySeries* get_series(int civ_id) const;
};

} // namespace Aeon
