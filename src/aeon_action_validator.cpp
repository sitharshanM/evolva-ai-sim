#include "aeon_ruler_ai.h"
#include <cmath>
#include <algorithm>

namespace Aeon {
bool ActionValidator::validate(const AIDecision& dec,
                               const AeonCivilization& self,
                               const std::vector<AeonCivilization>& all_civs,
                               int current_year,
                               const std::unordered_map<int,int>& war_cooldown,
                               const std::unordered_map<int,int>& trade_cooldown,
                               std::string& out_reason) {
    out_reason.clear();
    if (self.is_alive <= 0.0f || !std::isfinite(self.is_alive)) {
        out_reason = "REJECTED: Actor is not alive";
        return false;
    }
    if (!std::isfinite(dec.confidence) || dec.confidence < 0 || dec.confidence > 1 ||
        !std::isfinite(dec.priority) || dec.priority < 0 || dec.priority > 1 ||
        !std::isfinite(dec.utility_score) || dec.utility_score < 0 || dec.utility_score > 1 ||
        !std::isfinite(dec.duration_years) || dec.duration_years <= 0) {
        out_reason = "REJECTED: Invalid decision numeric fields";
        return false;
    }
    const bool external = dec.action_type == "DECLARE_WAR" ||
        dec.action_type == "PROPOSE_TRADE" || dec.action_type == "FORM_ALLIANCE" ||
        dec.action_type == "NEGOTIATE_PEACE";
    auto reject_policy = [&](const char* reason) {
        out_reason = std::string("REJECTED: Political eligibility: ") + reason;
        return false;
    };
    const bool democratic = self.government == GovForm::REPUBLIC || self.government == GovForm::DEMOCRACY ||
        self.government == GovForm::FEDERATION;
    if (dec.action_type == "MILITARY_COUP") {
        int pillars = int(self.stability > 60) + int(self.military_loyalty > 70) +
            int(self.democratic_institution_strength > 60) + int(self.legitimacy > 70);
        if (pillars >= 3 || self.army_size < 125 ||
            (self.government == GovForm::MILITARY_JUNTA && self.military_loyalty >= 35))
            return reject_policy("State institutions or military conditions prevent a coup");
    }
    if (dec.action_type == "PROCLAIM_DICTATORSHIP" &&
        (self.government == GovForm::DICTATORSHIP ||
         (self.government == GovForm::MILITARY_JUNTA && (self.ruler_authority < 65 ||
          self.military_loyalty < 70 || self.democratic_institution_strength > 40 || self.years_current_gov < 5))))
        return reject_policy("Autocratic consolidation prerequisites are not met");
    if (dec.action_type == "CALL_ELECTION" && !democratic &&
        (self.democratic_institution_strength < 40 || self.government == GovForm::DICTATORSHIP))
        return reject_policy("Government cannot currently hold elections");
    if (dec.action_type == "SUPPRESS_UNREST" && (self.unrest < 25 || self.army_size < 125))
        return reject_policy("Insufficient unrest or security forces");
    if ((dec.action_type == "RESTORE_REPUBLIC" || dec.action_type == "RESTORE_DEMOCRACY") && democratic)
        return reject_policy("Democratic government is already established");
    if ((dec.action_type == "TRANSITION_TO_EMPIRE" && self.government == GovForm::EMPIRE) ||
        (dec.action_type == "TRANSITION_TO_THEOCRACY" && self.government == GovForm::THEOCRACY))
        return reject_policy("Requested government is already established");
    const bool internal = dec.action_type == "BUILD_MILITARY" ||
        dec.action_type == "RESEARCH" || dec.action_type == "EXPAND" ||
        dec.action_type == "DECLARE_MARTIAL_LAW" ||
        dec.action_type == "HOLD" || dec.action_type == "QUELL_UNREST" ||
        dec.action_type == "BUILD_INFRASTRUCTURE" || dec.action_type == "REFORM" ||
        dec.action_type == "CONCESSIONS" || dec.action_type == "EMERGENCY_LAWS" ||
        dec.action_type == "CALL_ELECTION" || dec.action_type == "SUPPRESS_UNREST" ||
        dec.action_type == "RESTORE_REPUBLIC" || dec.action_type == "RESTORE_DEMOCRACY" ||
        dec.action_type == "MILITARY_COUP" || dec.action_type == "PROCLAIM_DICTATORSHIP" ||
        dec.action_type == "TRANSITION_TO_EMPIRE" || dec.action_type == "TRANSITION_TO_THEOCRACY";
    if (!external && !internal) {
        out_reason = "REJECTED: Unknown action type " + dec.action_type;
        return false;
    }
    if ((internal && dec.target_civ != -1) || (external && dec.target_civ < 0)) {
        out_reason = "REJECTED: Invalid target for action";
        return false;
    }
    if (dec.action_type == "BUILD_MILITARY" &&
        (!std::isfinite(self.economy.annual_income) || self.economy.annual_income < 40.0f)) {
        out_reason = "REJECTED: Insufficient income to build military";
        return false;
    }

    // Rule 1: STRICTLY NO SELF-TARGETING
    if (dec.target_civ == self.id) {
        out_reason = "REJECTED: Cannot target self (self-targeting is invalid)";
        return false;
    }

    // Rule 2: If target is specified, it must exist and be alive
    if (dec.target_civ >= 0) {
        if (dec.target_civ >= (int)all_civs.size()) {
            out_reason = "REJECTED: Target civilization ID " + std::to_string(dec.target_civ) + " does not exist";
            return false;
        }
        const auto& target = all_civs[dec.target_civ];
        if (target.is_alive <= 0.0f || !std::isfinite(target.is_alive)) {
            out_reason = "REJECTED: Target civilization " + target.name + " is extinct/dead";
            return false;
        }
    }

    // Rule 3: DECLARE_WAR validation
    if (dec.action_type == "DECLARE_WAR") {
        if (dec.target_civ < 0) {
            out_reason = "REJECTED: DECLARE_WAR requires a valid target civilization";
            return false;
        }
        if (self.at_war) {
            out_reason = "REJECTED: " + self.name + " is already at war";
            return false;
        }
        const auto& target = all_civs[dec.target_civ];
        if (target.at_war) {
            out_reason = "REJECTED: Target " + target.name + " is already engaged in war";
            return false;
        }
        auto cd_it = war_cooldown.find(dec.target_civ);
        if (cd_it != war_cooldown.end() && (current_year - cd_it->second) < 8) {
            out_reason = "REJECTED: War cooldown active with " + target.name +
                         " (" + std::to_string(8 - (current_year - cd_it->second)) + " years remaining)";
            return false;
        }
        if (self.stability < 25.0f) {
            out_reason = "REJECTED: Stability too low to declare war (" + std::to_string(int(self.stability)) + "%)";
            return false;
        }
        if (self.army_size < 500.0f) {
            out_reason = "REJECTED: Army too small to initiate conflict (" + std::to_string(int(self.army_size)) + ")";
            return false;
        }
        auto rel_it = self.relations.find(dec.target_civ);
        if (rel_it != self.relations.end() && rel_it->second == DiplomacyStatus::ALLY) {
            out_reason = "REJECTED: Cannot declare war on formal ally without breaking alliance first";
            return false;
        }
    }

    // Rule 4: PROPOSE_TRADE validation & cooldowns
    if (dec.action_type == "PROPOSE_TRADE") {
        if (dec.target_civ < 0) {
            out_reason = "REJECTED: Trade proposal requires a target civilization";
            return false;
        }
        if (self.at_war || all_civs[dec.target_civ].at_war) {
            out_reason = "REJECTED: Cannot establish trade agreement during active war";
            return false;
        }
        // Check if trade agreement is already active
        if (self.active_trade_agreements.find(dec.target_civ) != self.active_trade_agreements.end()) {
            out_reason = "REJECTED: Active trade agreement already in effect with " + all_civs[dec.target_civ].name;
            return false;
        }
        // Check trade cooldown
        auto tcd_it = trade_cooldown.find(dec.target_civ);
        if (tcd_it != trade_cooldown.end() && (current_year - tcd_it->second) < 5) {
            out_reason = "REJECTED: Trade cooldown active with " + all_civs[dec.target_civ].name +
                         " (" + std::to_string(5 - (current_year - tcd_it->second)) + " years remaining)";
            return false;
        }
    }

    // Rule 5: FORM_ALLIANCE validation
    if (dec.action_type == "FORM_ALLIANCE") {
        if (dec.target_civ < 0) {
            out_reason = "REJECTED: Alliance requires a target civilization";
            return false;
        }
        if (self.at_war || all_civs[dec.target_civ].at_war) {
            out_reason = "REJECTED: Cannot form alliance during active wartime";
            return false;
        }
        auto sit = self.relations.find(dec.target_civ);
        if (sit != self.relations.end() && sit->second == DiplomacyStatus::ALLY) {
            out_reason = "REJECTED: Already allied with " + all_civs[dec.target_civ].name;
            return false;
        }
    }

    // Rule 6: NEGOTIATE_PEACE validation
    if (dec.action_type == "NEGOTIATE_PEACE") {
        if (!self.at_war) {
            out_reason = "REJECTED: " + self.name + " is not at war";
            return false;
        }
        if (dec.target_civ != self.war_with_civ) {
            out_reason = "REJECTED: Cannot negotiate peace with a realm not at war with self";
            return false;
        }
        const auto& target = all_civs[dec.target_civ];
        if (!target.at_war || target.war_with_civ != self.id) {
            out_reason = "REJECTED: War relationship is not reciprocal";
            return false;
        }
    }

    // Rule 7: EXPAND validation
    if (dec.action_type == "EXPAND") {
        if (self.at_war) {
            out_reason = "REJECTED: Cannot expand colonial frontier during active war";
            return false;
        }
        if (self.stability < 35.0f) {
            out_reason = "REJECTED: Domestic crisis/instability prevents territorial expansion";
            return false;
        }
        if (self.territory_tiles >= 400.0f) {
            out_reason = "REJECTED: Territorial saturation reached (maximum administrative capacity)";
            return false;
        }
    }

    return true;
}


}
