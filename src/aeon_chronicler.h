#pragma once
#include "aeon_history.h"
#include "aeon_world_types.h"
#include <string>
#include <vector>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  AeonChronicler  —  Asynchronously synthesizes annual history events into
//  narrative chapters written by an LLM historian, and detects macro historical eras.
// ─────────────────────────────────────────────────────────────────────────────
class AeonEngine;

class AeonChronicler {
public:
    AeonChronicler() = default;

    // Macro Era Detection & Evolution
    void update_eras(AeonEngine& engine, int current_year);
    const HistoricalEra& get_current_era() const { return current_era_; }
    const std::vector<HistoricalEra>& get_recorded_eras() const { return recorded_eras_; }

    // Call annually to generate the daily world newspaper
    std::string generate_aeon_daily(const AeonEngine& engine, int current_year);

    // Call periodically (e.g. every 25 years) to generate a new chapter
    void update_chronicle(const AeonHistory& history, int current_year);

    // Get the full book of history
    std::string get_chronicle() const;

private:
    std::vector<std::string> chapters_;
    int last_summarized_year_ = 0;

    // Macro historical eras
    HistoricalEra current_era_;
    std::vector<HistoricalEra> recorded_eras_;
    bool era_initialized_ = false;
};

} // namespace Aeon
