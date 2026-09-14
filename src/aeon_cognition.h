#pragma once
#include "aeon_ruler_ai.h"
#include <optional>
#include <map>

namespace Aeon {
enum class Knowledge { KNOWN, BELIEVED, UNKNOWN };
struct IntelligenceEstimate {
    Knowledge knowledge = Knowledge::UNKNOWN;
    float value = 0, confidence = 0;
    int observed_year = 0;
};
struct PerceivedNation {
    int id = -1;
    std::string name;
    bool alive = false, at_war = false;
    DiplomacyStatus relation = DiplomacyStatus::NEUTRAL;
    IntelligenceEstimate army;
    float trust = 0, hatred = 0;
};
// Advisors receive only this value object. It contains no world references.
struct NationObservation {
    int id = -1, year = 0, enemy = -1;
    float stability = 0, unrest = 0, income = 0, army = 0, exhaustion = 0;
    float aggression = 0, science = 0, diplomacy = 0, risk_tolerance = 0;
    float competence = 0.5f;
    std::string ruler, ideology;
    std::vector<PerceivedNation> nations;
    std::map<std::string, int> last_actions;
};
struct CabinetProposal {
    AIDecision decision;
    std::string advisor, critique;
    float benefit = 0, risk = 0, score = 0;
    float fatigue_penalty = 0;
    std::string rejection_reason;
};
struct StrategyObjective { int horizon = 1; std::string objective; float progress = 0; };
struct NationCognition {
    NationObservation observation;
    std::vector<CabinetProposal> proposals;
    std::vector<StrategyObjective> objectives;
    std::vector<std::string> memory;
    AIDecision selected;
    std::string explanation;
};
NationObservation observe_nation(const AeonCivilization&, const std::vector<AeonCivilization>&,
    const std::vector<AeonCharacter>&, const AeonHistory&, const AeonRulerAI&, int year);
NationCognition deliberate(const NationObservation&, const NationCognition* previous = nullptr);
}
