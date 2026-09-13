#include "aeon_ruler_ai.h"
#include <iostream>
#include <limits>
#include <stdexcept>

int main() {
    using namespace Aeon;
    std::vector<AeonCivilization> civs(2);
    for (int i = 0; i < 2; ++i) {
        civs[i].id = i;
        civs[i].is_alive = 1;
        civs[i].stability = 80;
        civs[i].army_size = 2000;
        civs[i].economy.annual_income = 100;
    }
    std::unordered_map<int, int> war, trade;
    std::string reason;
    AIDecision d;
    int checks = 0;
    auto check = [&](bool expected) {
        const bool result = ActionValidator::validate(d, civs[0], civs, 2030, war, trade, reason);
        if (result != expected || (!result && reason.empty()))
            throw std::runtime_error("Unexpected validation: " + d.action_type + " " + reason);
        ++checks;
    };
    check(true);
    d.action_type = "UNKNOWN"; check(false);
    d.action_type = "HOLD"; d.target_civ = 0; check(false);
    d.target_civ = -2; check(false);
    d.target_civ = -1; d.confidence = std::numeric_limits<float>::quiet_NaN(); check(false);
    d.confidence = 0.5f; d.duration_years = 0; check(false);
    d.duration_years = 1; civs[0].is_alive = 0; check(false);
    civs[0].is_alive = 1;
    d.action_type = "BUILD_MILITARY"; civs[0].economy.annual_income = 39; check(false);
    civs[0].economy.annual_income = 40; check(true);
    d.action_type = "DECLARE_WAR"; check(false);
    d.target_civ = 2; check(false);
    d.target_civ = 0; check(false);
    d.target_civ = 1; check(true);
    war[1] = 2029; check(false);
    war.clear(); civs[1].is_alive = 0; check(false);
    civs[1].is_alive = 1; civs[0].relations[1] = DiplomacyStatus::ALLY; check(false);
    civs[0].relations.clear();
    d.action_type = "PROPOSE_TRADE"; check(true);
    trade[1] = 2029; check(false);
    trade.clear(); civs[0].active_trade_agreements[1] = 10; check(false);
    d.action_type = "NEGOTIATE_PEACE"; check(false);
    civs[0].at_war = true; civs[0].war_with_civ = 1; check(false);
    civs[1].at_war = true; civs[1].war_with_civ = 0; check(true);
    d.action_type = "MILITARY_COUP"; d.target_civ = -1;
    civs[0].military_loyalty = 90; civs[0].legitimacy = 90;
    civs[0].democratic_institution_strength = 90; check(false);
    d.action_type = "PROCLAIM_DICTATORSHIP"; civs[0].government = GovForm::DICTATORSHIP; check(false);
    d.action_type = "CALL_ELECTION"; check(false);
    civs[0].government = GovForm::DEMOCRACY; check(true);
    d.action_type = "RESTORE_DEMOCRACY"; check(false);
    std::cout << checks << " action validation checks passed\n";
}
