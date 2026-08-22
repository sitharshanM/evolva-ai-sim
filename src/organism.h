#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include <cstdint>
#include "dna.h"
#include "neural_net.h"
#include "citizen.h"
#include "region_ai.h"

struct World;  // forward declaration

// ─────────────────────────────────────────────────────────────────────────────
//  Organism  —  A living creature driven by its DNA-encoded neural network
// ─────────────────────────────────────────────────────────────────────────────
enum class CognitiveDrive { SURVIVAL, DEFENSE, LABOR, REPRODUCTION };

inline const char* cognitive_drive_str(CognitiveDrive d) {
    switch (d) {
        case CognitiveDrive::SURVIVAL:    return "Survival Drive 🥖";
        case CognitiveDrive::DEFENSE:     return "Patriotic Defense 🛡️";
        case CognitiveDrive::LABOR:       return "Economic Labor ⛏️";
        case CognitiveDrive::REPRODUCTION:return "Reproduction Drive 🧬";
    }
    return "Survival";
}

struct Organism {
    // ── Identity ──────────────────────────────────────────────────────────────
    uint64_t id         = 0;
    uint64_t parent_id  = 0;
    int lineage_id      = 0;  // used for species color
    int faction_id      = 0;  // nation / tribal membership
    int generation      = 0;
    bool alive          = true;
    bool is_refugee     = false; // refugee fleeing war/famine
    bool is_infected    = false; // contagion virus status

    CognitiveDrive current_drive = CognitiveDrive::SURVIVAL;
    uint64_t last_attacker_id    = 0;

    // Individual Citizen Dossier & Regional AI Alignment
    CitizenProfile citizen;
    RegionInfo     region;

    // ── Physical state ────────────────────────────────────────────────────────
    glm::vec2 pos{0.0f, 0.0f};
    float angle  = 0.0f;   // radians, 0 = pointing +Y
    float radius = 7.0f;

    // ── Biological state ──────────────────────────────────────────────────────
    float energy      = Config::INITIAL_ENERGY;
    float stamina     = 100.0f;
    float max_stamina = 100.0f;
    float hunger      = 0.0f;  // 0 = full, 1 = starving
    float age         = 0.0f;

    // Epigenetics & Resistance Triggers
    float cold_resistance = 1.0f;
    float heat_tolerance  = 1.0f;

    // ── Expressed traits (from DNA) ───────────────────────────────────────────
    float speed        = 80.0f;
    float vision_range = 100.0f;
    float metabolism   = Config::BASE_METABOLISM;
    float aggression   = 0.5f;
    float herbivore    = 0.5f;
    float mutation_rate = Config::MUTATION_RATE_BASE;

    // ── Cooldowns ────────────────────────────────────────────────────────────
    float attack_cooldown    = 0.0f;
    float reproduce_cooldown = 0.0f;
    float ranged_cooldown    = 0.0f;

    // ── Genome & Brain ────────────────────────────────────────────────────────
    DNA       dna;
    NeuralNet nn;

    // ── Visual ────────────────────────────────────────────────────────────────
    glm::vec3 color{1.0f, 1.0f, 1.0f};

    // ── Fitness tracking ──────────────────────────────────────────────────────
    float lifetime_food_eaten = 0.0f;
    int   children_count      = 0;

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    void init_from_dna();                  // Express traits + load NN weights
    void update(float dt, World& world, std::mt19937& rng);

    bool can_reproduce() const {
        return alive
            && energy >= Config::REPRODUCE_ENERGY_THRESHOLD
            && reproduce_cooldown <= 0.0f;
    }

    // Asexual budding
    Organism make_offspring(std::mt19937& rng, uint64_t next_id) const;
    // Sexual (two-parent crossover)
    Organism make_offspring_with(const Organism& partner, std::mt19937& rng, uint64_t next_id) const;

    // Custom Gene Engineering Factory
    static Organism create_custom_organism(uint64_t id, glm::vec2 pos, float speed, float vision, float aggression, float herbivore, glm::vec3 color);

private:
    std::vector<float> build_inputs(const World& world) const;
    void apply_outputs(const std::vector<float>& out, float dt, World& world, std::mt19937& rng);
};
