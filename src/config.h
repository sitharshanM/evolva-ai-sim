#pragma once
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
//  Digital Life Simulator — Central Configuration
// ─────────────────────────────────────────────────────────────────────────────
namespace Config {

// ── World
// ─────────────────────────────────────────────────────────────────────
inline constexpr float WORLD_WIDTH = 3500.0f;
inline constexpr float WORLD_HEIGHT = 3500.0f;

// ── Population
// ────────────────────────────────────────────────────────────────
inline constexpr int INITIAL_ORGANISMS = 4000;
inline constexpr int MAX_ORGANISMS = 8000;
inline constexpr int MIN_ORGANISMS = 600; // respawn floor

// ── Food
// ──────────────────────────────────────────────────────────────────────
inline constexpr int INITIAL_FOOD = 6000;
inline constexpr int MAX_FOOD = 7000;
inline constexpr float FOOD_SPAWN_RATE = 40.0f; // clean food spawn rate
inline constexpr float FOOD_ENERGY = 35.0f;
inline constexpr float FOOD_RADIUS = 4.5f;
inline constexpr float FOOD_CLUSTER_PROB = 0.45f; // high food clustering

// ── Organism Biology
// ──────────────────────────────────────────────────────────
inline constexpr float ORG_MIN_RADIUS = 4.0f;
inline constexpr float ORG_MAX_RADIUS = 16.0f;
inline constexpr float MOVE_SPEED_MIN = 40.0f;
inline constexpr float MOVE_SPEED_MAX = 220.0f;
inline constexpr float TURN_SPEED_MAX = 5.5f;  // radians / second
inline constexpr float BASE_METABOLISM = 1.2f; // low energy drain
inline constexpr float MAX_ENERGY = 120.0f;
inline constexpr float INITIAL_ENERGY = 80.0f;

inline constexpr float REPRODUCE_ENERGY_THRESHOLD = 42.0f;
inline constexpr float REPRODUCE_ENERGY_COST = 16.0f;
inline constexpr float REPRODUCE_COOLDOWN = 3.5f; // rapid reproduction

inline constexpr float ATTACK_DAMAGE = 22.0f;
inline constexpr float ATTACK_RANGE = 22.0f;
inline constexpr float ATTACK_COOLDOWN = 0.8f; // seconds
inline constexpr float ATTACK_ENERGY_COST = 1.5f;

inline constexpr float MAX_AGE = 500.0f;      // seconds of sim time
inline constexpr float CORPSE_ENERGY = 0.70f; // 70% energy left as food

inline constexpr float VISION_MIN = 50.0f;
inline constexpr float VISION_MAX = 300.0f;

// ── DNA / Neural Network
// ──────────────────────────────────────────────────────
inline constexpr int TRAIT_GENE_COUNT = 7;
inline constexpr int NN_INPUT_SIZE = 16;
inline constexpr int NN_HIDDEN_SIZE = 12;
inline constexpr int NN_OUTPUT_SIZE = 4;

// Weight counts:
inline constexpr int NN_W1 =
    NN_INPUT_SIZE * NN_HIDDEN_SIZE + NN_HIDDEN_SIZE; // 204
inline constexpr int NN_WZ =
    NN_HIDDEN_SIZE * NN_HIDDEN_SIZE; // 144 (GRU update gate)
inline constexpr int NN_WH =
    NN_HIDDEN_SIZE * NN_HIDDEN_SIZE; // 144 (GRU candidate)
inline constexpr int NN_W2 =
    NN_HIDDEN_SIZE * NN_HIDDEN_SIZE + NN_HIDDEN_SIZE; // 156
inline constexpr int NN_W3 =
    NN_HIDDEN_SIZE * NN_OUTPUT_SIZE + NN_OUTPUT_SIZE; // 52
inline constexpr int NN_WEIGHT_COUNT =
    NN_W1 + NN_WZ + NN_WH + NN_W2 + NN_W3;                          // 700
inline constexpr int DNA_SIZE = TRAIT_GENE_COUNT + NN_WEIGHT_COUNT; // 707

inline constexpr float MUTATION_RATE_BASE = 0.04f;
inline constexpr float MUTATION_STRENGTH = 0.25f;
inline constexpr float MUTATION_RATE_MAX = 0.20f;

// ── Spatial Hashing
// ────────────────────────────────────────────────────────────
inline constexpr float CELL_SIZE = 120.0f; // should be >= max vision

// ── Rendering ────────────────────────────────────────────────────────────────
inline constexpr int WINDOW_WIDTH = 1600;
inline constexpr int WINDOW_HEIGHT = 900;
inline constexpr int MSAA_SAMPLES = 4;

// ── Simulation Speed ─────────────────────────────────────────────────────────
inline constexpr int MAX_TICKS_PER_FRAME = 20; // for fast-forward

// ── Factions & Politics
// ───────────────────────────────────────────────────────
inline constexpr int MAX_FACTIONS = 8;
inline constexpr float DIPLOMACY_TICK_INTERVAL = 15.0f; // LLM political cycle
inline constexpr float WAR_FERVOR_BUFF =
    1.4f; // combat speed & damage in enemy territory
inline constexpr float TRIBUTE_RATE =
    5.0f; // energy transferred per sec from vassal

// ── Ollama
// ────────────────────────────────────────────────────────────────────
inline constexpr float GOD_DECREE_INTERVAL = 15.0f; // seconds
inline constexpr const char *OLLAMA_HOST = "127.0.0.1";
inline constexpr int OLLAMA_PORT = 11434;
inline constexpr const char *OLLAMA_MODEL = "llama3.1";

} // namespace Config
