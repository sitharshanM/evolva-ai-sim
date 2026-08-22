#pragma once
// =============================================================================
//  aeon_history.h  —  Persistent historical event database
//
//  CHANGES (Phase 1):
//   • HistoryEvent now carries a unique uint64_t event_id + string uid
//   • causes[] vector stores the structured reasons behind an event
//   • CivRelationMemory tracks the multi-dimensional relationship history
//     between two civilizations across all time (wars, trust, hatred, …)
//   • AeonHistory::record() overloads remain backward-compatible
// =============================================================================
#include "aeon_config.h"
#include "aeon_world_types.h"
#include "aeon_unique_id.h"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <algorithm>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  HistoryEvent  —  Single recorded event in the world timeline
// ─────────────────────────────────────────────────────────────────────────────
struct HistoryEvent {
    // --- Identity ---
    uint64_t    event_id = 0;           // monotonic, unique
    std::string uid;                    // e.g. "EVENT_000042"

    // --- When ---
    int         year  = 0;
    int         month = 1;

    // --- What ---
    std::string category;               // WAR, DIPLOMACY, CIVILIZATION, PERSON,
                                        // ECONOMY, TECH, CULTURE, DISASTER, BLACK_SWAN
    std::string headline;               // Short summary line
    std::string detail;                 // Extended narrative

    // --- Who ---
    int         civ_id  = -1;          // primary actor
    int         civ2_id = -1;          // secondary actor / target
    std::string actor_uid;             // e.g. "CIV_0001"
    std::string target_uid;            // e.g. "CIV_0003"

    // --- Why (structured causes for AI + chronicle) ---
    std::vector<std::string> causes;   // e.g. {"high_aggression","military_advantage"}

    // --- Effects (filled in by simulation after resolution) ---
    std::vector<std::string> effects;  // e.g. {"territory_lost:3","treasury:-200"}

    // --- Importance ---
    float significance = 0.5f;         // 0-1; used by chronicle to prioritise
};

// ─────────────────────────────────────────────────────────────────────────────
//  CivRelationMemory  —  Full relationship history between two civilizations
//  Key: encoded as (min(id_a,id_b) << 20) | max(id_a,id_b) for symmetry
// ─────────────────────────────────────────────────────────────────────────────
struct CivRelationMemory {
    int civ_a = -1;
    int civ_b = -1;

    // Accumulated historical facts
    int   wars          = 0;
    int   victories     = 0;     // civ_a won against civ_b
    int   defeats       = 0;     // civ_a lost to civ_b
    int   treaties      = 0;
    int   betrayals     = 0;     // one side broke a treaty
    int   trade_deals   = 0;
    float trade_volume  = 0.0f;  // cumulative gold value traded
    int   territory_gained = 0;  // tiles civ_a took from civ_b
    int   territory_lost   = 0;  // tiles civ_b took from civ_a
    int   royal_marriages  = 0;
    int   aid_given     = 0;
    int   aid_denied    = 0;
    int   espionage_incidents = 0;

    // 8 Core Multi-Dimensional Sentiments (−100 … +100 or 0 … 100)
    float trust           = 0.0f;     // positive = trusted, negative = distrusted
    float hatred          = 0.0f;     // positive = hated
    float fear            = 0.0f;     // positive = feared
    float respect         = 50.0f;    // positive = respected
    float gratitude       = 0.0f;     // positive = gratitude for past aid
    float suspicion       = 10.0f;    // positive = suspicion of hostile intent
    float diplomatic_debt = 0.0f;     // favors owed (-100 to +100)
    float rivalry         = 0.0f;     // competition for supremacy (0 to 100)

    // Severe long-term grievance counters (decay much slower)
    float major_betrayal_grudge = 0.0f; // persists for 40+ years
    float war_atrocity_grudge   = 0.0f;

    int   last_interaction_year = -1;

    // Update helpers
    void record_war_start() {
        ++wars;
        hatred += 25.0f;
        trust -= 35.0f;
        suspicion += 30.0f;
        rivalry += 20.0f;
        war_atrocity_grudge += 20.0f;
        clamp();
    }
    void record_victory_for_a() {
        ++victories;
        respect += 12.0f;
        fear += 10.0f;
        clamp();
    }
    void record_defeat_for_a() {
        ++defeats;
        hatred += 8.0f;
        fear -= 6.0f;
        rivalry += 10.0f;
        clamp();
    }
    void record_treaty() {
        ++treaties;
        trust += 12.0f;
        suspicion = std::max(0.0f, suspicion - 10.0f);
        clamp();
    }
    void record_betrayal() {
        ++betrayals;
        trust -= 50.0f;
        hatred += 40.0f;
        suspicion += 50.0f;
        major_betrayal_grudge += 50.0f; // Long-lasting grudge
        rivalry += 25.0f;
        clamp();
    }
    void record_trade() {
        ++trade_deals;
        trust += 4.0f;
        suspicion = std::max(0.0f, suspicion - 3.0f);
        clamp();
    }
    void record_royal_marriage() {
        ++royal_marriages;
        trust += 25.0f;
        hatred = std::max(0.0f, hatred - 15.0f);
        gratitude += 15.0f;
        clamp();
    }
    void record_aid_given() {
        ++aid_given;
        trust += 15.0f;
        gratitude += 25.0f;
        diplomatic_debt += 20.0f;
        clamp();
    }
    void record_aid_denied() {
        ++aid_denied;
        trust -= 10.0f;
        gratitude = std::max(0.0f, gratitude - 15.0f);
        clamp();
    }
    void record_espionage() {
        ++espionage_incidents;
        trust -= 25.0f;
        suspicion += 35.0f;
        hatred += 15.0f;
        clamp();
    }

