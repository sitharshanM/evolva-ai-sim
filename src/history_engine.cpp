#include "history_engine.h"
#include <iostream>

void HistoryEngine::tick(float dt) {
    year_accumulator += dt;
    if (year_accumulator >= 12.0f) { // 1 year every 12s
        year_accumulator -= 12.0f;
        sim_year++;

        std::cout << "\n========================================================" << std::endl;
        std::cout << "📜 YEAR " << sim_year << " OF CIVILIZATION HAS BEGUN" << std::endl;
        std::cout << "========================================================\n" << std::endl;
    }
}

void HistoryEngine::log_event(const std::string& headline, const std::string& details, const std::string& category) {
    if (timeline.size() >= 500) timeline.erase(timeline.begin());
    HistoryTimelineEvent ev;
    ev.year = sim_year;
    ev.headline = headline;
    ev.details  = details;
    ev.category = category;
    timeline.push_back(ev);
}
