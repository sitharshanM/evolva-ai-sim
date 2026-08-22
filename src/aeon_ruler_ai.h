#pragma once
// =============================================================================
//  aeon_ruler_ai.h  —  Authoritative AI Decision & Validation System
// =============================================================================
#include "aeon_config.h"
#include "aeon_civilization.h"
#include "aeon_history.h"
#include "aeon_character.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <future>
#include <atomic>
#include <mutex>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  Long-term strategic goals
// ─────────────────────────────────────────────────────────────────────────────
enum class CivGoal {
    SURVIVE,
    EXPAND_TERRITORY,
    BECOME_MILITARY_SUPERPOWER,
    BECOME_TECH_LEADER,
    BECOME_RICHEST,
    CONTROL_RESOURCES,
    MAINTAIN_PEACE,
    BUILD_EMPIRE,
    REGIONAL_HEGEMON,
    ESTABLISH_FEDERATION,
    DOMINATE_TRADE,
    CULTURAL_DOMINANCE
};

inline const char* civ_goal_name(CivGoal g) {
    switch (g) {
        case CivGoal::SURVIVE:                  return "Survive";
        case CivGoal::EXPAND_TERRITORY:         return "Expand Territory";
        case CivGoal::BECOME_MILITARY_SUPERPOWER: return "Military Superpower";
        case CivGoal::BECOME_TECH_LEADER:       return "Technology Leader";
        case CivGoal::BECOME_RICHEST:           return "Richest Civilization";
        case CivGoal::CONTROL_RESOURCES:        return "Control Key Resources";
        case CivGoal::MAINTAIN_PEACE:           return "Maintain Peace";
        case CivGoal::BUILD_EMPIRE:             return "Build Empire";
        case CivGoal::REGIONAL_HEGEMON:         return "Regional Hegemon";
        case CivGoal::ESTABLISH_FEDERATION:     return "Establish Federation";
        case CivGoal::DOMINATE_TRADE:           return "Dominate Trade";
        case CivGoal::CULTURAL_DOMINANCE:       return "Cultural Dominance";
    }
    return "Balanced";
}

// ─────────────────────────────────────────────────────────────────────────────
//  AIDecision  —  Structured decision produced by AI
// ─────────────────────────────────────────────────────────────────────────────
struct AIDecision {
    std::string action_type    = "HOLD";
    int         target_civ     = -1;   // Valid other civ ID or -1 for internal
    std::string declaration;           // Human-readable announcement
    std::string reasoning;             // Strategic breakdown

    float       confidence     = 0.5f; // 0.0 - 1.0
    float       priority       = 0.5f; // 0.0 - 1.0
    float       utility_score  = 0.0f; // Normalized utility score in [0.0, 1.0]
    float       duration_years = 1.0f;

    // Validation metadata
    bool        is_validated   = false;
    std::string validation_notes;

    enum class Source { RULE_BASED, LLM_CACHE, LLM_LIVE, FALLBACK } source = Source::RULE_BASED;
};

// ─────────────────────────────────────────────────────────────────────────────
//  UtilityScore  —  Named utility entry for one candidate action
// ─────────────────────────────────────────────────────────────────────────────
struct UtilityScore {
    std::string action_type;
    int         target_civ  = -1;
    float       score       = 0.0f; // Strictly in [0.0, 1.0]
    std::string breakdown;          // Human-readable breakdown for logging
};

// ─────────────────────────────────────────────────────────────────────────────
//  ActionValidator  —  Authoritative simulation validation barrier
// ─────────────────────────────────────────────────────────────────────────────
class ActionValidator {
public:
    static bool validate(const AIDecision& dec,
                         const AeonCivilization& self,
                         const std::vector<AeonCivilization>& all_civs,
                         int current_year,
                         const std::unordered_map<int,int>& war_cooldown,
                         const std::unordered_map<int,int>& trade_cooldown,
                         std::string& out_reason);
};

// ─────────────────────────────────────────────────────────────────────────────
//  AeonRulerAI  —  AI controller for one civilization
// ─────────────────────────────────────────────────────────────────────────────
class AeonRulerAI {
public:
    explicit AeonRulerAI(int civ_id) : civ_id_(civ_id) {}

    // Called once per year — returns an action for the simulation to execute
    AIDecision decide(const AeonCivilization& self,
                      const std::vector<AeonCivilization>& all_civs,
                      const AeonHistory& history,
                      const std::vector<AeonCharacter>& characters,
                      int year);

