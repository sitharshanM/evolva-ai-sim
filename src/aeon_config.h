#pragma once
#include <cstdint>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  AEON — Emergent AI Civilization Simulator
//  Master Configuration Constants
// ─────────────────────────────────────────────────────────────────────────────

namespace Aeon {

// Time: 1 real second = 1 simulated month (12 real seconds = 1 simulated year)
constexpr float SECONDS_PER_YEAR     = 12.0f;
constexpr float MONTHS_PER_YEAR      = 12.0f;
constexpr float DAYS_PER_MONTH       = 30.0f;
constexpr float STARTING_SIM_YEAR    = 2026.0f;
constexpr float MAX_SIMULATION_YEARS = 10000.0f;

// World dimensions (Earth-Scale tile grid)
constexpr int MAP_WIDTH  = 360;
constexpr int MAP_HEIGHT = 180;

// Civilizations
constexpr int NUM_INITIAL_CIVS = 5;

// Population
constexpr int MAX_LIFESPAN       = 100; // years
constexpr int MAX_IMPORTANT_NPCS = 200; // tracked characters per civ

// Economy
constexpr int NUM_RESOURCES = 12;

// History
constexpr int MAX_HISTORY_EVENTS = 50000;

// Simulation speeds
constexpr float SPEED_NORMAL  = 1.0f;
constexpr float SPEED_FAST    = 10.0f;
constexpr float SPEED_TURBO   = 100.0f;

// Seasons
constexpr const char* SEASONS[4] = { "SPRING", "SUMMER", "AUTUMN", "WINTER" };

// Universe / seed
using Seed = uint64_t;

} // namespace Aeon
