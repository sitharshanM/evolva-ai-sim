#pragma once
#include <vector>
#include <string>
#include <random>
#include <unordered_set>
#include <glm/glm.hpp>
#include "organism.h"
#include "spatial_hash.h"
#include "stats.h"
#include "ollama_god.h"   // for DecreeParams

// ── Food particle ─────────────────────────────────────────────────────────────
struct Food {
    glm::vec2 pos{0.0f, 0.0f};
    float energy = Config::FOOD_ENERGY;
    float radius = Config::FOOD_RADIUS;
    bool  alive  = true;
};

// ── Visual decree effect (shown for a few seconds on screen) ──────────────────
struct DecreeVisual {
    glm::vec2 pos{0.0f, 0.0f};
    float radius    = 0.0f;
    float lifetime  = 3.0f;   // seconds remaining
    float max_life  = 3.0f;
    glm::vec3 color{1.0f, 0.8f, 0.2f};
};

// ── Log entry ─────────────────────────────────────────────────────────────────
struct DecreeLog {
    std::string decree_name;
    std::string description;
    float sim_time = 0.0f;
};



#include "faction.h"
#include "diplomacy.h"
#include "chronicle.h"
#include "caravan.h"
#include "god_powers.h"
#include "sound_engine.h"

// ── Global Political Decree (from Ollama Political Strategist) ─────────────────
struct PoliticalAction {
    std::string action_type;     // DECLARE_WAR, FORM_ALLIANCE, PEACE_TREATY, CIVIL_WAR, PROPAGANDA
    int         faction_a = -1;
    int         faction_b = -1;
    std::string treaty_name;
    std::string declaration;
    float       modifier_val = 1.0f;
};

struct WarBanner {
    glm::vec2 pos{0.0f, 0.0f};
    int       attacking_faction = -1;
    int       target_faction    = -1;
    float     lifetime          = 45.0f;
    float     max_life          = 45.0f;
};

struct ResourceNode {
    glm::vec2 pos{0.0f, 0.0f};
    int       type   = 0; // 0=Wood, 1=Iron, 2=Gold
    float     amount = 200.0f;
};

struct FarmPlot {
    glm::vec2 pos{0.0f, 0.0f};
    float     growth     = 0.0f; // 0 to 1.0
    int       faction_id = -1;
};

struct Projectile {
    glm::vec2 pos{0.0f, 0.0f};
    glm::vec2 vel{0.0f, 0.0f};
    int       faction_id = -1;
    float     damage     = 15.0f;
    float     lifetime   = 2.5f;
};

struct Shrine {
    glm::vec2 pos{0.0f, 0.0f};
    int       faction_id = -1;
    float     faith      = 50.0f;
};

struct SiegeEngine {
    glm::vec2 pos{0.0f, 0.0f};
    int       faction_id = -1;
    float     health     = 200.0f;
    float     cooldown   = 0.0f;
};

struct DisasterEvent {
    glm::vec2 pos{0.0f, 0.0f};
    int       type     = 0; // 0=Volcano, 1=Earthquake
    float     radius   = 150.0f;
    float     lifetime = 30.0f;
};

#include "world_layer.h"
#include "kingdom.h"
#include "ruler_ai.h"
#include "economy_engine.h"
#include "history_engine.h"

// ─────────────────────────────────────────────────────────────────────────────
//  World  —  The simulation container
// ─────────────────────────────────────────────────────────────────────────────
struct World {
    float width  = Config::WORLD_WIDTH;
    float height = Config::WORLD_HEIGHT;

    std::vector<Organism>     organisms;
    std::vector<Food>         food;
    std::vector<DecreeVisual> decree_visuals;
    std::vector<DecreeLog>    decree_log;
    std::vector<WarBanner>    war_banners;
    std::vector<ResourceNode> resource_nodes;
    std::vector<FarmPlot>     farm_plots;
    std::vector<Projectile>   projectiles;
    std::vector<Shrine>       shrines;
    std::vector<SiegeEngine>  siege_engines;
    std::vector<DisasterEvent> disasters;
    std::vector<uint8_t>      road_heat; // 100x100 grid heat for paved roads

    // Full 35-Layer World Engine Sub-Modules
    WorldLayer               world_layer;
    EconomyEngine            economy_engine;
    HistoryEngine            history_engine;
    std::vector<KingdomDossier> kingdoms;
    std::vector<RulerAI>     rulers;

    // Factions, Diplomacy, Chronicle & God Powers
    std::vector<Faction>    factions;
    DiplomaticMatrix        diplomacy;
    Chronicle               chronicle;
    GodPowerManager         god_powers;
    std::vector<MerchantCaravan> caravans;

    // Pending offspring (added end-of-frame to avoid iterator invalidation)
    std::vector<Organism> pending_births;

    SpatialHash org_hash  {Config::CELL_SIZE};
    SpatialHash food_hash {Config::CELL_SIZE};

    std::mt19937 rng;
    uint64_t next_id   = 1;
    float sim_time     = 0.0f;
    float food_accum   = 0.0f;

    // Global modifiers (set by LLM decrees)
    float food_spawn_mult       = 1.0f;
    float global_metabolism_mul = 1.0f;
    float global_mutation_boost = 1.0f;
    bool  plague_active         = false;
    float plague_timer          = 0.0f;
    float plague_strength       = 0.0f;

    // Statistics
    Stats stats;

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    World();
    void init();
    void tick(float dt);

    // ── Entity management ─────────────────────────────────────────────────────
    void spawn_food(glm::vec2 pos, float energy = Config::FOOD_ENERGY);
    void spawn_random_food(int count);
    Organism& spawn_organism(glm::vec2 pos = {-1.0f, -1.0f}, int lineage = -1);

    // ── Spatial helpers ───────────────────────────────────────────────────────
    void rebuild_hashes();
    glm::vec2 wrap(glm::vec2 p) const;
    float torus_dist(glm::vec2 a, glm::vec2 b) const;
    glm::vec2 torus_dir(glm::vec2 from, glm::vec2 to) const;
    glm::vec2 random_pos();

    // Find nearest food within radius; returns index or -1
    int nearest_food(glm::vec2 pos, float range) const;
    // Returns indices of all organisms within range of pos
    std::vector<int> nearby_organisms(glm::vec2 pos, float range) const;

    // ── LLM Decree application ────────────────────────────────────────────────
    void apply_decree(const std::string& type, const DecreeParams& p, const std::string& desc);
    void apply_political_action(const PoliticalAction& action);

    // ── Faction helpers ───────────────────────────────────────────────────────
    void update_factions();
    Faction* get_faction(int id);

    // ── God Powers & Caravans ──────────────────────────────────────────────────
    void execute_smite(glm::vec2 pos, float radius = 120.0f);
    void spawn_stone_wall(glm::vec2 pos, float radius = 45.0f);
    void paint_genesis_food(glm::vec2 pos, int count = 25);
    void update_caravans(float dt);
    void spawn_merchant_caravan(int home_fac, int dest_fac);

    const std::unordered_set<int>& active_lineages() const { return active_lineages_; }

private:
    void update_stats();
    void flush_births();
    void process_toxic_zones(float dt);
    void process_plague(float dt);

    // Lineage counters
    int next_lineage_id = 1;
    std::unordered_set<int> active_lineages_;
};