    // Imperfect information
    float perceived_army(int enemy_civ_id, float actual_army) const;

    // ── Memory of events — strictly clamped [-100..+100] ───────────────────
    struct MemoryEntry {
        int   other_civ = -1;
        float trust     = 0.0f;  // -100 to +100
        float fear      = 0.0f;  // 0 to 100
        float hatred    = 0.0f;  // 0 to 100
        int   year      = 0;
    };
    std::vector<MemoryEntry> memory;

    void add_memory(int other_civ, float trust, float fear, float hatred, int year);
    void decay_memory(float years_passed);

    // ── Long-term strategic goal ─────────────────────────────────────────────
    CivGoal strategic_goal = CivGoal::SURVIVE;
    void    update_strategic_goal(const AeonCivilization& self, const AeonCharacter* ruler);

    // ── Debug: last computed utility scores ──────────────────────────────────
    std::vector<UtilityScore> last_utility_scores;
    std::string               last_decision_log;
    std::string               last_action_chosen;

    // ── Action Fatigue & Cooldown Tracking ────────────────────────────────────
    std::unordered_map<std::string, int> action_last_used_year_;
    std::unordered_map<int, int> target_last_interacted_year_;
    std::unordered_map<int, int> trade_cooldown_;
    std::unordered_map<int, int> war_cooldown_;

    // ── LLM integration ──────────────────────────────────────────────────────
    std::string primary_goal;
    std::string hidden_goal;
    std::string model_name = "rule_based";

private:
    int    civ_id_    = -1;
    float  noise_seed_= 0.0f;

    // ── Multi-factor normalized utility scoring ──────────────────────────────
    std::vector<UtilityScore> evaluate_utilities(
        const AeonCivilization& self,
        const std::vector<AeonCivilization>& all_civs,
        const AeonHistory& history,
        const AeonCharacter* ruler,
        int year) const;

    float score_war(const AeonCivilization& self,
                    const AeonCivilization& target,
                    const AeonHistory& history,
                    const AeonCharacter* ruler,
                    int year,
                    std::string& out_breakdown) const;

    float score_military(const AeonCivilization& self,
                         const std::vector<AeonCivilization>& all_civs,
                         const AeonCharacter* ruler,
                         int year,
                         std::string& out_breakdown) const;

    float score_trade(const AeonCivilization& self,
                      const AeonCivilization& target,
                      const AeonHistory& history,
                      const AeonCharacter* ruler,
                      int year,
                      std::string& out_breakdown) const;

    float score_research(const AeonCivilization& self,
                         const AeonCharacter* ruler,
                         int year,
                         std::string& out_breakdown) const;

    float score_diplomacy(const AeonCivilization& self,
                          const AeonCivilization& target,
                          const AeonHistory& history,
                          const AeonCharacter* ruler,
                          int year,
                          std::string& out_breakdown) const;

    float score_expand(const AeonCivilization& self,
                       const AeonCharacter* ruler,
                       int year,
                       std::string& out_breakdown) const;

    float score_peace(const AeonCivilization& self,
                      const AeonCivilization& target,
                      const AeonCharacter* ruler,
                      std::string& out_breakdown) const;

    float score_infrastructure(const AeonCivilization& self,
                               const AeonCharacter* ruler,
                               int year,
                               std::string& out_breakdown) const;

    float score_quell_unrest(const AeonCivilization& self,
                             const AeonCharacter* ruler,
                             int year,
                             std::string& out_breakdown) const;

    float estimate_consequence_lookahead(const AeonCivilization& self,
                                         const std::string& action_type,
                                         int target_civ,
                                         const AeonCharacter* ruler) const;

    AIDecision rule_based_decide(const AeonCivilization& self,
                                  const std::vector<AeonCivilization>& all_civs,
                                  const AeonHistory& history,
                                  const AeonCharacter* ruler,
                                  int year);

    // ── Async LLM state ──────────────────────────────────────────────────────
    struct LLMAsyncState {
        std::future<std::string> future;
        AIDecision               cached_decision;
        bool                     has_cache = false;
        int                      req_year  = -999;
        int                      cached_target_id = -1;
        bool                     pending   = false;
    };
    std::shared_ptr<LLMAsyncState> llm_state_ = std::make_shared<LLMAsyncState>();
};

} // namespace Aeon
