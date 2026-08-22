#include "neural_net.h"
#include <cassert>
#include <algorithm>

// ── Constructor ───────────────────────────────────────────────────────────────
NeuralNet::NeuralNet() {
    w1.assign(IN * H, 0.0f);
    b1.assign(H, 0.0f);
    wz.assign(H * H, 0.0f);
    wh.assign(H * H, 0.0f);
    w2.assign(H * H, 0.0f);
    b2.assign(H, 0.0f);
    w3.assign(H * OUT, 0.0f);
    b3.assign(OUT, 0.0f);
    h.assign(H, 0.0f);
}

// ── Load from DNA genes ───────────────────────────────────────────────────────
void NeuralNet::load_from_genes(const float* weights, int count) {
    // Expected: count == Config::NN_WEIGHT_COUNT == 700
    int idx = 0;
    auto copy_n = [&](std::vector<float>& dst, int n) {
        for (int i = 0; i < n && idx < count; ++i, ++idx)
            dst[i] = weights[idx];
    };
    copy_n(w1, IN * H);
    copy_n(b1, H);
    copy_n(wz, H * H);
    copy_n(wh, H * H);
    copy_n(w2, H * H);
    copy_n(b2, H);
    copy_n(w3, H * OUT);
    copy_n(b3, OUT);
}

// ── Forward Pass ──────────────────────────────────────────────────────────────
std::vector<float> NeuralNet::forward(const std::vector<float>& input) {
    // ── Layer 1: input (IN) → hidden1 (H) ────────────────────────────────────
    std::vector<float> h1(H, 0.0f);
    for (int j = 0; j < H; ++j) {
        float sum = b1[j];
        for (int i = 0; i < IN; ++i)
            sum += w1[i * H + j] * input[i];
        h1[j] = tanh_f(sum);
    }

    // ── GRU Memory Update ─────────────────────────────────────────────────────
    // Update gate z = sigmoid(Wz·h_prev + h1)
    // Candidate   h̃ = tanh(Wh·h_prev + h1)
    // New hidden  h = (1-z)·h + z·h̃
    std::vector<float> z_gate(H), h_cand(H);
    for (int j = 0; j < H; ++j) {
        float sum_z = h1[j];
        float sum_h = h1[j];
        for (int k = 0; k < H; ++k) {
            sum_z += wz[k * H + j] * h[k];
            sum_h += wh[k * H + j] * h[k];
        }
        z_gate[j] = sigmoid(sum_z);
        h_cand[j] = tanh_f(sum_h);
    }
    for (int j = 0; j < H; ++j)
        h[j] = (1.0f - z_gate[j]) * h[j] + z_gate[j] * h_cand[j];

    // ── Layer 2: h (H) → hidden2 (H) ──────────────────────────────────────────
    std::vector<float> h2(H, 0.0f);
    for (int j = 0; j < H; ++j) {
        float sum = b2[j];
        for (int k = 0; k < H; ++k)
            sum += w2[k * H + j] * h[k];
        h2[j] = tanh_f(sum);
    }

    // ── Output: h2 (H) → out (OUT), tanh activation ──────────────────────────
    std::vector<float> out(OUT, 0.0f);
    for (int j = 0; j < OUT; ++j) {
        float sum = b3[j];
        for (int k = 0; k < H; ++k)
            sum += w3[k * OUT + j] * h2[k];
        out[j] = tanh_f(sum);
    }

    return out;
}
