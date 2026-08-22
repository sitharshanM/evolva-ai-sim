#pragma once
// =============================================================================
//  aeon_unique_id.h  —  Unique, human-readable, monotonically increasing IDs
//
//  FORMAT:
//    Civilizations  →  CIV_0001 … CIV_9999
//    Characters     →  CHAR_0001 … CHAR_9999
//    Cities         →  CITY_0001
//    Regions        →  REGION_0001
//    Wars           →  WAR_0001
//    Treaties       →  TREATY_0001
//    Events         →  EVENT_000001 (6 digits — can be millions)
//    Armies         →  ARMY_0001
//
//  The integer ID is kept for fast array-indexed lookups. The string UID is
//  stored alongside for persistent, human-readable references in saves and
//  history logs.
// =============================================================================
#include <cstdint>
#include <string>
#include <iomanip>
#include <sstream>

namespace Aeon {

// ── UID formatting helpers ────────────────────────────────────────────────────
inline std::string format_civ_uid(int id) {
    std::ostringstream ss;
    ss << "CIV_" << std::setw(4) << std::setfill('0') << id;
    return ss.str();
}

inline std::string format_char_uid(int id) {
    std::ostringstream ss;
    ss << "CHAR_" << std::setw(4) << std::setfill('0') << id;
    return ss.str();
}

inline std::string format_city_uid(int id) {
    std::ostringstream ss;
    ss << "CITY_" << std::setw(4) << std::setfill('0') << id;
    return ss.str();
}

inline std::string format_region_uid(int id) {
    std::ostringstream ss;
    ss << "REGION_" << std::setw(4) << std::setfill('0') << id;
    return ss.str();
}

inline std::string format_war_uid(int id) {
    std::ostringstream ss;
    ss << "WAR_" << std::setw(4) << std::setfill('0') << id;
    return ss.str();
}

inline std::string format_treaty_uid(int id) {
    std::ostringstream ss;
    ss << "TREATY_" << std::setw(4) << std::setfill('0') << id;
    return ss.str();
}

inline std::string format_event_uid(uint64_t id) {
    std::ostringstream ss;
    ss << "EVENT_" << std::setw(6) << std::setfill('0') << id;
    return ss.str();
}

inline std::string format_army_uid(int id) {
    std::ostringstream ss;
    ss << "ARMY_" << std::setw(4) << std::setfill('0') << id;
    return ss.str();
}

// ── Centralized counter state (lives inside WorldState / AeonEngine) ──────────
// Use the IDRegistry struct to pass around monotonic counters.
struct IDRegistry {
    int      next_civ_id    = 1;
    int      next_char_id   = 1;
    int      next_city_id   = 1;
    int      next_region_id = 1;
    int      next_war_id    = 1;
    int      next_treaty_id = 1;
    uint64_t next_event_id  = 1;
    int      next_army_id   = 1;

    // Allocate each type
    int      alloc_civ()     { return next_civ_id++; }
    int      alloc_char()    { return next_char_id++; }
    int      alloc_city()    { return next_city_id++; }
    int      alloc_region()  { return next_region_id++; }
    int      alloc_war()     { return next_war_id++; }
    int      alloc_treaty()  { return next_treaty_id++; }
    uint64_t alloc_event()   { return next_event_id++; }
    int      alloc_army()    { return next_army_id++; }

    // Formatted UIDs
    std::string new_civ_uid()    { return format_civ_uid(alloc_civ()); }
    std::string new_char_uid()   { return format_char_uid(alloc_char()); }
    std::string new_city_uid()   { return format_city_uid(alloc_city()); }
    std::string new_region_uid() { return format_region_uid(alloc_region()); }
    std::string new_war_uid()    { return format_war_uid(alloc_war()); }
    std::string new_treaty_uid() { return format_treaty_uid(alloc_treaty()); }
    std::string new_event_uid()  { return format_event_uid(alloc_event()); }
    std::string new_army_uid()   { return format_army_uid(alloc_army()); }
};

} // namespace Aeon
