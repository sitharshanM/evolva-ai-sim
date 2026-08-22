#pragma once
#include <deque>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
//  Stats  —  Tracks simulation metrics over time; consumed by UI & Ollama
// ─────────────────────────────────────────────────────────────────────────────
struct Stats {
    // ── Snapshot (updated each world tick) ───────────────────────────────────
    int   population      = 0;
    int   food_count      = 0;
    int   generation_max  = 0;
    float avg_energy      = 0.0f;
    float avg_speed       = 0.0f;
    float avg_vision      = 0.0f;
    float avg_aggression  = 0.0f;
    float avg_herbivore   = 0.0f;
    float avg_metabolism  = 0.0f;
    float herbivore_ratio = 0.0f;  // 0–1
    float carnivore_ratio = 0.0f;  // 0–1
    float sim_time        = 0.0f;
    long long total_births  = 0;
    long long total_deaths  = 0;
    int   lineage_count   = 0;     // distinct species

    // ── History (for ImGui plots) ─────────────────────────────────────────────
    static constexpr int HISTORY = 512;

    std::deque<float> history_population;
    std::deque<float> history_herbivore_ratio;
    std::deque<float> history_avg_speed;
    std::deque<float> history_avg_aggression;

    void record() {
        auto push = [](std::deque<float>& d, float v) {
            if (static_cast<int>(d.size()) >= HISTORY) d.pop_front();
            d.push_back(v);
        };
        push(history_population,       static_cast<float>(population));
        push(history_herbivore_ratio,  herbivore_ratio);
        push(history_avg_speed,        avg_speed);
        push(history_avg_aggression,   avg_aggression);
    }

    // Copy deque → contiguous float array for ImGui::PlotLines
    std::vector<float> pop_array()  const { return {history_population.begin(), history_population.end()}; }
    std::vector<float> herb_array() const { return {history_herbivore_ratio.begin(), history_herbivore_ratio.end()}; }
    std::vector<float> spd_array()  const { return {history_avg_speed.begin(), history_avg_speed.end()}; }
    std::vector<float> agg_array()  const { return {history_avg_aggression.begin(), history_avg_aggression.end()}; }
};
