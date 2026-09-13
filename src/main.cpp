#include "aeon_engine.h"
#include "aeon_cli.h"
#include "aeon_gui.h"
#include "aeon_test_government.h"
#include "../tests/action_execution_test.h"
#include "../tests/runtime_test.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    // Ensure clean UTF-8 console output on Windows to prevent corrupted characters
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    uint64_t seed = 928374ULL;
    bool force_cli = false;
    int run_years = 0;
    std::string export_replay, replay_path;
    bool parallel_advisors = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--export-replay") == 0 && i + 1 < argc) export_replay = argv[++i];
        if (std::strcmp(argv[i], "--replay") == 0 && i + 1 < argc) replay_path = argv[++i];
        if (std::strcmp(argv[i], "--parallel-advisors") == 0) parallel_advisors = true;
        if (std::strcmp(argv[i], "--test-runtime") == 0) {
            return run_runtime_tests() ? 0 : 1;
        }
        if (std::strcmp(argv[i], "--test-actions") == 0) {
            return run_action_execution_tests() ? 0 : 1;
        }
        if (std::strcmp(argv[i], "--test-government") == 0 || std::strcmp(argv[i], "--test") == 0) {
            bool ok = Aeon::GovernmentTestSuite::run_all_tests();
            return ok ? 0 : 1;
        }
        if (std::strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            seed = std::stoull(argv[i + 1]);
        }
        if ((std::strcmp(argv[i], "--years") == 0 || std::strcmp(argv[i], "--benchmark") == 0) && i + 1 < argc) {
            run_years = std::stoi(argv[i + 1]);
            force_cli = true;
        }
        if (std::strcmp(argv[i], "--cli") == 0 || std::strcmp(argv[i], "--headless") == 0) {
            force_cli = true;
        }
    }

    Aeon::AeonEngine engine;
    engine.init(seed);
    if (!replay_path.empty()) {
        std::string error;
        if (!Aeon::SimulationRuntime::replay_archive(engine, replay_path, error)) {
            std::cerr << error << '\n'; return 1;
        }
        if (run_years == 0) { std::cout << "Replay verified at year " << engine.year << '\n'; return 0; }
    }
    engine.runtime.parallel_advisors = parallel_advisors;

    if (run_years > 0) {
        std::cout << "\n========================================================================\n";
        std::cout << "  🚀 RUNNING FAST BATCH SIMULATION: " << run_years << " YEARS (Seed: " << seed << ")\n";
        std::cout << "========================================================================\n";
        int start_year = engine.year;
        for (int y = 0; y < run_years; ++y) {
            engine.year++;
            engine.tick_one_year();
        }
        std::cout << "\n========================================================================\n";
        std::cout << "  🏁 BATCH SIMULATION COMPLETE (" << start_year << " -> " << engine.year << ")\n";
        std::cout << "========================================================================\n";
        if (!export_replay.empty()) {
            std::string error;
            if (!engine.runtime.write_archive(engine, export_replay, error)) {
                std::cerr << error << '\n'; return 1;
            }
        }
        return 0;
    }

    if (!force_cli) {
        Aeon::AeonGUI gui;
        if (gui.init("AEON -- Emergent AI Civilization Simulator", 1280, 800)) {
            std::cout << "[SYSTEM] Launched AEON User-Friendly ImGui Dashboard GUI!\n";
            auto last_tick = std::chrono::high_resolution_clock::now();

            while (!gui.should_close()) {
                auto now = std::chrono::high_resolution_clock::now();
                float real_dt = std::chrono::duration<float>(now - last_tick).count();
                last_tick = now;

                engine.tick_second(real_dt);
                gui.render_frame(engine);
            }
            gui.shutdown();
            std::cout << "\nAEON simulation closed.\n";
            return 0;
        } else {
            std::cout << "[SYSTEM] Failed to create GUI window, falling back to CLI mode.\n";
        }
    }

    // CLI Mode Fallback
    Aeon::AeonCLI cli(engine);
    cli.draw();

    auto last_tick = std::chrono::high_resolution_clock::now();
    while (!cli.should_quit) {
        auto now = std::chrono::high_resolution_clock::now();
        float real_dt = std::chrono::duration<float>(now - last_tick).count();
        last_tick = now;

        engine.tick_second(real_dt);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "\nAEON simulation terminated.\n";
    return 0;
}
