#pragma once
#include "world.h"
#include "renderer.h"
#include "ollama_god.h"
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  SimApp  —  Top-level application: owns World, Renderer, OllamaGod
//             Runs the main loop, handles input, orchestrates all systems
// ─────────────────────────────────────────────────────────────────────────────
class SimApp {
public:
    SimApp() = default;
    ~SimApp();

    bool init(bool headless = false);
    void run();
    void run_headless();
    void shutdown();

private:
    World      world_;
    Renderer   renderer_;
    OllamaGod  ollama_;

    bool   paused_      = false;
    int    speed_mult_  = 1;      // ticks per render frame
    int    selected_id_ = -1;     // currently selected organism ID
    float  fps_         = 0.0f;
    float  dt_acc_      = 0.0f;

    // Ollama state
    float  god_timer_          = 0.0f;  // countdown to next decree request
    int    current_leader_idx_ = 0;     // round-robin dynasty leader index
    std::string narrator_text_;
    float  narrator_timer_     = 0.0f;
    std::string ollama_status_;

    // Input state for camera pan
    bool   rmb_down_   = false;
    double rmb_last_x_ = 0, rmb_last_y_ = 0;

    void handle_input(float dt);
    void handle_ollama(float dt);
};
