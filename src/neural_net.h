#pragma once
#include <vector>
#include <cmath>
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
//  NeuralNet  —  Feedforward network with a simplified GRU memory cell
//
//  Architecture:
//    Input (16) ──► Layer1 (→H, tanh)
//                      │
//                   GRU Memory: z = σ(Wz·h + h1)
//                               h̃ = tanh(Wh·h + h1)
//                               h = (1-z)·h + z·h̃
//                      │
//                   Layer2 (H→H, tanh)
//                      │
//                   Output (H→4, tanh)
// ─────────────────────────────────────────────────────────────────────────────
struct NeuralNet {
    static constexpr int IN  = Config::NN_INPUT_SIZE;
    static constexpr int H   = Config::NN_HIDDEN_SIZE;
    static constexpr int OUT = Config::NN_OUTPUT_SIZE;

    // Layer 1: IN → H
    std::vector<float> w1; // [IN * H]
    std::vector<float> b1; // [H]

    // GRU gates (H → H, no separate biases — uses layer1 output as bias source)
    std::vector<float> wz;  // update gate  [H*H]
    std::vector<float> wh;  // candidate    [H*H]

    // Layer 2: H → H
    std::vector<float> w2; // [H*H]
    std::vector<float> b2; // [H]

    // Output: H → OUT
    std::vector<float> w3; // [H*OUT]
    std::vector<float> b3; // [OUT]

    // Persistent GRU hidden state (memory across frames)
    std::vector<float> h;

    NeuralNet();

    // Load weights from a flat array (section of DNA)
    void load_from_genes(const float* weights, int count);

    // Compute one forward pass; updates h in-place (memory)
    std::vector<float> forward(const std::vector<float>& input);

    void reset_memory() { std::fill(h.begin(), h.end(), 0.0f); }

    static float tanh_f(float x) { return std::tanh(x); }
    static float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }
};
