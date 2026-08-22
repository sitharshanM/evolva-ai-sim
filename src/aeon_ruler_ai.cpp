// =============================================================================
//  aeon_ruler_ai.cpp  —  Authoritative AI Decision & Validation System
// =============================================================================
#include "aeon_ruler_ai.h"
#include "aeon_government.h"
#include "aeon_ollama.h"
#include "aeon_nemotron.h"
#include "aeon_openrouter.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  ActionValidator  —  Authoritative simulation validation barrier
// ─────────────────────────────────────────────────────────────────────────────
bool ActionValidator::validate(const AIDecision& dec,
                               const AeonCivilization& self,
                               const std::vector<AeonCivilization>& all_civs,
                               int current_year,
                               const std::unordered_map<int,int>& war_cooldown,
                               const std::unordered_map<int,int>& trade_cooldown,
                               std::string& out_reason) {
    if (dec.action_type == "HOLD" || dec.action_type == "QUELL_UNREST" ||
        dec.action_type == "BUILD_INFRASTRUCTURE" || dec.action_type == "REFORM" ||
        dec.action_type == "CONCESSIONS" || dec.action_type == "EMERGENCY_LAWS" ||
        dec.action_type == "CALL_ELECTION" || dec.action_type == "SUPPRESS_UNREST" ||
        dec.action_type == "RESTORE_REPUBLIC" || dec.action_type == "RESTORE_DEMOCRACY" ||
        dec.action_type == "MILITARY_COUP" || dec.action_type == "PROCLAIM_DICTATORSHIP" ||
        dec.action_type == "TRANSITION_TO_EMPIRE" || dec.action_type == "TRANSITION_TO_THEOCRACY") {
        return true;
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
        if (target.is_alive <= 0.0f) {
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

// ─────────────────────────────────────────────────────────────────────────────
//  Memory helpers (strictly clamped [-100, 100])
// ─────────────────────────────────────────────────────────────────────────────
float AeonRulerAI::perceived_army(int enemy_civ_id, float actual_army) const {
    float trust = 0.0f;
    for (const auto& m : memory) {
        if (m.other_civ == enemy_civ_id) { trust = m.trust; break; }
    }
    float uncertainty = (trust < 0) ? 0.20f : 0.08f;
    float factor = 1.0f + uncertainty * (noise_seed_ - 0.5f) * 2.0f;
    return std::max(100.0f, actual_army * factor);
}

void AeonRulerAI::add_memory(int other_civ, float trust, float fear, float hatred, int year) {
    if (other_civ == civ_id_ || other_civ < 0) return;

    for (auto& m : memory) {
        if (m.other_civ == other_civ) {
            m.trust   = std::clamp(m.trust + trust, -100.0f, 100.0f);
            m.fear    = std::clamp(m.fear + fear, 0.0f, 100.0f);
            m.hatred  = std::clamp(m.hatred + hatred, 0.0f, 100.0f);
            m.year    = year;
            return;
        }
    }
    if (memory.size() < 50) {
        MemoryEntry e;
        e.other_civ = other_civ;
        e.trust     = std::clamp(trust, -100.0f, 100.0f);
        e.fear      = std::clamp(fear, 0.0f, 100.0f);
        e.hatred    = std::clamp(hatred, 0.0f, 100.0f);
        e.year      = year;
        memory.push_back(e);
    }
}

void AeonRulerAI::decay_memory(float years_passed) {
    for (auto& m : memory) {
        m.hatred = std::max(0.0f, m.hatred - years_passed * 0.4f);
        m.fear   = std::max(0.0f, m.fear   - years_passed * 0.3f);
        if (m.trust < 0.0f) {
            m.trust = std::min(0.0f, m.trust + years_passed * 0.2f);
        } else if (m.trust > 0.0f) {
            m.trust = std::max(0.0f, m.trust - years_passed * 0.1f);
        }
        m.trust = std::clamp(m.trust, -100.0f, 100.0f);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Strategic Goal update
// ─────────────────────────────────────────────────────────────────────────────
void AeonRulerAI::update_strategic_goal(const AeonCivilization& self, const AeonCharacter* ruler) {
    if (self.stability < 35.0f || self.population.total < 25000 || self.army_size < 500.0f) {
        strategic_goal = CivGoal::SURVIVE;
        return;
    }

    if (ruler) {
        switch (ruler->trait) {
            case RulerTrait::WARMONGER:
            case RulerTrait::MILITARIST:
                strategic_goal = CivGoal::BECOME_MILITARY_SUPERPOWER; return;
            case RulerTrait::AUTHORITARIAN:
            case RulerTrait::TYRANT:
            case RulerTrait::PARANOID:
                strategic_goal = CivGoal::BUILD_EMPIRE; return;
            case RulerTrait::DEMOCRATIC:
            case RulerTrait::DIPLOMAT:
                strategic_goal = (self.diplomacy_pref > 0.6f) ? CivGoal::ESTABLISH_FEDERATION : CivGoal::MAINTAIN_PEACE; return;
            case RulerTrait::SCHOLAR:
            case RulerTrait::TECHNOCRAT:
                strategic_goal = CivGoal::BECOME_TECH_LEADER; return;
            case RulerTrait::MERCHANT:
                strategic_goal = CivGoal::DOMINATE_TRADE; return;
            case RulerTrait::EXPANSIONIST:
                strategic_goal = CivGoal::EXPAND_TERRITORY; return;
            case RulerTrait::IDEALIST:
                strategic_goal = CivGoal::CULTURAL_DOMINANCE; return;
            case RulerTrait::REFORMER:
            case RulerTrait::PRAGMATIST:
            case RulerTrait::ISOLATIONIST:
                strategic_goal = CivGoal::MAINTAIN_PEACE; return;
        }
    }

    if (self.aggression > 0.70f)
        strategic_goal = CivGoal::BUILD_EMPIRE;
    else if (self.science_pref > 0.65f)
        strategic_goal = CivGoal::BECOME_TECH_LEADER;
    else if (self.trade_pref > 0.65f)
        strategic_goal = CivGoal::DOMINATE_TRADE;
    else if (self.diplomacy_pref > 0.60f)
        strategic_goal = CivGoal::MAINTAIN_PEACE;
    else
        strategic_goal = CivGoal::EXPAND_TERRITORY;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Utility Scoring Functions (Strictly normalized in [0.0, 1.0])
// ─────────────────────────────────────────────────────────────────────────────
float AeonRulerAI::score_quell_unrest(const AeonCivilization& self,
                                     const AeonCharacter* ruler,
                                     int year,
                                     std::string& out_breakdown) const {
    (void)year;
    // Internal crisis override: if stability < 40%, quelling unrest is top priority
    if (self.stability >= 65.0f) return 0.0f;

    float instability = (65.0f - self.stability) / 65.0f; // 0.0 to 1.0
    float comp_skill = ruler ? ruler->competence : 0.5f;

    float score = std::clamp(instability * 0.70f + comp_skill * 0.20f + 0.10f, 0.0f, 1.0f);
    out_breakdown = "instability:" + std::to_string(int(instability * 100)) + "% stability:" + std::to_string(int(self.stability)) + "%";
    return score;
}

float AeonRulerAI::score_war(const AeonCivilization& self,
                             const AeonCivilization& target,
                             const AeonHistory& history,
                             const AeonCharacter* ruler,
                             int year,
                             std::string& out_breakdown) const {
    if (self.id == target.id || self.at_war || target.is_alive <= 0.0f) return 0.0f;

    auto cd = war_cooldown_.find(target.id);
    if (cd != war_cooldown_.end() && (year - cd->second) < 8) return 0.0f;

    float self_mil = self.military_power * (1.0f + (ruler ? ruler->military_skill * 0.3f : 0.0f));
    float perceived = perceived_army(target.id, target.military_power);
    float relative_power = self_mil / (self_mil + perceived + 1.0f);

    float trust = 0.0f, hatred = 0.0f, fear = 0.0f, rivalry = 0.0f;
    const auto* rel = history.relation_view(self.id, target.id);
    if (rel) {
        trust   = rel->trust;
        hatred  = rel->hatred;
        fear    = rel->fear;
        rivalry = rel->rivalry;
    }
    for (const auto& m : memory) {
        if (m.other_civ == target.id) {
            trust  = std::min(trust, m.trust);
            hatred = std::max(hatred, m.hatred);
        }
    }

    float dx = float(self.capital_x - target.capital_x);
    float dy = float(self.capital_y - target.capital_y);
    float dist = std::sqrt(dx*dx + dy*dy);
    float proximity = std::clamp(1.0f - (dist / 140.0f), 0.0f, 1.0f);

    float upkeep_burden = std::clamp((self.army_size * 0.035f) / std::max(10.0f, self.economy.gdp), 0.0f, 1.0f);
    float exhaustion_penalty = std::clamp(self.war_exhaustion / 100.0f, 0.0f, 1.0f);
    float stability_factor = std::clamp(self.stability / 100.0f, 0.0f, 1.0f);

    if (self.stability < 35.0f) return 0.0f;

    // 14 Personality Parameters
    float f_ambition = ruler ? (ruler->ambition - 0.5f) * 0.25f : 0.0f;
    float f_cruelty  = ruler ? (ruler->cruelty - 0.5f) * 0.15f : 0.0f;
    float f_risk     = ruler ? (ruler->risk_tolerance - 0.5f) * 0.15f : 0.0f;
    float f_loyalty  = ruler ? (ruler->loyalty * 0.20f) : 0.0f;

    // Realm Personality & Faction Pressure
    float f_realm_mil = (self.national_personality.military_pref - 0.5f) * 0.25f;
    float f_factions  = 0.0f;
    for (const auto& fac : self.factions) {
        if (fac.type == FactionType::MILITARY_NOBLES || fac.type == FactionType::AUTHORITARIANS) {
            f_factions += (fac.influence / 100.0f) * 0.12f;
        }
    }

    // Strategic Plan Alignment
    float f_strategic = 0.0f;
    if (self.strategic_plan.immediate_goal == "WAR_MOBILIZATION" && self.strategic_plan.immediate_target_civ == target.id) {
        f_strategic += 0.25f;
    }
    if ((self.secret_goal.type == SecretGoalType::DESTROY_RIVAL || self.secret_goal.type == SecretGoalType::AVENGE_DEFEAT)
        && self.secret_goal.target_civ_id == target.id) {
        f_strategic += 0.20f;
    }

    float f_hatred = (hatred / 100.0f) * 0.25f;
    float f_trust_inhibit = (std::max(0.0f, trust) / 100.0f) * 0.35f;

    float raw = (self.aggression * 0.20f) + (relative_power * 0.25f) + f_hatred + (rivalry / 100.0f * 0.15f)
                + (proximity * 0.10f) + f_ambition + f_cruelty + f_risk + f_realm_mil + f_factions + f_strategic
                - f_loyalty - f_trust_inhibit - (fear / 100.0f * 0.20f) - (exhaustion_penalty * 0.30f)
                - (upkeep_burden * 0.20f) - ((1.0f - stability_factor) * 0.30f);

    float score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "rel_pwr:+" << int(relative_power * 100) << "% hate:+" << int(hatred)
       << " amb:+" << int(f_ambition * 100) << "% strat:+" << int(f_strategic * 100)
       << "% trst_inh:-" << int(f_trust_inhibit * 100) << "%";
    out_breakdown = ss.str();

    return score;
}

float AeonRulerAI::score_military(const AeonCivilization& self,
                                  const std::vector<AeonCivilization>& all_civs,
                                  const AeonCharacter* ruler,
                                  int year,
                                  std::string& out_breakdown) const {
    float max_threat = 0.0f;
    for (const auto& other : all_civs) {
        if (other.id == self.id || other.is_alive <= 0.0f) continue;
        float perceived = perceived_army(other.id, other.army_size);
        float threat = perceived / (self.army_size + perceived + 1.0f);
        if (threat > max_threat) max_threat = threat;
    }

    float upkeep_burden = std::clamp((self.army_size * 0.035f) / std::max(10.0f, self.economy.gdp), 0.0f, 1.0f);
    float manpower_drain = std::clamp(self.army_size / std::max(1000.0f, (float)self.population.total * 0.08f), 0.0f, 1.0f);
    float mil_skill = ruler ? ruler->military_skill : 0.5f;

    auto it = action_last_used_year_.find("BUILD_MILITARY");
    float fatigue = (it != action_last_used_year_.end() && (year - it->second) < 3) ? (0.25f - (year - it->second) * 0.08f) : 0.0f;

    float f_paranoia = ruler ? ruler->paranoia * 0.20f : 0.0f;
    float f_realm_mil = (self.national_personality.military_pref - 0.5f) * 0.20f;

    float f_faction = 0.0f;
    for (const auto& fac : self.factions) {
        if (fac.type == FactionType::MILITARY_NOBLES) {
            f_faction += (fac.influence / 100.0f) * 0.15f;
        }
    }

    float f_strategic = 0.0f;
    if (self.strategic_plan.immediate_goal == "BUILD_MILITARY" || self.strategic_plan.medium_term_goal == "MILITARY_SUPERIORITY") {
        f_strategic += 0.20f;
    }

    float raw = (max_threat * 0.30f) + (self.aggression * 0.15f) + (mil_skill * 0.15f)
                + f_paranoia + f_realm_mil + f_faction + f_strategic
                + (self.at_war ? 0.30f : 0.0f)
                - (upkeep_burden * 0.30f) - (manpower_drain * 0.20f) - fatigue;

    float score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "threat:+" << int(max_threat * 100) << "% mil_goal:+" << int(f_strategic * 100)
       << "% faction:+" << int(f_faction * 100) << "% upkeep:-" << int(upkeep_burden * 100) << "%";
    out_breakdown = ss.str();

    return score;
}

float AeonRulerAI::score_trade(const AeonCivilization& self,
                               const AeonCivilization& target,
                               const AeonHistory& history,
                               const AeonCharacter* ruler,
                               int year,
                               std::string& out_breakdown) const {
    if (self.id == target.id || self.at_war || target.at_war || target.is_alive <= 0.0f) return 0.0f;
    if (self.active_trade_agreements.find(target.id) != self.active_trade_agreements.end()) return 0.0f;

    auto tcd_it = trade_cooldown_.find(target.id);
    if (tcd_it != trade_cooldown_.end() && (year - tcd_it->second) < 5) return 0.0f;

    if (self.stability < 35.0f) return 0.0f;

    float trust = 0.0f, hatred = 0.0f;
    const auto* rel = history.relation_view(self.id, target.id);
    if (rel) {
        trust  = rel->trust;
        hatred = rel->hatred;
    }
    for (const auto& m : memory) {
        if (m.other_civ == target.id) {
            trust = std::max(trust, m.trust);
            hatred = std::max(hatred, m.hatred);
        }
    }

    if (trust < -20.0f || hatred > 40.0f) return 0.0f;

    float norm_trust = std::clamp((trust + 100.0f) / 200.0f, 0.0f, 1.0f);
    float partner_wealth = std::clamp(target.economy.gdp / 5000.0f, 0.0f, 1.0f);
    float econ_skill = ruler ? ruler->economic_skill : 0.5f;

    auto it = action_last_used_year_.find("PROPOSE_TRADE");
    float fatigue = (it != action_last_used_year_.end() && (year - it->second) < 3) ? 0.20f : 0.0f;

    float f_greed = ruler ? ruler->greed * 0.25f : 0.0f;
    float f_realm_merch = (self.national_personality.economic_pref - 0.5f) * 0.25f;

    float f_faction = 0.0f;
    for (const auto& fac : self.factions) {
        if (fac.type == FactionType::MERCHANT_GUILD) {
            f_faction += (fac.influence / 100.0f) * 0.20f;
        }
    }

    float f_strategic = 0.0f;
    if (self.strategic_plan.medium_term_goal == "EXPAND_ECONOMIC_CAPACITY") f_strategic += 0.20f;

    float raw = (self.trade_pref * 0.25f) + (norm_trust * 0.20f) + (partner_wealth * 0.15f)
                + (econ_skill * 0.15f) + f_greed + f_realm_merch + f_faction + f_strategic - fatigue;

    float score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "trst:+" << int(trust) << " wealth:+" << int(partner_wealth * 100)
       << "% greed:+" << int(f_greed * 100) << "% merchant_fac:+" << int(f_faction * 100) << "%";
    out_breakdown = ss.str();

    return score;
}

float AeonRulerAI::score_research(const AeonCivilization& self,
                                  const AeonCharacter* ruler,
                                  int year,
                                  std::string& out_breakdown) const {
    float tech_gap = std::clamp((100.0f - self.tech.progress) / 100.0f, 0.0f, 1.0f);
    float econ_strength = std::clamp(self.economy.gdp / 3000.0f, 0.0f, 1.0f);
    float sci_skill = ruler ? ruler->scientific_skill : 0.5f;

    auto it = action_last_used_year_.find("RESEARCH");
    float fatigue = (it != action_last_used_year_.end() && (year - it->second) < 2) ? 0.15f : 0.0f;

    float f_intellect = ruler ? (ruler->intelligence - 0.5f) * 0.20f : 0.0f;
    float f_realm_sci = (self.national_personality.science_pref - 0.5f) * 0.25f;

    float f_strategic = 0.0f;
    if (self.strategic_plan.medium_term_goal == "TECH_LEADERSHIP") f_strategic += 0.25f;
    if (strategic_goal == CivGoal::BECOME_TECH_LEADER) f_strategic += 0.15f;

    float raw = (self.science_pref * 0.25f) + (tech_gap * 0.20f) + (econ_strength * 0.15f)
                + (sci_skill * 0.15f) + f_intellect + f_realm_sci + f_strategic - fatigue;

    float score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "sci_pref:+" << int(self.science_pref * 100) << "% gap:+" << int(tech_gap * 100)
       << "% intellect:+" << int(f_intellect * 100) << "% strat:+" << int(f_strategic * 100) << "%";
    out_breakdown = ss.str();

    return score;
}

float AeonRulerAI::score_diplomacy(const AeonCivilization& self,
                                   const AeonCivilization& target,
                                   const AeonHistory& history,
                                   const AeonCharacter* ruler,
                                   int year,
                                   std::string& out_breakdown) const {
    if (self.id == target.id || self.at_war || target.at_war || target.is_alive <= 0.0f) return 0.0f;

    auto sit = self.relations.find(target.id);
    if (sit != self.relations.end() && sit->second == DiplomacyStatus::ALLY) return 0.0f;

    if (self.stability < 35.0f) return 0.0f;

    float trust = 0.0f, hatred = 0.0f;
    const auto* rel = history.relation_view(self.id, target.id);
    if (rel) {
        trust  = rel->trust;
        hatred = rel->hatred;
    }
    for (const auto& m : memory) {
        if (m.other_civ == target.id) {
            trust  = std::max(trust, m.trust);
            hatred = std::max(hatred, m.hatred);
        }
    }

    if (trust < 20.0f || hatred > 30.0f) return 0.0f;

    float norm_trust = std::clamp((trust + 100.0f) / 200.0f, 0.0f, 1.0f);
    float dip_skill = ruler ? ruler->diplomatic_skill : 0.5f;

    auto it = action_last_used_year_.find("FORM_ALLIANCE");
    float fatigue = (it != action_last_used_year_.end() && (year - it->second) < 4) ? 0.25f : 0.0f;

    float f_loyalty = ruler ? (ruler->loyalty - 0.5f) * 0.20f : 0.0f;
    float f_realm_dip = (self.national_personality.diplomacy_pref - 0.5f) * 0.25f;

    float f_strategic = 0.0f;
    if (self.strategic_plan.medium_term_goal == "DIPLOMATIC_ALLIANCE") f_strategic += 0.25f;

    float raw = (self.diplomacy_pref * 0.25f) + (norm_trust * 0.25f) + (dip_skill * 0.15f)
                + f_loyalty + f_realm_dip + f_strategic - (self.aggression * 0.15f)
                - ((hatred / 100.0f) * 0.30f) - fatigue;

    float score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "trst:+" << int(trust) << " diplo_goal:+" << int(f_strategic * 100)
       << "% loyalty:+" << int(f_loyalty * 100) << "%";
    out_breakdown = ss.str();

    return score;
}

float AeonRulerAI::score_expand(const AeonCivilization& self,
                                const AeonCharacter* ruler,
                                int year,
                                std::string& out_breakdown) const {
    if (self.at_war || self.stability < 35.0f) return 0.0f;

    float territory_saturation = std::clamp(self.territory_tiles / 300.0f, 0.0f, 1.0f);
    float manpower_availability = std::clamp(self.army_size / 20000.0f, 0.0f, 1.0f);
    float comp_skill = ruler ? ruler->competence : 0.5f;

    auto it = action_last_used_year_.find("EXPAND");
    int years_since_expand = (it != action_last_used_year_.end()) ? (year - it->second) : 999;
    float expand_cooldown = (years_since_expand < 4) ? (0.45f - years_since_expand * 0.10f) : 0.0f;

    float f_ambition = ruler ? (ruler->ambition - 0.5f) * 0.20f : 0.0f;
    float f_realm_exp = (self.national_personality.expansion_pref - 0.5f) * 0.25f;

    float f_strategic = 0.0f;
    if (self.strategic_plan.immediate_goal == "EXPAND") f_strategic += 0.25f;

    float raw = (self.expansion_pref * 0.35f) + (manpower_availability * 0.20f) + (comp_skill * 0.15f)
                + f_ambition + f_realm_exp + f_strategic - (territory_saturation * 0.25f) - expand_cooldown;

    float score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "exp_pref:+" << int(self.expansion_pref * 100) << "% manpower:+" << int(manpower_availability * 100)
       << "% amb:+" << int(f_ambition * 100) << "% exp_drive:+" << int(f_realm_exp * 100) << "%";
    out_breakdown = ss.str();

    return score;
}

float AeonRulerAI::score_peace(const AeonCivilization& self,
                               const AeonCivilization& target,
                               const AeonCharacter* ruler,
                               std::string& out_breakdown) const {
    (void)target;
    if (!self.at_war) return 0.0f;

    float exhaustion = std::clamp(self.war_exhaustion / 100.0f, 0.0f, 1.0f);
    float instability = std::clamp((100.0f - self.stability) / 100.0f, 0.0f, 1.0f);
    float dip_skill = ruler ? ruler->diplomatic_skill : 0.5f;

    float raw = (exhaustion * 0.50f) + (instability * 0.35f) + (dip_skill * 0.15f) - (self.aggression * 0.30f);
    if (strategic_goal == CivGoal::MAINTAIN_PEACE || strategic_goal == CivGoal::SURVIVE)
        raw += 0.25f;

    float score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "exhaust:" << int(self.war_exhaustion) << "% stability:" << int(self.stability) << "%";
    out_breakdown = ss.str();

    return score;
}

float AeonRulerAI::score_infrastructure(const AeonCivilization& self,
                                       const AeonCharacter* ruler,
                                       int year,
                                       std::string& out_breakdown) const {
    float econ_capacity = std::clamp(self.economy.gdp / 2500.0f, 0.0f, 1.0f);
    float comp_skill = ruler ? ruler->competence : 0.5f;
    float stability_need = std::clamp((90.0f - self.stability) / 100.0f, 0.0f, 1.0f);

    auto it = action_last_used_year_.find("BUILD_INFRASTRUCTURE");
    float fatigue = (it != action_last_used_year_.end() && (year - it->second) < 2) ? 0.15f : 0.0f;

    float ruler_boost = (ruler && (ruler->trait == RulerTrait::REFORMER || ruler->trait == RulerTrait::ISOLATIONIST)) ? 0.25f : 0.0f;

    float raw = (econ_capacity * 0.30f) + (comp_skill * 0.25f) + (stability_need * 0.35f) + ruler_boost + 0.10f - fatigue;
    float score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "econ_cap:" << int(econ_capacity * 100) << "% stability:" << int(self.stability) << "%";
    out_breakdown = ss.str();

    return score;
}

// ─────────────────────────────────────────────────────────────────────────────
//  estimate_consequence_lookahead  —  1-Year Decision Consequence Estimator
// ─────────────────────────────────────────────────────────────────────────────
float AeonRulerAI::estimate_consequence_lookahead(const AeonCivilization& self,
                                                  const std::string& action_type,
                                                  int target_civ,
                                                  const AeonCharacter* ruler) const {
    (void)target_civ;
    float future_val = 0.0f;
    float comp = ruler ? ruler->competence : 0.5f;

    if (action_type == "QUELL_UNREST" || action_type == "EMERGENCY_LAWS" || action_type == "REFORM") {
        if (self.stability < 30.0f || self.unrest > 60.0f) {
            future_val += 0.25f * comp; // Preventing state collapse has massive future payoff
        }
    } else if (action_type == "RESEARCH") {
        future_val += 0.08f * (ruler ? ruler->scientific_skill : 0.5f);
    } else if (action_type == "BUILD_INFRASTRUCTURE") {
        if (self.stability > 50.0f) {
            future_val += 0.10f * (ruler ? ruler->economic_skill : 0.5f);
        } else {
            future_val -= 0.20f; // Building roads while country collapses politically is wasteful
        }
    } else if (action_type == "BUILD_MILITARY") {
        if (self.at_war || self.military_power < 300.0f) {
            future_val += 0.20f * (ruler ? ruler->military_skill : 0.5f);
        } else {
            future_val -= 0.05f; // Upkeep drain
        }
    } else if (action_type == "DECLARE_WAR") {
        if (self.war_exhaustion > 50.0f || self.stability < 40.0f) {
            future_val -= 0.35f; // Destructive war when already exhausted
        }
    }
    return future_val;
}

// ─────────────────────────────────────────────────────────────────────────────
//  evaluate_utilities  —  Score all candidate actions, return sorted list
// ─────────────────────────────────────────────────────────────────────────────
std::vector<UtilityScore> AeonRulerAI::evaluate_utilities(
    const AeonCivilization& self,
    const std::vector<AeonCivilization>& all_civs,
    const AeonHistory& history,
    const AeonCharacter* ruler,
    int year) const {

    std::vector<UtilityScore> raw_scores;

    // ── 1. QUELL_UNREST (Critical priority if unstable) ───────────────────────
    if (self.stability < 65.0f || self.unrest > 40.0f) {
        UtilityScore us;
        us.action_type = "QUELL_UNREST";
        us.target_civ  = -1;
        us.score       = score_quell_unrest(self, ruler, year, us.breakdown);
        if (us.score > 0.01f) raw_scores.push_back(us);
    }

    // ── 2. BUILD_INFRASTRUCTURE ───────────────────────────────────────────────
    {
        UtilityScore us;
        us.action_type = "BUILD_INFRASTRUCTURE";
        us.target_civ  = -1;
        us.score       = score_infrastructure(self, ruler, year, us.breakdown);
        if (us.score > 0.01f) raw_scores.push_back(us);
    }

    // ── 3. RESEARCH ───────────────────────────────────────────────────────────
    {
        UtilityScore us;
        us.action_type = "RESEARCH";
        us.target_civ  = -1;
        us.score       = score_research(self, ruler, year, us.breakdown);
        if (us.score > 0.01f) raw_scores.push_back(us);
    }

    // ── 4. BUILD_MILITARY ─────────────────────────────────────────────────────
    {
        UtilityScore us;
        us.action_type = "BUILD_MILITARY";
        us.target_civ  = -1;
        us.score       = score_military(self, all_civs, ruler, year, us.breakdown);
        if (us.score > 0.01f) raw_scores.push_back(us);
    }

    // ── 5. EXPAND ─────────────────────────────────────────────────────────────
    if (!self.at_war && self.territory_tiles < 400.0f) {
        UtilityScore us;
        us.action_type = "EXPAND";
        us.target_civ  = -1;
        us.score       = score_expand(self, ruler, year, us.breakdown);
        if (us.score > 0.01f) raw_scores.push_back(us);
    }

    // ── 6. NEGOTIATE_PEACE (if currently at war) ──────────────────────────────
    if (self.at_war && self.war_with_civ >= 0 && self.war_with_civ < (int)all_civs.size()) {
        const auto& other = all_civs[self.war_with_civ];
        if (other.is_alive > 0.0f) {
            UtilityScore us;
            us.action_type = "NEGOTIATE_PEACE";
            us.target_civ  = other.id;
            us.score       = score_peace(self, other, ruler, us.breakdown);
            if (us.score > 0.01f) raw_scores.push_back(us);
        }
    }

    // ── 7. Per-Target Actions (Diplomacy, War, Trade) ──────────────────────────
    for (const auto& other : all_civs) {
        if (other.id == self.id || other.is_alive <= 0.0f) continue;

        // DECLARE_WAR
        if (!self.at_war && !other.at_war) {
            UtilityScore us;
            us.action_type = "DECLARE_WAR";
            us.target_civ  = other.id;
            us.score       = score_war(self, other, history, ruler, year, us.breakdown);
            if (us.score > 0.05f) raw_scores.push_back(us);
        }

        // PROPOSE_TRADE
        if (!self.at_war && !other.at_war) {
            UtilityScore us;
            us.action_type = "PROPOSE_TRADE";
            us.target_civ  = other.id;
            us.score       = score_trade(self, other, history, ruler, year, us.breakdown);
            if (us.score > 0.05f) raw_scores.push_back(us);
        }

        // FORM_ALLIANCE
        if (!self.at_war && !other.at_war) {
            UtilityScore us;
            us.action_type = "FORM_ALLIANCE";
            us.target_civ  = other.id;
            us.score       = score_diplomacy(self, other, history, ruler, year, us.breakdown);
            if (us.score > 0.05f) raw_scores.push_back(us);
        }
    }

    // ── 8. Government Transitions & Crisis Response Actions ──────────────────
    {
        GovernmentTransitionEngine gov_engine;
        auto gov_actions = gov_engine.evaluate_government_actions(self, all_civs, ruler, year);
        for (const auto& ga : gov_actions) {
            if (ga.total_score > 0.05f) {
                UtilityScore us;
                us.action_type = ga.action_type;
                us.target_civ  = -1;
                us.score       = ga.total_score;
                us.breakdown   = ga.explanation;
                raw_scores.push_back(us);
            }
        }
    }

    // ── 9. HOLD baseline ──────────────────────────────────────────────────────
    {
        UtilityScore us;
        us.action_type = "HOLD";
        us.target_civ  = -1;
        us.score       = 0.05f;
        us.breakdown   = "baseline consolidation";
        raw_scores.push_back(us);
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Apply Diminishing Returns, Crisis Priorities, Consequence Lookahead & Gov Modifiers
    // ─────────────────────────────────────────────────────────────────────────
    std::vector<UtilityScore> scores;
    scores.reserve(raw_scores.size());

    for (auto us : raw_scores) {
        // 1. Action Repetition Diminishing Returns (Anti-Spam)
        int consecutive = self.history_memory.count_consecutive_action(us.action_type);
        float repetition_penalty = 0.0f;
        if (consecutive == 1) repetition_penalty = -0.10f;
        else if (consecutive == 2) repetition_penalty = -0.20f;
        else if (consecutive >= 3) repetition_penalty = -0.35f;

        // Exempt existential emergency responses from repetition penalties
        if (us.action_type == "BUILD_MILITARY" && (self.at_war || self.military_power < 300.0f)) {
            repetition_penalty = 0.0f;
        }
        if ((us.action_type == "QUELL_UNREST" || us.action_type == "EMERGENCY_LAWS" || us.action_type == "REFORM") && self.stability < 30.0f) {
            repetition_penalty = 0.0f;
        }

        // 2. Crisis State & Stability Priority Overrides
        float crisis_mod = 0.0f;
        if (self.stability < 20.0f) {
            if (us.action_type == "QUELL_UNREST" || us.action_type == "EMERGENCY_LAWS" || us.action_type == "REFORM" || us.action_type == "CONCESSIONS") {
                crisis_mod += 0.35f;
            } else if (us.action_type == "BUILD_INFRASTRUCTURE" || us.action_type == "RESEARCH" || us.action_type == "EXPAND") {
                crisis_mod -= 0.45f;
            }
        }
        if (self.stability < 10.0f) {
            // In an existential stability crisis, internal stabilization actions are paramount.
            // MILITARY_COUP is NOT exempt — a coup during freefall only makes things worse.
            if (us.action_type != "QUELL_UNREST" && us.action_type != "EMERGENCY_LAWS" && us.action_type != "NEGOTIATE_PEACE") {
                crisis_mod -= 0.60f;
            }
        }

        // 3. Centralized Government Type Behavioral Modifiers
        float gov_mod = 0.0f;
        if (self.government == GovForm::DEMOCRACY || self.government == GovForm::REPUBLIC) {
            if (us.action_type == "CALL_ELECTION" || us.action_type == "REFORM" || us.action_type == "FORM_ALLIANCE") gov_mod += 0.12f;
            if (us.action_type == "PROCLAIM_DICTATORSHIP") gov_mod -= 0.35f;
        } else if (self.government == GovForm::MILITARY_JUNTA) {
            if (us.action_type == "BUILD_MILITARY" || us.action_type == "EMERGENCY_LAWS") gov_mod += 0.15f;
            if (us.action_type == "CALL_ELECTION") gov_mod -= 0.40f;
            // A stable junta hard-blocks re-coup: only internal mutiny allows it
            if (us.action_type == "MILITARY_COUP" && self.military_loyalty >= 35.0f) {
                us.score = 0.0f; // Hard zero: already a junta with sufficient loyalty
            }
        } else if (self.government == GovForm::DICTATORSHIP) {
            if (us.action_type == "BUILD_MILITARY" || us.action_type == "QUELL_UNREST") gov_mod += 0.12f;
            if (us.action_type == "CALL_ELECTION" || us.action_type == "REFORM") gov_mod -= 0.30f;
        }

        // 4. Anti-oscillation: MILITARY_COUP cooldown in the AI decision layer
        //    If a coup was attempted recently (≤ 3 years ago), hard-suppress it
        //    regardless of current utility score. Prevents oscillation loops.
        if (us.action_type == "MILITARY_COUP") {
            auto coup_it = action_last_used_year_.find("MILITARY_COUP");
            if (coup_it != action_last_used_year_.end() && (year - coup_it->second) < 3) {
                us.score = 0.0f; // Hard suppress: coup was chosen < 3 years ago
            }
        }

        // 4. Consequence Lookahead net future value
        float lookahead = estimate_consequence_lookahead(self, us.action_type, us.target_civ, ruler);

        // Final score calculation strictly normalized in [0.0, 1.0]
        us.score = std::clamp(us.score + repetition_penalty + crisis_mod + gov_mod + lookahead, 0.0f, 1.0f);
        scores.push_back(us);
    }

    // Sort descending by normalized utility score
    std::sort(scores.begin(), scores.end(),
              [](const UtilityScore& a, const UtilityScore& b){ return a.score > b.score; });

    return scores;
}

// ─────────────────────────────────────────────────────────────────────────────
//  rule_based_decide  —  Pick best validated utility action
// ─────────────────────────────────────────────────────────────────────────────
AIDecision AeonRulerAI::rule_based_decide(const AeonCivilization& self,
                                           const std::vector<AeonCivilization>& all_civs,
                                           const AeonHistory& history,
                                           const AeonCharacter* ruler,
                                           int year) {
    update_strategic_goal(self, ruler);
    auto utils = evaluate_utilities(self, all_civs, history, ruler, year);
    last_utility_scores = utils;

    AIDecision dec;
    dec.action_type    = "HOLD";
    dec.declaration    = self.name + " consolidates internal affairs.";
    dec.duration_years = 1.0f;
    dec.source         = AIDecision::Source::RULE_BASED;
    dec.confidence     = 0.5f;
    dec.priority       = 0.5f;

    // Pick top scored action that passes strict validation
    for (const auto& best : utils) {
        AIDecision candidate;
        candidate.action_type   = best.action_type;
        candidate.target_civ    = best.target_civ;
        candidate.utility_score = best.score;
        candidate.confidence    = std::clamp(best.score, 0.1f, 0.99f);
        candidate.priority      = std::clamp(best.score + 0.1f, 0.1f, 0.99f);
        candidate.source        = AIDecision::Source::RULE_BASED;

        std::string tgt_name = (best.target_civ >= 0 && best.target_civ < (int)all_civs.size()) ?
                               all_civs[best.target_civ].name : "";

        if (best.action_type == "DECLARE_WAR") {
            candidate.declaration    = self.name + " declares war on " + tgt_name + "!";
            candidate.duration_years = 3.0f + self.aggression * 3.0f;
        } else if (best.action_type == "PROPOSE_TRADE") {
            candidate.declaration    = self.name + " proposes a bilateral 10-year trade pact with " + tgt_name + ".";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "FORM_ALLIANCE") {
            candidate.declaration    = self.name + " establishes a mutual defensive alliance with " + tgt_name + ".";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "BUILD_MILITARY") {
            candidate.declaration    = self.name + " invests in military expansion and defense.";
            candidate.duration_years = 2.0f;
        } else if (best.action_type == "RESEARCH") {
            candidate.declaration    = self.name + " accelerates scientific research.";
            candidate.duration_years = 2.0f;
        } else if (best.action_type == "EXPAND") {
            candidate.declaration    = self.name + " settles new frontier territories.";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "BUILD_INFRASTRUCTURE") {
            candidate.declaration    = self.name + " constructs roads, public works, and economic infrastructure.";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "QUELL_UNREST") {
            candidate.declaration    = self.name + " enacts emergency reforms to restore civil order and stability.";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "NEGOTIATE_PEACE") {
            candidate.declaration    = self.name + " offers peace terms to " + tgt_name + ".";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "MILITARY_COUP") {
            candidate.declaration    = "Armed forces launch a MILITARY COUP in " + self.name + " to restore order!";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "PROCLAIM_DICTATORSHIP") {
            candidate.declaration    = self.name + " proclaims a supreme DICTATORSHIP under absolute autocracy.";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "EMERGENCY_LAWS") {
            candidate.declaration    = self.name + " decrees emergency state powers and martial regulations.";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "REFORM") {
            candidate.declaration    = self.name + " passes sweeping constitutional and economic reforms.";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "CONCESSIONS") {
            candidate.declaration    = self.name + " grants concessions and subsidies to appease civil unrest.";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "CALL_ELECTION") {
            candidate.declaration    = self.name + " calls general elections to renew democratic legitimacy.";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "SUPPRESS_UNREST") {
            candidate.declaration    = self.name + " deploys security forces to suppress dissident unrest.";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "RESTORE_REPUBLIC" || best.action_type == "RESTORE_DEMOCRACY") {
            candidate.declaration    = self.name + " restores democratic constitutional governance!";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "TRANSITION_TO_EMPIRE") {
            candidate.declaration    = self.name + " crowns an imperial throne and proclaims an Empire!";
            candidate.duration_years = 1.0f;
        } else if (best.action_type == "TRANSITION_TO_THEOCRACY") {
            candidate.declaration    = self.name + " establishes ecclesiastical governance as a holy Theocracy.";
            candidate.duration_years = 1.0f;
        }

        std::string reason_rej;
        if (ActionValidator::validate(candidate, self, all_civs, year, war_cooldown_, trade_cooldown_, reason_rej)) {
            candidate.is_validated = true;
            dec = candidate;
            break;
        }
    }

    // Build reasoning log with comprehensive state metrics
    std::ostringstream reason;
    reason << "[UTILITY BREAKDOWN] (State: Stab:" << int(self.stability)
           << "% Unrest:" << int(self.unrest)
           << "% Inst:" << int(self.democratic_institution_strength)
           << "% MilLoyal:" << int(self.military_loyalty)
           << "% Legit:" << int(self.legitimacy) << "%)\n";
    int show = std::min(4, (int)utils.size());
    for (int i = 0; i < show; ++i) {
        const char* name = "";
        if (utils[i].target_civ >= 0 && utils[i].target_civ < (int)all_civs.size())
            name = all_civs[utils[i].target_civ].name.c_str();
        reason << "  " << i+1 << ". " << utils[i].action_type
               << (utils[i].target_civ >= 0 ? std::string(" vs ") + name : "")
               << " = " << std::fixed << std::setprecision(3) << utils[i].score
               << " (" << utils[i].breakdown << ")\n";
    }
    dec.reasoning = reason.str();

    // Update fatigue tracking
    action_last_used_year_[dec.action_type] = year;
    if (dec.action_type == "PROPOSE_TRADE" && dec.target_civ >= 0) trade_cooldown_[dec.target_civ] = year;
    if (dec.action_type == "DECLARE_WAR" && dec.target_civ >= 0) war_cooldown_[dec.target_civ] = year;

    return dec;
}

// ─────────────────────────────────────────────────────────────────────────────
//  LLM prompt builder
// ─────────────────────────────────────────────────────────────────────────────
static std::string build_ollama_prompt(const AeonCivilization& self,
                                        const std::vector<AeonCivilization>& all_civs,
                                        const std::vector<AeonRulerAI::MemoryEntry>& mem,
                                        const AeonCharacter* ruler,
                                        int year) {
    std::ostringstream ss;
    ss << "You are " << (ruler ? ruler->name : "the Sovereign")
       << ", ruler of " << self.name << " (ID:" << self.id << "), in year " << year << ".\n";
    if (ruler) {
        ss << "Ruler Trait: " << ruler_trait_name(ruler->trait)
           << " | Mil Skill: " << int(ruler->military_skill * 100) << "%"
           << " | Diplo Skill: " << int(ruler->diplomatic_skill * 100) << "%"
           << " | Econ Skill: " << int(ruler->economic_skill * 100) << "%\n";
    }
    ss << "\nYOUR NATION STATE:\n";
    ss << "  Population  : " << self.population.total / 1000 << "k\n";
    ss << "  Tech Era    : " << tech_era_name(self.tech.era)
       << " (" << int(self.tech.progress) << "% progress)\n";
    ss << "  GDP         : " << int(self.economy.gdp) << " gold\n";
    ss << "  Stability   : " << int(self.stability) << "%\n";
    ss << "  Army Size   : " << int(self.army_size) << " troops\n";
    ss << "  Territory   : " << int(self.territory_tiles) << " regions\n";
    ss << "  War Exhaust : " << int(self.war_exhaustion) << "%\n";
    ss << "  At War      : " << (self.at_war ? "YES" : "No") << "\n";
    ss << "  Active Trades: " << self.active_trade_agreements.size() << " agreements\n";
    ss << "  Goal        : " << self.primary_goal << "\n\n";

    ss << "NEIGHBORING REALMS:\n";
    for (const auto& other : all_civs) {
        if (other.id == self.id || other.is_alive <= 0.0f) continue;
        auto rel_it = self.relations.find(other.id);
        std::string rel = (rel_it != self.relations.end()) ?
            diplomacy_status_name(rel_it->second) : "Neutral";
        float trust = 0.0f, hatred = 0.0f;
        for (const auto& m : mem) {
            if (m.other_civ == other.id) { trust = m.trust; hatred = m.hatred; break; }
        }
        ss << "  - " << other.name << " (ID:" << other.id << ")"
           << " | Pop:" << other.population.total/1000 << "k"
           << " | Army:" << int(other.army_size)
           << " | Rel:" << rel
           << " | Trust:" << int(trust)
           << " | Hatred:" << int(hatred) << "\n";
    }

    ss << "\nINSTRUCTIONS: Choose ONE strategic action. Reply ONLY with valid JSON:\n";
    ss << "CRITICAL RULE: DO NOT TARGET YOUR OWN REALM (" << self.name << ").\n\n";
    ss << R"({
  "action": "RESEARCH",
  "target": "",
  "declaration": "Public declaration text.",
  "reasoning": "Strategic reason.",
  "confidence": 0.85,
  "priority": 0.90
})";
    ss << "\nValid actions: DECLARE_WAR, FORM_ALLIANCE, PROPOSE_TRADE, RESEARCH, BUILD_MILITARY, EXPAND, BUILD_INFRASTRUCTURE, QUELL_UNREST, NEGOTIATE_PEACE, HOLD\n";
    return ss.str();
}

static bool parse_ollama_response(const std::string& text,
                                   const std::vector<AeonCivilization>& all_civs,
                                   int self_id,
                                   AIDecision& out) {
    auto start = text.find('{');
    auto end   = text.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end <= start)
        return false;

    try {
        auto j = json::parse(text.substr(start, end - start + 1));

        out.action_type  = j.value("action",      "HOLD");
        out.declaration  = j.value("declaration", "");
        out.reasoning    = j.value("reasoning",   "LLM decision");
        out.confidence   = std::clamp(j.value("confidence", 0.5f), 0.0f, 1.0f);
        out.priority     = std::clamp(j.value("priority",   0.5f), 0.0f, 1.0f);
        out.source       = AIDecision::Source::LLM_LIVE;
        out.target_civ   = -1;

        std::string target_name = j.value("target", "");
        if (!target_name.empty()) {
            for (const auto& c : all_civs) {
                if (c.id != self_id && c.is_alive > 0.0f &&
                    (c.name == target_name || std::to_string(c.id) == target_name)) {
                    out.target_civ = c.id;
                    break;
                }
            }
        }

        if (out.target_civ == self_id) {
            out.target_civ = -1;
            out.action_type = "HOLD";
            return false;
        }

        out.duration_years = 2.0f;
        return true;
    } catch (...) {
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  decide  —  Main entry
// ─────────────────────────────────────────────────────────────────────────────
AIDecision AeonRulerAI::decide(const AeonCivilization& self,
                                const std::vector<AeonCivilization>& all_civs,
                                const AeonHistory& history,
                                const std::vector<AeonCharacter>& characters,
                                int year) {
    noise_seed_ = float((year * 31 + civ_id_ * 97) % 1000) / 1000.0f;
    decay_memory(1.0f);

    const AeonCharacter* ruler = nullptr;
    if (self.ruler_id >= 0) {
        for (const auto& ch : characters) {
            if (ch.id == self.ruler_id && ch.is_alive) { ruler = &ch; break; }
        }
    }

    if (!llm_state_) {
        llm_state_ = std::make_shared<LLMAsyncState>();
    }
    auto& st = *llm_state_;

    // ── 1. Check pending LLM async result ─────────────────────────────────────
    if (st.pending && st.future.valid()) {
        auto status = st.future.wait_for(std::chrono::milliseconds(0));
        if (status == std::future_status::ready) {
            std::string response = st.future.get();
            st.pending = false;
            AIDecision parsed;
            if (!response.empty() && parse_ollama_response(response, all_civs, self.id, parsed)) {
                std::string rej_reason;
                if (ActionValidator::validate(parsed, self, all_civs, year, war_cooldown_, trade_cooldown_, rej_reason)) {
                    parsed.is_validated      = true;
                    st.cached_decision       = parsed;
                    st.cached_target_id      = parsed.target_civ;
                    st.has_cache             = true;
                    std::cout << "[YEAR " << year << "] [LLM✓] " << self.name
                              << ": " << parsed.action_type
                              << " -- \"" << parsed.declaration << "\" (conf:"
                              << int(parsed.confidence * 100) << "%)" << std::endl;
                } else {
                    st.has_cache = false;
                }
            }
        }
    }

    // ── 2. Request new LLM decision if connected ──────────────────────────────
    if (model_name != "rule_based" && !model_name.empty() && !st.pending
        && (year - st.req_year) >= 3) {
        st.req_year = year;
        st.pending  = true;

        std::string prompt = build_ollama_prompt(self, all_civs, memory, ruler, year);
        std::string model  = model_name;

        st.future = std::async(std::launch::async, [prompt, model]() -> std::string {
            if (model.find("eldoria") != std::string::npos) {
                std::string key = AeonOpenRouter::get_eldoria_key();
                if (!key.empty()) return AeonOpenRouter::generate_content_custom(
                    prompt, key, "qwen/qwen-2.5-72b-instruct");
            }
            if (model.find("drakor") != std::string::npos) {
                std::string key = AeonOpenRouter::get_drakor_key();
                if (!key.empty()) return AeonOpenRouter::generate_content_custom(
                    prompt, key, "nousresearch/hermes-3-llama-3.1-405b");
            }
            if (model.find("openrouter") != std::string::npos) {
                if (AeonOpenRouter::is_configured())
                    return AeonOpenRouter::generate_content(prompt);
            }
            if (model.find("nemotron") != std::string::npos ||
                model.find("nvidia")   != std::string::npos) {
                if (AeonNemotron::is_configured())
                    return AeonNemotron::generate_content(prompt);
            }
            OllamaRequest req;
            req.model       = model;
            req.prompt      = prompt;
            req.max_tokens  = 250;
            req.temperature = 0.70f;
            return AeonOllama::generate_blocking(req);
        });
    }

    // ── 3. Select & Validate Action ───────────────────────────────────────────
    AIDecision dec;
    bool used_cache = false;

    if (st.has_cache && (year - st.req_year) <= 3) {
        std::string val_rej;
        if (ActionValidator::validate(st.cached_decision, self, all_civs, year, war_cooldown_, trade_cooldown_, val_rej)) {
            dec        = st.cached_decision;
            dec.source = AIDecision::Source::LLM_CACHE;
            used_cache = true;
        } else {
            st.has_cache = false;
        }
    }

    if (!used_cache) {
        dec = rule_based_decide(self, all_civs, history, ruler, year);
    }

    // ── 4. Logging: Deduplicate repetitive logs ───────────────────────────────
    if (dec.action_type != "HOLD") {
        const char* src_tag = "[rule]";
        if (dec.source == AIDecision::Source::LLM_CACHE) src_tag = "[LLM-cache]";
        if (dec.source == AIDecision::Source::LLM_LIVE)  src_tag = "[LLM-live]";

        std::cout << "[YEAR " << year << "] AI " << self.name
                  << " decided: " << dec.action_type << " " << src_tag
                  << " -- \"" << dec.declaration << "\"" << std::endl;

        if (ruler) {
            std::cout << "           Ruler: " << ruler->name
                      << " (" << ruler_trait_name(ruler->trait)
                      << ", Skill: " << std::fixed << std::setprecision(2) << ruler->get_effective_skill() << ")"
                      << " | Goal: " << civ_goal_name(strategic_goal) << std::endl;
        }

        // Print breakdown only if action changed or major event
        if (dec.action_type != last_action_chosen || dec.action_type == "DECLARE_WAR" || dec.action_type == "FORM_ALLIANCE") {
            std::cout << dec.reasoning;
        }
    }

    last_action_chosen = dec.action_type;
    return dec;
}

} // namespace Aeon
