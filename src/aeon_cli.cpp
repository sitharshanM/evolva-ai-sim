#include "aeon_cli.h"
#include "aeon_ollama.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

namespace Aeon {

AeonCLI::AeonCLI(AeonEngine& engine) : engine_(engine) {}

// ─── draw ─────────────────────────────────────────────────────────────────────
void AeonCLI::draw() {
    draw_header();
    draw_events();
    draw_prompt();
}

void AeonCLI::draw_header() {
    std::cout << "\n" << engine_.status_line() << "\n";
}

void AeonCLI::draw_map() {
    // map command kept for on-demand use only
}

void AeonCLI::draw_events() {
    if (!engine_.active_events.empty()) {
        std::cout << "Active Events:";
        for (int i = 0; i < std::min(4, (int)engine_.active_events.size()); ++i)
            std::cout << "  " << engine_.active_events[i].description;
        std::cout << "\n";
    }
}

void AeonCLI::draw_prompt() {
    std::cout << "\n> " << std::flush;
}

// ─── handle_command ───────────────────────────────────────────────────────────
bool AeonCLI::handle_command(const std::string& raw_cmd) {
    std::istringstream ss(raw_cmd);
    std::string cmd;
    ss >> cmd;
    for (auto& c : cmd) c = char(std::tolower(c));

    if (cmd == "quit" || cmd == "exit" || cmd == "q") {
        should_quit = true;
        return false;

    } else if (cmd == "help" || cmd == "?") {
        print_help();

    } else if (cmd == "countries" || cmd == "civilizations" || cmd == "civs") {
        print_countries();

    } else if (cmd == "diplomacy") {
        print_diplomacy();

    } else if (cmd == "history") {
        int from = 1, to = engine_.year;
        ss >> from >> to;
        print_history(from, to);

    } else if (cmd == "people" || cmd == "rulers") {
        print_people();

    } else if (cmd == "council") {
        if (engine_.council_exists)
            std::cout << "Council of Nations formed in Year " << engine_.council_year
                      << " with " << engine_.council_members.size() << " members.\n";
        else
            std::cout << "Council of Nations has not formed yet.\n";

    } else if (cmd == "inspect") {
        std::string arg; ss >> arg;
        if (!arg.empty() && std::isdigit((unsigned char)arg[0])) {
            int id = std::stoi(arg);
            // If a second number follows, inspect tile; else inspect civ
            std::string arg2; ss >> arg2;
            if (!arg2.empty()) {
                int y = std::stoi(arg2);
                std::cout << engine_.inspect_tile(id, y);
            } else {
                std::cout << engine_.inspect_ai_decision(id);
            }
        } else {
            std::cout << "Usage:  inspect <civ_id>   or   inspect <x> <y>\n";
        }

    } else if (cmd == "pause") {
        engine_.paused = true;
        std::cout << "Simulation paused.\n";

    } else if (cmd == "resume") {
        engine_.paused = false;
        std::cout << "Simulation resumed.\n";

    } else if (cmd == "speed") {
        float s = 1.0f; ss >> s;
        engine_.speed = s;
        std::cout << "Speed set to " << s << "x\n";

    } else if (cmd == "step") {
        engine_.tick_second(1.0f / engine_.speed);
        std::cout << "Year " << engine_.year << "\n";

    } else if (cmd == "run") {
        int years = 10; ss >> years;
        std::cout << "Running " << years << " years...\n";
        for (int i = 0; i < years; ++i)
            engine_.tick_second(1.0f / engine_.speed);
        std::cout << "Done. Year " << engine_.year << "\n";

    } else if (cmd == "model") {
        // model <civ_id> <model_name>  -- e.g. model 0 llama3.1
        // model <civ_id> rule_based    -- revert to rule-based
        // model list                   -- show current models
        std::string arg1; ss >> arg1;
        if (arg1 == "list" || arg1.empty()) {
            std::cout << "\n--- AI MODELS ---\n";
            for (int i = 0; i < (int)engine_.civs.size(); ++i) {
                std::cout << "  [" << i << "] " << engine_.civs[i].name
                          << " : " << engine_.ai_controllers[i].model_name << "\n";
            }
            std::cout << "  Ollama status: " << (AeonOllama::is_available() ? "CONNECTED" : "OFFLINE") << "\n\n";
        } else {
            // model <id> <name>
            int id = 0;
            try { id = std::stoi(arg1); } catch(...) { std::cout << "Invalid civ id.\n"; return true; }
            std::string model_name; ss >> model_name;
            if (model_name.empty()) { std::cout << "Usage: model <civ_id> <model_name>\n"; return true; }
            if (id >= 0 && id < (int)engine_.ai_controllers.size()) {
                engine_.ai_controllers[id].model_name = model_name;
                std::cout << engine_.civs[id].name << " AI set to: " << model_name << "\n";
            } else {
                std::cout << "Invalid civ id.\n";
            }
        }

    } else if (cmd == "market") {
        std::cout << "\n" << engine_.market_engine.market_report() << "\n";

    } else if (cmd == "religions") {
        std::cout << "\n--- WORLD RELIGIONS & FAITH ---\n";
        for (const auto& r : engine_.aeon_religion_engine.religions) {
            std::cout << "  - " << r.name << " (Holy Seat: " << r.holy_city << ") | Relic: " << r.sacred_relic << "\n";
        }

    } else if (cmd == "factions") {
        std::cout << "\n--- INTERNAL POLITICAL FACTIONS ---\n";
        for (const auto& c : engine_.civs) {
            if (c.is_alive <= 0.0f) continue;
            std::cout << "  " << c.name << " Factions:\n";
            for (const auto& f : c.factions) {
                std::cout << "    - " << f.name << " (" << faction_type_name(f.type) << ")"
                          << " | Clout: " << int(f.influence) << "%"
                          << " | Loyalty: " << int(f.loyalty) << "%"
                          << " | Rebellion Risk: " << int(f.rebellion_risk) << "%\n";
            }
        }
        std::cout << "\n";

    } else if (cmd == "trade") {
        std::cout << "\n--- ACTIVE TRADE ROUTES ---\n";
        for (const auto& tr : engine_.market_engine.active_routes) {
            if (tr.active) {
                std::cout << "  Route #" << tr.id << ": " << tr.origin_city
                          << " <-> " << tr.dest_city << " (" << tr.primary_good
                          << ", Vol: " << int(tr.annual_volume) << " gold/yr)\n";
            }
        }
        std::cout << "\n";

    } else if (cmd == "save") {
        std::string filename; ss >> filename;
        if (filename.empty()) filename = "aeon_universe_y" + std::to_string(engine_.year);
        engine_.save_world(filename);

    } else if (cmd == "load") {
        std::string filename; ss >> filename;
        if (filename.empty()) filename = "aeon_universe_y" + std::to_string(engine_.year);
        engine_.load_world(filename);

    } else if (cmd == "chronicle") {
        std::cout << engine_.chronicler.get_chronicle();

    } else if (cmd == "status") {
        std::cout << engine_.status_line() << "\n";
        print_countries();

    } else {
        if (!cmd.empty())
            std::cout << "Unknown command '" << cmd << "'. Type 'help' for list.\n";
    }
    return true;
}

// ─── Sub-commands ─────────────────────────────────────────────────────────────
void AeonCLI::print_help() {
    std::cout << "\n"
        "AEON Commands\n"
        "-------------\n"
        "  civs / countries      List all civilizations and their stats\n"
        "  diplomacy             Show diplomatic relations between civs\n"
        "  market                Show global resource prices and trade\n"
        "  religions             Show world faiths and Holy Crusades\n"
        "  factions              Show political factions & rebellion risk\n"
        "  trade                 List active inter-empire trade routes\n"
        "  history [from] [to]   Show history events between two years\n"
        "  chronicle             Read LLM Grand History Chronicle Book\n"
        "  people / rulers       List living rulers and important characters\n"
        "  inspect <civ_id>      Show AI ruler goals and detailed civ info\n"
        "  council               Show Council of Nations status\n"
        "  save [filename]       Save current universe to JSON file\n"
        "  load [filename]       Load universe state from JSON file\n"
        "  pause / resume        Pause or resume the simulation\n"
        "  speed <n>             Set simulation speed (1, 10, 100)\n"
        "  step                  Advance one year manually\n"
        "  run <years>           Run N years immediately\n"
        "  quit / exit           Exit AEON\n\n";
}

void AeonCLI::print_countries() {
    std::cout << "\n--- CIVILIZATIONS ---\n";
    for (const auto& c : engine_.civs) {
        if (c.is_alive <= 0.0f) {
            std::cout << "  [EXTINCT]  " << c.name << "\n";
            continue;
        }
        std::cout << "  " << c.name
                  << "  |  Pop: " << c.population.total / 1000 << "k"
                  << "  |  Tech: " << tech_era_name(c.tech.era)
                  << "  |  Gov: " << gov_form_name(c.government)
                  << "  |  Stability: " << int(c.stability) << "%"
                  << "  |  Army: " << int(c.army_size)
                  << (c.at_war ? "  |  AT WAR" : "")
                  << "\n";
    }
    std::cout << "\n";
}

void AeonCLI::print_diplomacy() {
    std::cout << "\n--- DIPLOMATIC RELATIONS ---\n";
    bool any = false;
    for (const auto& c : engine_.civs) {
        if (c.is_alive <= 0.0f) continue;
        for (const auto& r : c.relations) {
            if (r.first >= 0 && r.first < (int)engine_.civs.size() &&
                r.second != DiplomacyStatus::NEUTRAL) {
                std::cout << "  " << c.name
                          << " -> " << engine_.civs[r.first].name
                          << " : " << diplomacy_status_name(r.second) << "\n";
                any = true;
            }
        }
    }
    if (!any) std::cout << "  All civilizations are Neutral with each other.\n";
    std::cout << "\n";
}

void AeonCLI::print_history(int from_year, int to_year) {
    std::cout << "\n--- HISTORY: Year " << from_year << " to " << to_year << " ---\n";
    std::cout << engine_.history.timeline_summary(from_year, to_year);
    std::cout << "\n";
}

void AeonCLI::print_people() {
    std::cout << "\n--- RULERS & CHARACTERS ---\n";
    for (const auto& ch : engine_.characters) {
        if (!ch.is_alive) continue;
        std::cout << "  " << ch.name
                  << "  Age: " << ch.age
                  << "  Civ: " << (ch.civ_id >= 0 ? engine_.civs[ch.civ_id].name : "Independent")
                  << "  Influence: " << int(ch.influence)
                  << "\n";
    }
    std::cout << "\n";
}

} // namespace Aeon
