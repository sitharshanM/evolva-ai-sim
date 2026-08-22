#pragma once
#include <vector>
#include <random>
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
//  DNA  —  Flat genome encoding organism traits + neural-network weights
//
//  Layout:  [0..6] trait genes  |  [7..706] NN weights
//
//  Trait genes (all stored raw; express_traits() maps them to usable values):
//    [0] speed_gene          → lerp(MOVE_SPEED_MIN, MOVE_SPEED_MAX, t)
//    [1] size_gene           → lerp(ORG_MIN_RADIUS, ORG_MAX_RADIUS, t)
//    [2] vision_range_gene   → lerp(VISION_MIN, VISION_MAX, t)
//    [3] metabolism_gene     → lerp(1.0, 5.0, t)
//    [4] aggression_gene     → [0, 1]
//    [5] herbivore_gene      → [0, 1]   (1 = pure herbivore)
//    [6] mutation_rate_gene  → lerp(0.005, MUTATION_RATE_MAX, t)
// ─────────────────────────────────────────────────────────────────────────────
struct DNA {
    std::vector<float> genes; // size = Config::DNA_SIZE (707)

    // ── Expressed trait values ────────────────────────────────────────────────
    float speed           = 80.0f;
    float radius          = 7.0f;
    float vision_range    = 100.0f;
    float metabolism      = Config::BASE_METABOLISM;
    float aggression      = 0.5f;
    float herbivore       = 0.5f;
    float mutation_rate   = Config::MUTATION_RATE_BASE;

    // ── Factory ───────────────────────────────────────────────────────────────
    static DNA random(std::mt19937& rng);

    // ── Genetics ──────────────────────────────────────────────────────────────
    static DNA crossover(const DNA& a, const DNA& b, std::mt19937& rng);
    DNA mutate(std::mt19937& rng) const;

    // Maps raw gene values to usable trait values
    void express_traits();

    // ── Helpers ───────────────────────────────────────────────────────────────
    int   nn_weight_count() const { return Config::NN_WEIGHT_COUNT; }
    const float* nn_weights() const { return genes.data() + Config::TRAIT_GENE_COUNT; }

    // Distance between two genomes (for speciation)
    float genetic_distance(const DNA& other) const;

private:
    static float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }
    static float lerp(float a, float b, float t) { return a + (b - a) * t; }
};