    void clamp() {
        trust           = std::clamp(trust, -100.0f, 100.0f);
        hatred          = std::clamp(hatred, 0.0f, 100.0f);
        fear            = std::clamp(fear, 0.0f, 100.0f);
        respect         = std::clamp(respect, 0.0f, 100.0f);
        gratitude       = std::clamp(gratitude, 0.0f, 100.0f);
        suspicion       = std::clamp(suspicion, 0.0f, 100.0f);
        diplomatic_debt = std::clamp(diplomatic_debt, -100.0f, 100.0f);
        rivalry         = std::clamp(rivalry, 0.0f, 100.0f);
    }

    // Natural differential decay over time
    // Minor events decay in 5-10 years; major betrayals decay over 40+ years
    void decay(float years) {
        // Fast decay: minor suspicion & gratitude
        suspicion = std::max(5.0f, suspicion - years * 0.5f);
        gratitude = std::max(0.0f, gratitude - years * 0.4f);

        // Medium decay: fear & general hatred
        fear      = std::max(0.0f, fear - years * 0.3f);
        hatred    = std::max(0.0f, hatred - years * 0.3f);

        // Very slow decay: major betrayal & atrocity grudges (takes 40-50 years)
        major_betrayal_grudge = std::max(0.0f, major_betrayal_grudge - years * 0.08f);
        war_atrocity_grudge   = std::max(0.0f, war_atrocity_grudge - years * 0.08f);

        // Persistent floor for hatred & ceiling for trust if major betrayal exists
        if (major_betrayal_grudge > 5.0f) {
            hatred = std::max(hatred, major_betrayal_grudge * 0.6f);
            trust  = std::min(trust, 10.0f - major_betrayal_grudge * 0.5f);
        } else {
            // Natural slow recovery towards neutral
            if (trust < 0.0f) trust = std::min(0.0f, trust + years * 0.25f);
            else if (trust > 0.0f) trust = std::max(0.0f, trust - years * 0.10f);
        }

        clamp();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  HistoricalRecordTracker  —  Global records across thousands of years
// ─────────────────────────────────────────────────────────────────────────────
struct HistoricalRecordTracker {
    // Rulers
    std::string longest_reigning_ruler;
    int         longest_reign_years = 0;
    std::string longest_reign_civ;

    std::string most_successful_ruler;
    int         most_successful_victories = 0;

    // Realms
    std::string largest_empire_civ;
    float       largest_territory = 0.0f;

    std::string richest_realm_civ;
    float       richest_gdp = 0.0f;

    std::string most_advanced_civ;
    TechEra     highest_tech_era = TechEra::AGRICULTURE;

    std::string most_unstable_civ;
    int         most_coups_rebellions = 0;

    // Conflicts & Diplomacy
    std::string longest_war_name;
    int         longest_war_years = 0;

    std::string deadliest_war_name;
    long long   deadliest_war_casualties = 0;

    std::string longest_alliance_name;
    int         longest_alliance_years = 0;

    std::string greatest_betrayal_desc;
    int         greatest_betrayal_year = 0;

    std::string largest_rebellion_civ;
    long long   largest_rebellion_size = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
//  AeonHistory  —  Persistent 10,000-year history database
// ─────────────────────────────────────────────────────────────────────────────
class AeonHistory {
public:
    AeonHistory() = default;

    HistoricalRecordTracker stats; // Global all-time statistics

    // ── Recording ─────────────────────────────────────────────────────────────
    /// Full-featured record (Phase 1+)
    void record(int year, int month,
                const std::string& cat,
                const std::string& headline,
                const std::string& detail,
                int civ_id  = -1,
                int civ2_id = -1,
                const std::vector<std::string>& causes  = {},
                float significance = 0.5f);

    /// Convenience — backward-compatible with old call sites
    void record(int year, int month, const std::string& cat,
                const std::string& headline, const std::string& detail,
                int civ_id, int civ2_id,
                float significance) {
        record(year, month, cat, headline, detail, civ_id, civ2_id, {}, significance);
    }

    // ── Querying ──────────────────────────────────────────────────────────────
    std::vector<const HistoryEvent*> since(int from_year) const;
    std::vector<const HistoryEvent*> by_category(const std::string& cat) const;
    std::vector<const HistoryEvent*> by_civ(int civ_id) const;
    std::vector<const HistoryEvent*> by_event_id(uint64_t eid) const;
    std::string timeline_summary(int from_year, int to_year) const;
    std::string generate_epoch_summary() const;

    const std::vector<HistoryEvent>& all() const { return events_; }
    uint64_t event_count() const { return events_.size(); }

    // ── Relation memory access ────────────────────────────────────────────────
    CivRelationMemory& relation(int civ_a, int civ_b);
    const CivRelationMemory* relation_view(int civ_a, int civ_b) const;

    /// Advance time — apply natural decay to all relations
    void decay_relations(float years_passed);

    // ── ID counter (wired to IDRegistry from outside, or self-managed) ────────
    void set_id_registry(IDRegistry* reg) { id_reg_ = reg; }

private:
    std::vector<HistoryEvent>                           events_;
    std::unordered_map<uint64_t, CivRelationMemory>     relations_; // key = packed pair

    IDRegistry  local_registry_;          // used if no external registry is wired
    IDRegistry* id_reg_ = &local_registry_;

    static uint64_t relation_key(int a, int b) {
        int lo = (a < b) ? a : b;
        int hi = (a < b) ? b : a;
        return (static_cast<uint64_t>(lo) << 20) | static_cast<uint64_t>(hi);
    }
};

} // namespace Aeon
