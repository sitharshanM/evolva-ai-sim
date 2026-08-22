#pragma once
#include <string>
#include <vector>

struct HistoryTimelineEvent {
    int         year;
    std::string headline;
    std::string details;
    std::string category; // WAR, CIVILIZATION, DIPLOMACY, BLACK_SWAN
};

class HistoryEngine {
public:
    HistoryEngine() = default;

    int sim_year = 1;
    float year_accumulator = 0.0f; // 12 seconds per year

    std::vector<HistoryTimelineEvent> timeline;

    void tick(float dt);
    void log_event(const std::string& headline, const std::string& details, const std::string& category = "CIVILIZATION");
};
