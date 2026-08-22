#include "aeon_analytics.h"
#include "aeon_engine.h"
#include <algorithm>

namespace Aeon {

void AeonAnalyticsEngine::init() {
    series_data.clear();
    global_co2_history.clear();
    global_years.clear();
}

void AeonAnalyticsEngine::record_year(const AeonEngine& engine) {
    float cur_year = static_cast<float>(engine.year);
    global_years.push_back(cur_year);
    global_co2_history.push_back(engine.gis_climate_engine.global_co2_ppm);

    // Keep max 200 history entries
    if (global_years.size() > 200) {
        global_years.erase(global_years.begin());
        global_co2_history.erase(global_co2_history.begin());
    }

    for (const auto& civ : engine.civs) {
        if (civ.id < 0) continue;

        CivHistorySeries* s = nullptr;
        for (auto& series : series_data) {
            if (series.civ_id == civ.id) {
                s = &series;
                break;
            }
        }

        if (!s) {
            CivHistorySeries new_s;
            new_s.civ_id = civ.id;
            new_s.civ_name = civ.name;
            series_data.push_back(new_s);
            s = &series_data.back();
        }

        s->civ_name = civ.name;
        s->years.push_back(cur_year);
        s->gdp.push_back(civ.economy.gdp);
        s->treasury.push_back(civ.economy.annual_income);
        s->population.push_back(static_cast<float>(civ.population.total) / 1000.0f); // in Thousands
        s->military_power.push_back(civ.military_power);
        s->stability.push_back(civ.stability);
        s->tech_progress.push_back(civ.tech.progress + static_cast<int>(civ.tech.era) * 100.0f);

        // Cap to 200 data points
        if (s->years.size() > 200) {
            s->years.erase(s->years.begin());
            s->gdp.erase(s->gdp.begin());
            s->treasury.erase(s->treasury.begin());
            s->population.erase(s->population.begin());
            s->military_power.erase(s->military_power.begin());
            s->stability.erase(s->stability.begin());
            s->tech_progress.erase(s->tech_progress.begin());
        }
    }
}

const CivHistorySeries* AeonAnalyticsEngine::get_series(int civ_id) const {
    for (const auto& s : series_data) {
        if (s.civ_id == civ_id) return &s;
    }
    return nullptr;
}

} // namespace Aeon
