// =============================================================================
//  aeon_history.cpp  —  Historical event database implementation (Phase 1)
// =============================================================================
#include "aeon_history.h"
#include "aeon_config.h"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  record  —  Full-featured event recording
// ─────────────────────────────────────────────────────────────────────────────
void AeonHistory::record(int year, int month,
                          const std::string& cat,
                          const std::string& headline,
                          const std::string& detail,
                          int civ_id, int civ2_id,
                          const std::vector<std::string>& causes,
                          float significance) {
    if (events_.size() >= MAX_HISTORY_EVENTS) return;

    HistoryEvent ev;
    ev.event_id    = id_reg_->alloc_event();
    ev.uid         = format_event_uid(ev.event_id);
    ev.year        = year;
    ev.month       = month;
    ev.category    = cat;
    ev.headline    = headline;
    ev.detail      = detail;
    ev.civ_id      = civ_id;
    ev.civ2_id     = civ2_id;
    ev.causes      = causes;
    ev.significance = significance;

    if (civ_id  >= 0) ev.actor_uid  = format_civ_uid(civ_id);
    if (civ2_id >= 0) ev.target_uid = format_civ_uid(civ2_id);

    events_.push_back(std::move(ev));

    // Terminal stdout log for important events
    if (cat == "WAR" || cat == "DIPLOMACY" || cat == "BLACK_SWAN" || cat == "CIVILIZATION") {
        std::cout << "[YEAR " << year << "] [" << cat << "] " << headline << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Queries
// ─────────────────────────────────────────────────────────────────────────────
std::vector<const HistoryEvent*> AeonHistory::since(int from_year) const {
    std::vector<const HistoryEvent*> out;
    for (const auto& e : events_)
        if (e.year >= from_year) out.push_back(&e);
    return out;
}

std::vector<const HistoryEvent*> AeonHistory::by_category(const std::string& cat) const {
    std::vector<const HistoryEvent*> out;
    for (const auto& e : events_)
        if (e.category == cat) out.push_back(&e);
    return out;
}

std::vector<const HistoryEvent*> AeonHistory::by_civ(int civ_id) const {
    std::vector<const HistoryEvent*> out;
    for (const auto& e : events_)
        if (e.civ_id == civ_id || e.civ2_id == civ_id) out.push_back(&e);
    return out;
}

std::vector<const HistoryEvent*> AeonHistory::by_event_id(uint64_t eid) const {
    std::vector<const HistoryEvent*> out;
    for (const auto& e : events_)
        if (e.event_id == eid) { out.push_back(&e); break; }
    return out;
}

std::string AeonHistory::timeline_summary(int from_year, int to_year) const {
    std::ostringstream ss;
    for (const auto& e : events_) {
        if (e.year >= from_year && e.year <= to_year) {
            ss << "  " << e.uid << " YEAR " << e.year
               << " [" << e.category << "] " << e.headline << "\n";
        }
    }
    return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Relation memory
// ─────────────────────────────────────────────────────────────────────────────
CivRelationMemory& AeonHistory::relation(int civ_a, int civ_b) {
    uint64_t key = relation_key(civ_a, civ_b);
    auto it = relations_.find(key);
    if (it == relations_.end()) {
        CivRelationMemory mem;
        mem.civ_a = (civ_a < civ_b) ? civ_a : civ_b;
        mem.civ_b = (civ_a < civ_b) ? civ_b : civ_a;
        relations_[key] = mem;
    }
    return relations_[key];
}

const CivRelationMemory* AeonHistory::relation_view(int civ_a, int civ_b) const {
    auto it = relations_.find(relation_key(civ_a, civ_b));
    return (it != relations_.end()) ? &it->second : nullptr;
}

void AeonHistory::decay_relations(float years_passed) {
    for (auto& kv : relations_) {
        kv.second.decay(years_passed);
    }
}

std::string AeonHistory::generate_epoch_summary() const {
    std::ostringstream ss;
    ss << "\n========================================================================\n";
    ss << "  🏆 THE HISTORICAL CHRONICLES OF AEON — ALL-TIME EPOCH RECORDS\n";
    ss << "========================================================================\n";
    
    // Dynamic Title Generation from actual statistics
    std::string epoch_title = "THE AGE OF BALANCE";
    if (!stats.largest_empire_civ.empty() && stats.largest_territory > 150.0f) {
        epoch_title = "THE GREAT " + stats.largest_empire_civ + " EXPANSION";
    } else if (!stats.richest_realm_civ.empty() && stats.richest_gdp > 3000.0f) {
        epoch_title = "THE " + stats.richest_realm_civ + " ECONOMIC BOOM";
    } else if (!stats.most_advanced_civ.empty() && (int)stats.highest_tech_era >= (int)TechEra::COMPUTING) {
        epoch_title = "THE " + stats.most_advanced_civ + " TECHNOLOGICAL ENLIGHTENMENT";
    } else if (!stats.longest_war_name.empty() && stats.longest_war_years > 20) {
        epoch_title = "THE ERA OF PERPETUAL WARFARE";
    }
    
    ss << "  🌟 EPOCH DESIGNATION: \"" << epoch_title << "\"\n";
    ss << "  ────────────────────────────────────────────────────────────────────────\n";
    if (!stats.longest_reigning_ruler.empty()) {
        ss << "  👑 Longest-Reigning Monarch: " << stats.longest_reigning_ruler 
           << " (" << stats.longest_reign_civ << ") — " << stats.longest_reign_years << " years on the throne\n";
    }
    if (!stats.most_successful_ruler.empty()) {
        ss << "  ⚔️ Greatest Conqueror:       " << stats.most_successful_ruler 
           << " — " << stats.most_successful_victories << " victorious military campaigns\n";
    }
    if (!stats.largest_empire_civ.empty()) {
        ss << "  🗺️ Largest Imperial Extent:   " << stats.largest_empire_civ 
           << " (" << int(stats.largest_territory) << " territorial provinces)\n";
    }
    if (!stats.richest_realm_civ.empty()) {
        ss << "  💰 Peak Economic Powerhouse: " << stats.richest_realm_civ 
           << " (Peak GDP: " << int(stats.richest_gdp) << ")\n";
    }
    if (!stats.most_advanced_civ.empty()) {
        ss << "  🔬 Supreme Tech Pioneer:     " << stats.most_advanced_civ 
           << " (Achieved " << tech_era_name(stats.highest_tech_era) << ")\n";
    }
    if (!stats.longest_war_name.empty()) {
        ss << "  🩸 Longest Great War:        " << stats.longest_war_name 
           << " (" << stats.longest_war_years << " continuous years)\n";
    }
    if (!stats.longest_alliance_name.empty()) {
        ss << "  🤝 Enduring Alliance:        " << stats.longest_alliance_name 
           << " (" << stats.longest_alliance_years << " years unbroken)\n";
    }
    if (!stats.greatest_betrayal_desc.empty()) {
        ss << "  🗡️ Greatest Betrayal:        " << stats.greatest_betrayal_desc 
           << " (Year " << stats.greatest_betrayal_year << ")\n";
    }
    if (!stats.largest_rebellion_civ.empty()) {
        ss << "  🔥 Largest Insurrection:     " << stats.largest_rebellion_civ 
           << " (" << stats.largest_rebellion_size << " insurgent forces)\n";
    }
    ss << "========================================================================\n\n";
    return ss.str();
}

} // namespace Aeon
