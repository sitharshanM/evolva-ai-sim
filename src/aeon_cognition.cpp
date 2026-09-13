#include "aeon_cognition.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace Aeon {
NationObservation observe_nation(const AeonCivilization& self,
    const std::vector<AeonCivilization>& world, const std::vector<AeonCharacter>& characters,
    const AeonHistory& history, const AeonRulerAI& ai, int year) {
    NationObservation o;
    o.id = self.id; o.year = year; o.enemy = self.at_war ? self.war_with_civ : -1;
    o.stability = self.stability; o.unrest = self.unrest; o.income = self.economy.annual_income;
    o.army = self.army_size; o.exhaustion = self.war_exhaustion;
    o.aggression = self.aggression; o.science = self.science_pref; o.diplomacy = self.diplomacy_pref;
    o.risk_tolerance = std::clamp(self.aggression, 0.1f, 0.9f);
    o.ideology = gov_form_name(self.government);
    for (const auto& ch : characters) if (ch.id == self.ruler_id && ch.is_alive) {
        o.ruler = ch.name; o.competence = ch.competence; break;
    }
    for (const auto& entry : ai.action_last_used_year_) o.last_actions.insert(entry);
    for (const auto& target : world) {
        if (target.id == self.id || target.is_commons) continue;
        PerceivedNation p;
        p.id = target.id; p.name = target.name; p.alive = target.is_alive > 0;
        p.at_war = target.at_war;
        auto relation = self.relations.find(target.id);
        if (relation != self.relations.end()) p.relation = relation->second;
        if (const auto* m = history.relation_view(self.id, target.id)) {
            p.trust = m->trust; p.hatred = m->hatred;
        }
        // Public doctrine/terrain contact permits an estimate, never the true army count.
        const bool contact = p.relation != DiplomacyStatus::NEUTRAL ||
            std::abs(target.capital_x - self.capital_x) + std::abs(target.capital_y - self.capital_y) < 70;
        if (contact) {
            const float confidence = p.relation == DiplomacyStatus::ALLY ? 0.85f : 0.45f;
            const float bias = float(((year / 3 + self.id * 17 + target.id * 31) % 17) - 8) / 10.0f;
            p.army = {Knowledge::BELIEVED,
                std::max(0.0f, std::round(target.army_size * (1 + bias * (1-confidence)) / 500) * 500),
                confidence, year};
        }
        o.nations.push_back(p);
    }
    return o;
}

NationCognition deliberate(const NationObservation& o, const NationCognition* previous) {
    NationCognition c;
    c.observation = o;
    if (previous) c.memory = previous->memory;
    const std::string doctrine = o.aggression > 0.65f ? "Regional security through military strength" :
        o.science > 0.7f ? "Scientific and industrial leadership" : "Prosperity through stable institutions and trade";
    for (int horizon : {100, 20, 5, 1}) {
        c.objectives.push_back({horizon, horizon == 100 ? doctrine :
            horizon == 20 ? "Develop productive capacity and resilient alliances" :
            horizon == 5 ? "Improve domestic stability and national capabilities" :
            o.stability < 50 ? "Resolve domestic instability" : "Invest in sustainable growth",
            std::clamp(o.stability / 100.0f, 0.0f, 1.0f)});
    }
    auto propose = [&](std::string advisor, std::string action, int target,
                       float benefit, float risk, std::string critique) {
        CabinetProposal p;
        p.advisor = advisor; p.decision.action_type = action; p.decision.target_civ = target;
        p.benefit = benefit; p.risk = std::clamp(risk, 0.0f, 1.0f); p.critique = critique;
        float fatigue = 0;
        const auto used = o.last_actions.find(action);
        if (used != o.last_actions.end()) fatigue = std::max(0, 4 - (o.year-used->second)) * 0.06f;
        p.score = std::clamp(benefit - risk*(1-o.risk_tolerance)*0.7f - fatigue, 0.0f, 1.0f);
        p.decision.utility_score = p.score;
        p.decision.confidence = std::clamp(0.5f + o.competence*0.3f - risk*0.25f, 0.0f, 1.0f);
        p.decision.reasoning = advisor + ": " + critique;
        p.decision.declaration = action + " under the national strategy";
        c.proposals.push_back(p);
    };
    propose("Public affairs", "QUELL_UNREST", -1, (100-o.stability+o.unrest)/150, 0.05f, "Prioritize immediate stability when institutions are strained.");
    propose("Economy", "BUILD_INFRASTRUCTURE", -1, 0.48f, 0.1f, "Construction yields growth but competes with urgent needs.");
    propose("Science", "RESEARCH", -1, 0.3f+o.science*0.25f, 0.1f, "Research has delayed returns.");
    if (o.income >= 40) propose("Defense", "BUILD_MILITARY", -1, 0.25f+o.aggression*0.3f,
        40/std::max(40.0f,o.income), "Recruitment consumes 40 annual income and increases upkeep.");
    for (const auto& p : o.nations) {
        if (!p.alive) continue;
        if (o.enemy == p.id) {
            propose("Foreign affairs", "NEGOTIATE_PEACE", p.id, 0.5f+o.exhaustion/100,
                0.15f, "Peace ends the current conflict; future relations remain uncertain.");
            continue;
        }
        if (o.enemy >= 0 || p.at_war) continue;
        propose("Economy", "PROPOSE_TRADE", p.id, 0.4f+o.diplomacy*0.2f+p.trust/500,
            0.15f, "Trade is subject to existing agreements and cooldowns.");
        if (p.relation != DiplomacyStatus::ALLY) propose("Foreign affairs", "FORM_ALLIANCE", p.id,
            0.3f+o.diplomacy*0.2f+p.trust/300, 0.2f, "An alliance imposes long-term diplomatic commitments.");
        if (p.army.knowledge != Knowledge::UNKNOWN && p.relation != DiplomacyStatus::ALLY) {
            float ratio = o.army/std::max(500.0f,p.army.value);
            propose("Defense", "DECLARE_WAR", p.id, o.aggression*0.6f+std::min(0.25f,ratio*0.08f)+p.hatred/400,
                std::clamp(1/ std::max(0.1f,ratio) + (1-p.army.confidence)*0.4f, 0.0f,1.0f),
                "Enemy forces are an estimate. Critic penalizes uncertainty and force disadvantage.");
        }
    }
    propose("Cabinet", "HOLD", -1, 0.05f, 0, "Preserve the status quo if other actions are infeasible.");
    std::stable_sort(c.proposals.begin(), c.proposals.end(), [](const auto& a, const auto& b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.decision.action_type != b.decision.action_type) return a.decision.action_type < b.decision.action_type;
        return a.decision.target_civ < b.decision.target_civ;
    });
    c.selected = c.proposals.front().decision;
    c.explanation = doctrine + ". Ruler selects the highest feasible risk-adjusted cabinet proposal.";
    c.memory.push_back(std::to_string(o.year) + ": stability " + std::to_string(int(o.stability)) + "; " + c.selected.action_type);
    if (c.memory.size() > 100) c.memory.erase(c.memory.begin());
    return c;
}
}
