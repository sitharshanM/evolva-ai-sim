#pragma once
#include "aeon_engine.h"
#include <iostream>

inline bool run_action_execution_tests() {
    using namespace Aeon;
    AeonEngine engine;
    engine.civs.resize(2);
    for (int i = 0; i < 2; ++i) {
        auto& civ = engine.civs[i];
        civ.id = i;
        civ.name = "Test " + std::to_string(i);
        civ.is_alive = 1;
        civ.stability = 80;
        civ.army_size = 2000;
        civ.economy.annual_income = 100;
        engine.ai_controllers.emplace_back(i);
    }
    int failures = 0;
    auto check = [&](bool result, const char* message) {
        if (!result) { std::cerr << message << '\n'; ++failures; }
    };
    AIDecision d;
    d.action_type = "DECLARE_WAR";
    d.target_civ = 1;
    d.is_validated = true;
    engine.civs[0].stability = 10;
    engine.apply_decision(0, d);
    check(!engine.civs[0].at_war && !engine.civs[1].at_war,
          "A stale validation flag bypassed execution validation");
    check(engine.ai_controllers[0].war_cooldown_.empty(), "Rejected action started cooldown");
    engine.civs[0].stability = 80;
    engine.apply_decision(0, d);
    check(engine.civs[0].at_war && engine.civs[1].at_war, "War was not bilateral");
    check(engine.ai_controllers[0].war_cooldown_.at(1) == engine.year, "War cooldown missing");
    d.action_type = "NEGOTIATE_PEACE";
    engine.apply_decision(0, d);
    check(!engine.civs[0].at_war && !engine.civs[1].at_war &&
          engine.civs[0].war_with_civ == -1 && engine.civs[1].war_with_civ == -1,
          "Peace did not clear both war states");
    check(engine.active_events.empty(), "Peace left an active war event");
    d.action_type = "DECLARE_WAR";
    engine.apply_decision(0, d);
    check(!engine.civs[0].at_war, "War cooldown was bypassed");
    d.action_type = "BUILD_MILITARY";
    d.target_civ = -1;
    engine.civs[0].economy.annual_income = 39;
    const float army = engine.civs[0].army_size;
    engine.apply_decision(0, d);
    check(engine.civs[0].army_size == army, "Unaffordable military build mutated state");
    std::cout << "Action execution tests: " << failures << " failures\n";
    return failures == 0;
}
