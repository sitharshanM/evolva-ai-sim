#pragma once
#include "aeon_engine.h"
#include <string>
#include <vector>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  AeonCLI  —  Terminal command-line interface for AEON
// ─────────────────────────────────────────────────────────────────────────────
class AeonCLI {
public:
    explicit AeonCLI(AeonEngine& engine);

    // Draw the full terminal dashboard (map + status + events + prompt)
    void draw();

    // Handle a single CLI command string; returns false to quit
    bool handle_command(const std::string& cmd);

    // Whether to quit
    bool should_quit = false;

private:
    AeonEngine& engine_;

    // Map view state
    int pan_x_ = 0, pan_y_ = 0;
    int view_w_ = 80, view_h_ = 24;

    void draw_header();
    void draw_map();
    void draw_events();
    void draw_prompt();
    void print_help();
    void print_countries();
    void print_diplomacy();
    void print_history(int from_year, int to_year);
    void print_people();
};

} // namespace Aeon
