#pragma once
#include "aeon_history.h"
#include <string>
#include <vector>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  AeonChronicler  —  Asynchronously synthesizes annual history events into
//  narrative chapters written by an LLM historian.
// ─────────────────────────────────────────────────────────────────────────────
class AeonEngine;

class AeonChronicler {
public:
    AeonChronicler() = default;

    // Call annually to generate the daily world newspaper
    std::string generate_aeon_daily(const AeonEngine& engine, int current_year);

    // Call periodically (e.g. every 25 years) to generate a new chapter
    void update_chronicle(const AeonHistory& history, int current_year);

    // Get the full book of history
    std::string get_chronicle() const;

private:
    std::vector<std::string> chapters_;
    int last_summarized_year_ = 0;
};

} // namespace Aeon

