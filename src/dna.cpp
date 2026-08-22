#include "dna.h"
#include <cmath>
#include <algorithm>
#include <numeric>

// ── Helpers ──────────────────────────────────────────────────────────────────
static float sig(float x) { return 1.0f / (1.0f + std::exp(-x)); }
static float lp(float a, float b, float t) { return a + (b - a) * std::clamp(t, 0.0f, 1.0f); }

// ── Factory ──────────────────────────────────────────────────────────────────
DNA DNA::random(std::mt19937& rng) {
    DNA d;
    d.genes.resize(Config::DNA_SIZE);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (auto& g : d.genes) g = dist(rng);
    d.express_traits();
    return d;
}

// ── Genetics ─────────────────────────────────────────────────────────────────
DNA DNA::crossover(const DNA& a, const DNA& b, std::mt19937& rng) {
    DNA child;
    child.genes.resize(Config::DNA_SIZE);
    std::uniform_int_distribution<int> coin(0, 1);
    // Uniform crossover: each gene independently chosen from either parent
    for (int i = 0; i < Config::DNA_SIZE; ++i)
        child.genes[i] = coin(rng) ? a.genes[i] : b.genes[i];
    child.express_traits();
    return child;
}

DNA DNA::mutate(std::mt19937& rng) const {
    DNA child = *this;
    std::normal_distribution<float> noise(0.0f, Config::MUTATION_STRENGTH);
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    float rate = std::clamp(child.mutation_rate, 0.005f, Config::MUTATION_RATE_MAX);
    for (auto& g : child.genes) {
        if (chance(rng) < rate)
            g = std::clamp(g + noise(rng), -3.5f, 3.5f);
    }
    child.express_traits();
    return child;
}

// ── Trait Expression ─────────────────────────────────────────────────────────
void DNA::express_traits() {
    if (genes.size() < Config::TRAIT_GENE_COUNT) return;
    // Map raw gene (any float) through sigmoid → [0,1] then scale
    speed         = lp(Config::MOVE_SPEED_MIN, Config::MOVE_SPEED_MAX, sig(genes[0]));
    radius        = lp(Config::ORG_MIN_RADIUS, Config::ORG_MAX_RADIUS, sig(genes[1]));
    vision_range  = lp(Config::VISION_MIN, Config::VISION_MAX, sig(genes[2]));
    metabolism    = lp(1.0f, 5.5f, sig(genes[3]));
    aggression    = sig(genes[4]);
    herbivore     = sig(genes[5]);
    mutation_rate = lp(0.005f, Config::MUTATION_RATE_MAX, sig(genes[6]));
}

// ── Genetic Distance ─────────────────────────────────────────────────────────
float DNA::genetic_distance(const DNA& other) const {
    // Only compare trait genes for speciation (cheaper than full NN diff)
    float dist = 0.0f;
    int n = std::min(Config::TRAIT_GENE_COUNT, (int)std::min(genes.size(), other.genes.size()));
    for (int i = 0; i < n; ++i) {
        float d = genes[i] - other.genes[i];
        dist += d * d;
    }
    return std::sqrt(dist / std::max(n, 1));
}
