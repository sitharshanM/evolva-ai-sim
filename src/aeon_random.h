#pragma once
// =============================================================================
//  aeon_random.h  —  Deterministic seed-isolated pseudo-random number engine
//
//  DESIGN: Every sub-system that needs randomness should receive an AeonRandom
//  reference (or draw from a sub-stream seeded off the master seed). This lets
//  us reproduce any simulation exactly from just `simulation_seed`.
//
//  The internal engine is std::mt19937_64 whose state can be serialized to a
//  64-bit vector via serialize() / deserialize() for save-load support.
// =============================================================================
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>
#include <random>
#include <limits>
#include <algorithm>
#include <cmath>

namespace Aeon {

class AeonRandom {
public:
    // ── Construction ─────────────────────────────────────────────────────────
    explicit AeonRandom(uint64_t seed = 928374ULL) { reseed(seed); }

    void reseed(uint64_t seed) {
        seed_ = seed;
        rng_.seed(seed);
        advance_count_ = 0;
    }

    uint64_t current_seed()    const { return seed_; }
    uint64_t advance_count()   const { return advance_count_; }

    // ── Core generators ──────────────────────────────────────────────────────
    /// Uniform float in [0, 1)
    float uniform() {
        ++advance_count_;
        return dist01_(rng_);
    }
    /// Uniform float in [lo, hi)
    float uniform(float lo, float hi) { return lo + (hi - lo) * uniform(); }

    /// Uniform int in [lo, hi]  (inclusive on both ends)
    int uniform_int(int lo, int hi) {
        if (lo >= hi) return lo;
        ++advance_count_;
        return lo + static_cast<int>(rng_() % static_cast<uint64_t>(hi - lo + 1));
    }

    /// Raw 64-bit random value
    uint64_t raw() { ++advance_count_; return rng_(); }

    /// True with given probability p ∈ [0,1]
    bool chance(float p) { return uniform() < p; }

    /// Normal / Gaussian sample — Box-Muller transform
    float normal(float mean = 0.0f, float stddev = 1.0f) {
        float u1 = std::max(1e-7f, uniform());
        float u2 = uniform();
        float z  = std::sqrt(-2.0f * std::log(u1)) * std::cos(6.28318530718f * u2);
        return mean + stddev * z;
    }

    /// Sample from [0..n-1] weighted by weights array
    int weighted_sample(const std::vector<float>& weights) {
        float total = 0.0f;
        for (float w : weights) total += w;
        float r   = uniform() * total;
        float acc = 0.0f;
        for (int i = 0; i < (int)weights.size(); ++i) {
            acc += weights[i];
            if (r <= acc) return i;
        }
        return (int)weights.size() - 1;
    }

    // ── State serialization for deterministic save/load ───────────────────────
    /// Serialize seed + advance_count → compact string
    std::string serialize() const {
        std::ostringstream oss;
        oss << seed_ << ' ' << advance_count_;
        return oss.str();
    }

    /// Restore from serialize() string — re-seeds and fast-forwards
    void deserialize(const std::string& s) {
        std::istringstream iss(s);
        uint64_t stored_seed = 928374ULL;
        uint64_t stored_adv  = 0ULL;
        iss >> stored_seed >> stored_adv;
        reseed(stored_seed);
        // Fast-forward to the same position
        rng_.discard(stored_adv);
        advance_count_ = stored_adv;
    }

    // ── Sub-stream seeding ────────────────────────────────────────────────────
    /// Create a child RNG deterministically from this seed + stream_id
    AeonRandom sub_stream(uint64_t stream_id) const {
        // splitmix64 mix
        uint64_t x = seed_ ^ (stream_id * 0x9E3779B97F4A7C15ULL);
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
        x ^= x >> 31;
        return AeonRandom(x);
    }

private:
    uint64_t  seed_          = 928374ULL;
    uint64_t  advance_count_ = 0;
    std::mt19937_64 rng_;
    std::uniform_real_distribution<float> dist01_{0.0f, 1.0f};
};

} // namespace Aeon
