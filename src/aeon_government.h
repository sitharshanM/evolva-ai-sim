#pragma once
// =============================================================================
//  aeon_government.h  —  Dynamic Government, Transition & Power Engine
// =============================================================================
#include "aeon_config.h"
#include "aeon_world_types.h"
#include "aeon_character.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Aeon {

struct AeonCivilization;
class AeonEngine;

// ─────────────────────────────────────────────────────────────────────────────
//  Government Modifiers
// ─────────────────────────────────────────────────────────────────────────────
struct GovernmentModifiers {
    float stability_decay_resistance = 1.0f; // Multiplier on stability loss
    float unrest_decay_rate          = 1.0f; // Multiplier on unrest reduction
    float military_power_mult        = 1.0f; // Multiplier on military combat strength
    float economy_mult               = 1.0f; // Multiplier on GDP growth
    float tax_efficiency             = 1.0f; // Revenue extraction efficiency
    float research_mult              = 1.0f; // Multiplier on scientific research
    float diplomacy_bonus            = 0.0f; // Trust/diplomatic affinity modifier
    float base_legitimacy            = 75.0f;// Base institutional legitimacy
    float ruler_authority_mult       = 1.0f; // Power concentration multiplier
    float corruption_growth          = 0.0f; // Annual corruption drift
    float rebellion_risk_mult        = 1.0f; // Multiplier on civil war/rebellion odds
    float military_loyalty_bias      = 0.0f; // Drift on military loyalty
    float happiness_mult             = 1.0f; // Population happiness multiplier
};

// ─────────────────────────────────────────────────────────────────────────────
//  Government System Registry & Matrix
// ─────────────────────────────────────────────────────────────────────────────
class GovernmentSystem {
public:
    static GovernmentModifiers get_modifiers(GovForm form);
    static const char* get_form_description(GovForm form);
    static bool is_authoritarian(GovForm form);
    static bool is_democratic(GovForm form);
    static bool is_military_ruled(GovForm form);
};

// ─────────────────────────────────────────────────────────────────────────────
//  Government Transition Configuration (Configurable Weights & Thresholds)
// ─────────────────────────────────────────────────────────────────────────────
struct GovernmentTransitionConfig {
    float dictatorship_threshold        = 0.72f;
    float coup_threshold                = 0.70f; // ENFORCED gate: coup must score >= this
    float emergency_laws_threshold      = 0.60f;
    float revolution_threshold          = 0.75f;
    float reform_threshold              = 0.55f;

    // Coup cooldown (years after any coup before another is possible)
    int   coup_cooldown_years           = 20;

    // Utility Component Weights (Configurable)
    float stability_weight              = 0.28f;
    float unrest_weight                 = 0.24f;
    float military_power_weight         = 0.20f; // Power alone is insufficient
    float military_loyalty_weight       = 0.26f; // Higher weight: disloyalty is a key driver
    float military_discontent_weight    = 0.22f; // Rank-and-file unhappiness
    float ruler_authority_weight        = 0.18f;
    float legitimacy_weight             = 0.16f;
    float personality_weight            = 0.25f;
    float institution_weight            = 0.18f;
    float elite_support_weight          = 0.15f;
    float public_support_weight         = 0.15f;
    float external_threat_weight        = 0.14f;
    float randomness_factor             = 0.06f; // Reduced noise for more determinism
};

// ─────────────────────────────────────────────────────────────────────────────
//  Transition Action Score Breakdown (For Transparent Logging & AI Evaluation)
// ─────────────────────────────────────────────────────────────────────────────
struct TransitionScoreBreakdown {
    std::string action_type;
    float       total_score = 0.0f; // Normalized in [0.0, 1.0]
    std::string explanation;

    // Component factors
    float stability_factor    = 0.0f;
    float unrest_factor       = 0.0f;
    float military_factor     = 0.0f;
    float loyalty_factor      = 0.0f;
    float authority_factor    = 0.0f;
    float personality_factor  = 0.0f;
    float institution_factor  = 0.0f;
    float threat_factor       = 0.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
//  GovernmentTransitionEngine
// ─────────────────────────────────────────────────────────────────────────────
class GovernmentTransitionEngine {
public:
    GovernmentTransitionConfig config;

    GovernmentTransitionEngine() = default;

    // Evaluate all government and crisis response utilities for a civilization
    std::vector<TransitionScoreBreakdown> evaluate_government_actions(
        const AeonCivilization& self,
        const std::vector<AeonCivilization>& all_civs,
        const AeonCharacter* ruler,
        int current_year) const;

    // Specific transition calculators
    TransitionScoreBreakdown calculate_military_coup_score(
        const AeonCivilization& self,
        const std::vector<AeonCivilization>& all_civs,
        const AeonCharacter* ruler,
        int current_year) const;  // current_year required to fix cooldown math

    TransitionScoreBreakdown calculate_dictatorship_score(
        const AeonCivilization& self,
        const std::vector<AeonCivilization>& all_civs,
        const AeonCharacter* ruler,
        int current_year) const;  // current_year required to fix cooldown math

    TransitionScoreBreakdown calculate_emergency_laws_score(
        const AeonCivilization& self,
        const std::vector<AeonCivilization>& all_civs,
        const AeonCharacter* ruler) const;

    TransitionScoreBreakdown calculate_reform_score(
        const AeonCivilization& self,
        const AeonCharacter* ruler) const;

    TransitionScoreBreakdown calculate_concessions_score(
        const AeonCivilization& self,
        const AeonCharacter* ruler) const;

    TransitionScoreBreakdown calculate_call_election_score(
        const AeonCivilization& self,
        const AeonCharacter* ruler) const;

    TransitionScoreBreakdown calculate_suppress_unrest_score(
        const AeonCivilization& self,
        const AeonCharacter* ruler) const;

    TransitionScoreBreakdown calculate_restore_republic_score(
        const AeonCivilization& self,
        const AeonCharacter* ruler) const;

    TransitionScoreBreakdown calculate_theocracy_transition_score(
        const AeonCivilization& self,
        const AeonCharacter* ruler) const;

    TransitionScoreBreakdown calculate_empire_transition_score(
        const AeonCivilization& self,
        const AeonCharacter* ruler) const;

    // Apply the chosen government transition/crisis action
    bool apply_transition(
        const std::string& action_type,
        AeonCivilization& civ,
        std::vector<AeonCivilization>& all_civs,
        std::vector<AeonCharacter>& characters,
        AeonEngine& engine,
        int current_year);

    // Power consolidation (called yearly for authoritarian / junta / dictatorship realms)
    void tick_power_consolidation(
        AeonCivilization& civ,
        const AeonCharacter* ruler,
        int current_year,
        AeonEngine& engine);

    // Dictatorship / Junta Succession Crisis Handler
    void handle_authoritarian_succession(
        AeonCivilization& civ,
        const AeonCharacter& dead_ruler,
        std::vector<AeonCharacter>& characters,
        AeonEngine& engine,
        int current_year);
};

} // namespace Aeon
