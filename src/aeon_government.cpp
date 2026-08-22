// =============================================================================
//  aeon_government.cpp  —  Dynamic Government, Transition & Power Engine
// =============================================================================
#include "aeon_government.h"
#include "aeon_civilization.h"
#include "aeon_engine.h"
#include "aeon_random.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  Government System Modifiers Matrix
// ─────────────────────────────────────────────────────────────────────────────
GovernmentModifiers GovernmentSystem::get_modifiers(GovForm form) {
    GovernmentModifiers m;
    switch (form) {
        case GovForm::TRIBAL:
            m.stability_decay_resistance = 0.85f;
            m.unrest_decay_rate          = 1.10f;
            m.military_power_mult        = 1.10f;
            m.economy_mult               = 0.80f;
            m.tax_efficiency             = 0.70f;
            m.research_mult              = 0.75f;
            m.diplomacy_bonus            = -5.0f;
            m.base_legitimacy            = 60.0f;
            m.ruler_authority_mult       = 0.80f;
            m.corruption_growth          = 0.1f;
            m.rebellion_risk_mult        = 1.20f;
            m.military_loyalty_bias      = 0.0f;
            m.happiness_mult             = 1.00f;
            break;

        case GovForm::CHIEFDOM:
            m.stability_decay_resistance = 0.90f;
            m.unrest_decay_rate          = 1.05f;
            m.military_power_mult        = 1.05f;
            m.economy_mult               = 0.85f;
            m.tax_efficiency             = 0.80f;
            m.research_mult              = 0.80f;
            m.diplomacy_bonus            = -2.0f;
            m.base_legitimacy            = 65.0f;
            m.ruler_authority_mult       = 0.90f;
            m.corruption_growth          = 0.2f;
            m.rebellion_risk_mult        = 1.10f;
            m.military_loyalty_bias      = 0.0f;
            m.happiness_mult             = 0.95f;
            break;

        case GovForm::MONARCHY:
            m.stability_decay_resistance = 1.15f;
            m.unrest_decay_rate          = 0.95f;
            m.military_power_mult        = 1.05f;
            m.economy_mult               = 1.00f;
            m.tax_efficiency             = 1.00f;
            m.research_mult              = 0.95f;
            m.diplomacy_bonus            = 5.0f;
            m.base_legitimacy            = 80.0f;
            m.ruler_authority_mult       = 1.10f;
            m.corruption_growth          = 0.4f;
            m.rebellion_risk_mult        = 0.90f;
            m.military_loyalty_bias      = 0.2f;
            m.happiness_mult             = 0.95f;
            break;

        case GovForm::REPUBLIC:
            m.stability_decay_resistance = 1.00f;
            m.unrest_decay_rate          = 1.20f;
            m.military_power_mult        = 0.95f;
            m.economy_mult               = 1.15f;
            m.tax_efficiency             = 1.05f;
            m.research_mult              = 1.15f;
            m.diplomacy_bonus            = 10.0f;
            m.base_legitimacy            = 75.0f;
            m.ruler_authority_mult       = 0.80f;
            m.corruption_growth          = 0.3f;
            m.rebellion_risk_mult        = 0.80f;
            m.military_loyalty_bias      = -0.1f;
            m.happiness_mult             = 1.10f;
            break;

        case GovForm::DEMOCRACY:
            m.stability_decay_resistance = 0.90f;
            m.unrest_decay_rate          = 1.35f;
            m.military_power_mult        = 0.90f;
            m.economy_mult               = 1.25f;
            m.tax_efficiency             = 1.10f;
            m.research_mult              = 1.25f;
            m.diplomacy_bonus            = 15.0f;
            m.base_legitimacy            = 85.0f;
            m.ruler_authority_mult       = 0.70f;
            m.corruption_growth          = 0.1f;
            m.rebellion_risk_mult        = 0.65f;
            m.military_loyalty_bias      = -0.2f;
            m.happiness_mult             = 1.20f;
            break;

        case GovForm::OLIGARCHY:
            m.stability_decay_resistance = 1.05f;
            m.unrest_decay_rate          = 0.85f;
            m.military_power_mult        = 1.00f;
            m.economy_mult               = 1.20f;
            m.tax_efficiency             = 1.15f;
            m.research_mult              = 1.05f;
            m.diplomacy_bonus            = 2.0f;
            m.base_legitimacy            = 60.0f;
            m.ruler_authority_mult       = 0.90f;
            m.corruption_growth          = 0.8f;
            m.rebellion_risk_mult        = 1.15f;
            m.military_loyalty_bias      = 0.0f;
            m.happiness_mult             = 0.85f;
            break;

        case GovForm::MILITARY_JUNTA:
            m.stability_decay_resistance = 1.30f; // High martial order
            m.unrest_decay_rate          = 0.70f; // Underlying dissent simmers
            m.military_power_mult        = 1.35f; // Martial specialization
            m.economy_mult               = 0.85f; // Command economy inefficiencies
            m.tax_efficiency             = 1.10f; // Forceful extraction
            m.research_mult              = 0.85f;
            m.diplomacy_bonus            = -15.0f; // International isolation
            m.base_legitimacy            = 45.0f; // Coup regime illegitimacy
            m.ruler_authority_mult       = 1.35f;
            m.corruption_growth          = 0.6f;
            m.rebellion_risk_mult        = 1.40f; // High risk of counter-revolution
            m.military_loyalty_bias      = 0.8f;
            m.happiness_mult             = 0.75f;
            break;

        case GovForm::DICTATORSHIP:
            m.stability_decay_resistance = 1.35f; // Total state control
            m.unrest_decay_rate          = 0.60f; // Violent suppression
            m.military_power_mult        = 1.25f;
            m.economy_mult               = 0.80f; // Cronyism / sanctions
            m.tax_efficiency             = 1.20f; // Arbitrary confiscation
            m.research_mult              = 0.80f; // Censorship / brain drain
            m.diplomacy_bonus            = -25.0f; // Pariah state status
            m.base_legitimacy            = 40.0f; // Low institutional legitimacy
            m.ruler_authority_mult       = 1.60f; // Supreme autocrat
            m.corruption_growth          = 0.9f;
            m.rebellion_risk_mult        = 1.60f; // Violent overthrow risk
            m.military_loyalty_bias      = 0.5f;
            m.happiness_mult             = 0.70f;
            break;

        case GovForm::THEOCRACY:
            m.stability_decay_resistance = 1.20f;
            m.unrest_decay_rate          = 1.05f;
            m.military_power_mult        = 1.15f; // Holy zeal
            m.economy_mult               = 0.90f;
            m.tax_efficiency             = 1.05f; // Tithing
            m.research_mult              = 0.70f; // Dogmatic constraints
            m.diplomacy_bonus            = -5.0f;
            m.base_legitimacy            = 85.0f; // Divine mandate
            m.ruler_authority_mult       = 1.25f;
            m.corruption_growth          = 0.3f;
            m.rebellion_risk_mult        = 0.85f;
            m.military_loyalty_bias      = 0.3f;
            m.happiness_mult             = 0.90f;
            break;

        case GovForm::EMPIRE:
            m.stability_decay_resistance = 1.25f;
            m.unrest_decay_rate          = 0.85f;
            m.military_power_mult        = 1.30f;
            m.economy_mult               = 1.15f; // Tributary wealth
            m.tax_efficiency             = 1.15f;
            m.research_mult              = 1.05f;
            m.diplomacy_bonus            = 0.0f;
            m.base_legitimacy            = 80.0f;
            m.ruler_authority_mult       = 1.40f;
            m.corruption_growth          = 0.5f;
            m.rebellion_risk_mult        = 1.10f; // Imperial overstretch
            m.military_loyalty_bias      = 0.4f;
            m.happiness_mult             = 0.90f;
            break;

        case GovForm::FEDERATION:
            m.stability_decay_resistance = 0.95f;
            m.unrest_decay_rate          = 1.25f;
            m.military_power_mult        = 1.00f;
            m.economy_mult               = 1.20f;
            m.tax_efficiency             = 0.90f; // Decentralized
            m.research_mult              = 1.20f;
            m.diplomacy_bonus            = 20.0f;
            m.base_legitimacy            = 80.0f;
            m.ruler_authority_mult       = 0.75f;
            m.corruption_growth          = 0.2f;
            m.rebellion_risk_mult        = 0.70f;
            m.military_loyalty_bias      = -0.1f;
            m.happiness_mult             = 1.15f;
            break;

        case GovForm::TECHNOCRACY:
            m.stability_decay_resistance = 1.10f;
            m.unrest_decay_rate          = 1.10f;
            m.military_power_mult        = 1.10f;
            m.economy_mult               = 1.30f;
            m.tax_efficiency             = 1.25f; // Algorithmically optimized
            m.research_mult              = 1.60f; // Hyper-research focus
            m.diplomacy_bonus            = 10.0f;
            m.base_legitimacy            = 75.0f;
            m.ruler_authority_mult       = 1.10f;
            m.corruption_growth          = -0.2f; // Anti-corruption audits
            m.rebellion_risk_mult        = 0.80f;
            m.military_loyalty_bias      = 0.1f;
            m.happiness_mult             = 1.05f;
            break;

        case GovForm::AI_COUNCIL:
            m.stability_decay_resistance = 1.40f;
            m.unrest_decay_rate          = 1.30f;
            m.military_power_mult        = 1.25f;
            m.economy_mult               = 1.40f;
            m.tax_efficiency             = 1.30f;
            m.research_mult              = 1.80f;
            m.diplomacy_bonus            = 5.0f;
            m.base_legitimacy            = 70.0f;
            m.ruler_authority_mult       = 1.20f;
            m.corruption_growth          = -0.5f;
            m.rebellion_risk_mult        = 0.90f;
            m.military_loyalty_bias      = 0.0f;
            m.happiness_mult             = 1.00f;
            break;
    }
    return m;
}

const char* GovernmentSystem::get_form_description(GovForm form) {
    switch (form) {
        case GovForm::TRIBAL:         return "Loose confederation of autonomous clans and elders.";
        case GovForm::CHIEFDOM:       return "Centralized tribal hierarchy under a hereditary warlord.";
        case GovForm::MONARCHY:       return "Hereditary crown ruled by divine right and noble court.";
        case GovForm::REPUBLIC:       return "Representative senate elected by citizens and landholders.";
        case GovForm::DEMOCRACY:      return "Universal franchise with constitutional separation of powers.";
        case GovForm::OLIGARCHY:      return "Exclusive rule by wealthy patricians, merchants, and cartels.";
        case GovForm::MILITARY_JUNTA: return "Martial council of high-ranking generals seizing state power.";
        case GovForm::DICTATORSHIP:   return "Totalitarian autocracy under a supreme ruler with absolute command.";
        case GovForm::THEOCRACY:      return "Ecclesiastical rule governed by divine scripture and high priesthood.";
        case GovForm::EMPIRE:         return "Imperial hegemony uniting diverse realms under an exalted throne.";
        case GovForm::FEDERATION:     return "Union of partially self-governing states under a central pact.";
        case GovForm::TECHNOCRACY:    return "Decision-makers selected based on scientific and technical expertise.";
        case GovForm::AI_COUNCIL:     return "Synthetic algorithmic governance optimizing resource distribution.";
    }
    return "Unknown governance system.";
}

bool GovernmentSystem::is_authoritarian(GovForm form) {
    return (form == GovForm::DICTATORSHIP || form == GovForm::MILITARY_JUNTA ||
            form == GovForm::EMPIRE || form == GovForm::THEOCRACY);
}

bool GovernmentSystem::is_democratic(GovForm form) {
    return (form == GovForm::DEMOCRACY || form == GovForm::REPUBLIC || form == GovForm::FEDERATION);
}

bool GovernmentSystem::is_military_ruled(GovForm form) {
    return (form == GovForm::MILITARY_JUNTA || form == GovForm::DICTATORSHIP);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Transition Utility Calculators (Non-Deterministic, Factor-Weighted)
// ─────────────────────────────────────────────────────────────────────────────
TransitionScoreBreakdown GovernmentTransitionEngine::calculate_military_coup_score(
    const AeonCivilization& self,
    const std::vector<AeonCivilization>& all_civs,
    const AeonCharacter* ruler,
    int current_year) const {

    TransitionScoreBreakdown b;
    b.action_type = "MILITARY_COUP";

    // ── JUNTA SELF-COUP BLOCK ──────────────────────────────────────────────────
    // A military junta does not coup itself unless there is an internal mutiny
    bool is_junta_counter_coup = false;
    if (self.government == GovForm::MILITARY_JUNTA) {
        if (self.military_loyalty >= 35.0f) {
            b.explanation = "Hard block: Junta military loyalty sufficient — no counter-coup.";
            return b;
        }
        is_junta_counter_coup = true;
    }

    // ── MULTI-PILLAR HARD SUPPRESSION ─────────────────────────────────────────
    // Count intact stability pillars. A coup needs multiple pillars to be broken.
    // Military power alone is NEVER sufficient — other pillars must also be compromised.
    int intact_pillars = 0;
    if (self.stability          > 60.0f) intact_pillars++;
    if (self.military_loyalty   > 70.0f) intact_pillars++;  // Loyal army protects ruler
    if (self.democratic_institution_strength > 60.0f) intact_pillars++;
    if (self.legitimacy         > 70.0f) intact_pillars++;

    if (intact_pillars >= 3) {
        // 3+ pillars intact: coup is categorically impossible
        std::ostringstream ss;
        ss << "Hard suppression: " << intact_pillars
           << " stability pillars intact (stab=" << int(self.stability)
           << "% loyal=" << int(self.military_loyalty)
           << "% inst=" << int(self.democratic_institution_strength)
           << "% legit=" << int(self.legitimacy) << "%). Coup impossible.";
        b.explanation = ss.str();
        return b;
    }
    // 2 pillars intact: only partially suppressed (score scaled down later)
    bool soft_suppressed = (intact_pillars == 2);

    // ── PHYSICAL FORCE PREREQUISITE ───────────────────────────────────────────
    // Tiny armies cannot seize state power
    float army_adequacy = (self.army_size < 2500.0f)
        ? std::clamp(self.army_size / 2500.0f, 0.0f, 1.0f)
        : 1.0f;
    if (army_adequacy < 0.05f) {
        b.explanation = "Army too small to mount a coup (" + std::to_string(int(self.army_size)) + " troops).";
        return b;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  FACTOR 1: Instability Pressure — low stability enables coup
    // ─────────────────────────────────────────────────────────────────────────
    float instability      = std::clamp((65.0f - self.stability) / 65.0f, 0.0f, 1.0f);
    b.stability_factor     = instability * config.stability_weight;

    // ─────────────────────────────────────────────────────────────────────────
    //  FACTOR 2: Civil Unrest — widespread discontent gives coup a popular excuse
    // ─────────────────────────────────────────────────────────────────────────
    float unrest_pressure  = std::clamp(self.unrest / 100.0f, 0.0f, 1.0f);
    b.unrest_factor        = unrest_pressure * config.unrest_weight;

    // ─────────────────────────────────────────────────────────────────────────
    //  FACTOR 3: Military Power × Political Crisis INTERACTION
    //  KEY DESIGN: Military power alone is NEVER sufficient.
    //  A powerful, loyal military in a stable state contributes NEAR-ZERO coup pressure.
    //  Power only matters when combined with political crisis.
    // ─────────────────────────────────────────────────────────────────────────
    float mil_cap          = std::clamp((self.army_size - 1200.0f) / 18000.0f, 0.0f, 1.0f);
    float pol_crisis_level = (instability + unrest_pressure) * 0.5f;
    // Interaction: power * crisis — if crisis is 0, military power contributes 0
    float mil_power_pressure = mil_cap * std::max(pol_crisis_level, 0.05f) * config.military_power_weight;
    mil_power_pressure      *= army_adequacy; // Physical force floor applied here
    b.military_factor       = mil_power_pressure;

    // ─────────────────────────────────────────────────────────────────────────
    //  FACTOR 4: Military DISLOYALTY (CORRECTED direction)
    //  HIGH loyalty to ruler → REDUCES coup probability (loyal army defends ruler)
    //  LOW loyalty + HIGH discontent → INCREASES coup probability (army alienated)
    // ─────────────────────────────────────────────────────────────────────────
    float loyalty_normalized = std::clamp(self.military_loyalty / 100.0f, 0.0f, 1.0f);
    float disloyalty         = 1.0f - loyalty_normalized; // inverse: low loyalty = high pressure
    float discontent_norm    = std::clamp(self.military_discontent / 100.0f, 0.0f, 1.0f);
    float coup_support_norm  = std::clamp(self.coup_support / 100.0f, 0.0f, 1.0f);
    // Combined disloyalty pressure: low loyalty + active discontent + coup support
    float mil_disloyalty     = (disloyalty * 0.50f + discontent_norm * 0.30f + coup_support_norm * 0.20f);

    if (is_junta_counter_coup) {
        // Internal mutiny scenario: loyalty already checked < 35 above
        float mutiny = std::clamp((35.0f - self.military_loyalty) / 35.0f, 0.0f, 1.0f);
        b.loyalty_factor = mutiny * 0.45f;
    } else {
        b.loyalty_factor = mil_disloyalty * config.military_loyalty_weight;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  FACTOR 5: Legitimacy Crisis — ruler has no mandate
    //  Weak legitimacy creates power vacuums the military can fill
    // ─────────────────────────────────────────────────────────────────────────
    float legitimacy_crisis = std::clamp((60.0f - self.legitimacy) / 60.0f, 0.0f, 1.0f);
    // Legitimacy only matters in combination with some instability
    float legit_pressure    = legitimacy_crisis * std::max(instability, 0.1f) * config.legitimacy_weight;
    b.authority_factor      = legit_pressure;

    // ─────────────────────────────────────────────────────────────────────────
    //  FACTOR 6: Institutional Weakness — no checks to block a takeover
    // ─────────────────────────────────────────────────────────────────────────
    float inst_weakness     = std::clamp((100.0f - self.democratic_institution_strength) / 100.0f, 0.0f, 1.0f);
    b.institution_factor    = inst_weakness * config.institution_weight * 0.55f;

    // ─────────────────────────────────────────────────────────────────────────
    //  FACTOR 7: Ruler Traits — opportunistic/alienating rulers boost coup risk
    // ─────────────────────────────────────────────────────────────────────────
    float trait_mod = 0.0f;
    if (ruler) {
        if (ruler->trait == RulerTrait::MILITARIST || ruler->trait == RulerTrait::WARMONGER)
            trait_mod += 0.10f;  // Military-oriented rulers are coup-vulnerable/prone
        if (ruler->trait == RulerTrait::TYRANT || ruler->trait == RulerTrait::PARANOID)
            trait_mod += 0.10f;  // Tyranny alienates the military
        if (ruler->trait == RulerTrait::AUTHORITARIAN)
            trait_mod += 0.06f;  // Autocratic drift creates military rivalry
        if (ruler->trait == RulerTrait::DEMOCRATIC || ruler->trait == RulerTrait::IDEALIST)
            trait_mod -= 0.20f;  // Legitimately loved leaders suppress coup probability
        if (ruler->trait == RulerTrait::REFORMER)
            trait_mod -= 0.12f;  // Reformers retain military support through change
        if (ruler->trait == RulerTrait::DIPLOMAT)
            trait_mod -= 0.08f;
    }
    b.personality_factor = trait_mod;

    // ─────────────────────────────────────────────────────────────────────────
    //  FACTOR 8: Crisis Events — recent disasters, wars, rebellions increase window
    // ─────────────────────────────────────────────────────────────────────────
    float crisis_events = 0.0f;
    if (self.history_memory.recent_crisis_year > -900) {
        int yrs_crisis = current_year - self.history_memory.recent_crisis_year;
        if (yrs_crisis >= 0 && yrs_crisis < 15)
            crisis_events += 0.10f * std::clamp(1.0f - yrs_crisis / 15.0f, 0.0f, 1.0f);
    }
    if (self.at_war)           crisis_events += 0.06f;  // Active war creates opportunity
    if (self.war_exhaustion > 60.0f) crisis_events += 0.05f;  // Exhausted wars destabilize
    b.threat_factor = std::clamp(crisis_events, 0.0f, 0.20f);

    // ── ASSEMBLE COUP SCORE ───────────────────────────────────────────────────
    float raw = b.stability_factor + b.unrest_factor + b.military_factor +
                b.loyalty_factor   + b.authority_factor + b.institution_factor +
                b.personality_factor + b.threat_factor;

    // Soft suppression: 2 stability pillars intact — coup very unlikely
    if (soft_suppressed) raw *= 0.35f;

    // ── COUP COOLDOWN (uses actual current_year, not hardcoded) ───────────────
    if (self.recent_coup_year > -900) {
        int yrs_since = current_year - self.recent_coup_year;
        if (yrs_since >= 0 && yrs_since < config.coup_cooldown_years) {
            // Score scales from near-zero back to full over the cooldown period
            raw *= std::clamp((float)yrs_since / (float)config.coup_cooldown_years, 0.05f, 0.80f);
        }
    }

    // Controlled stochastic noise (reduced to prevent tiny fluctuations from flipping decisions)
    float noise = float((self.id * 37 + (int)self.stability * 11) % 100) / 100.0f;
    raw += (noise - 0.5f) * config.randomness_factor;

    b.total_score = std::clamp(raw, 0.0f, 1.0f);

    // ── EXPLAINABILITY REPORT ─────────────────────────────────────────────────
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "[POLITICAL ANALYSIS]\n";
    ss << "  Stability:           " << int(self.stability) << "%";
    if (self.stability > 60.0f) ss << " \u2714"; else ss << " \u26A0";
    ss << "\n  Unrest:              " << int(self.unrest) << "%\n";
    ss << "  Military Power:      " << int(self.army_size) << " troops\n";
    ss << "  Military Loyalty:    " << int(self.military_loyalty) << "%";
    if (self.military_loyalty > 70.0f) ss << " \u2714 (loyal — protects ruler)"; else ss << " \u26A0 (disloyal — coup risk)";
    ss << "\n  Military Discontent: " << int(self.military_discontent) << "%\n";
    ss << "  Institution Strength:" << int(self.democratic_institution_strength) << "%";
    if (self.democratic_institution_strength > 60.0f) ss << " \u2714"; else ss << " \u26A0";
    ss << "\n  Ruler Legitimacy:    " << int(self.legitimacy) << "%";
    if (self.legitimacy > 70.0f) ss << " \u2714"; else ss << " \u26A0";
    ss << "\n";
    ss << "  Intact Pillars:      " << intact_pillars << "/4\n";
    ss << "  \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";
    ss << "  Instability (+):    +" << b.stability_factor << "\n";
    ss << "  Unrest (+):         +" << b.unrest_factor << "\n";
    ss << "  Mil.Power*Crisis(+):+" << b.military_factor << "\n";
    ss << "  Disloyalty (+):     +" << b.loyalty_factor << "\n";
    ss << "  Legit.Crisis (+):   +" << b.authority_factor << "\n";
    ss << "  Inst.Weakness (+):  +" << b.institution_factor << "\n";
    ss << "  Ruler Trait:        " << (b.personality_factor >= 0 ? "+" : "") << b.personality_factor << "\n";
    ss << "  Events (+):         +" << b.threat_factor << "\n";
    if (soft_suppressed) ss << "  Soft-suppress:      x0.35 (2 pillars intact)\n";
    ss << "  \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";
    ss << "  Coup Score:          " << b.total_score << "  (Threshold: " << config.coup_threshold << ")\n";
    if (b.total_score >= config.coup_threshold) {
        ss << "  >> MILITARY_COUP threshold MET. Enters candidate pool.\n";
    } else {
        ss << "  >> MILITARY_COUP REJECTED (below threshold).\n";
        ss << "     Reason: ";
        if (self.military_loyalty > 70.0f)
            ss << "Military remains loyal to the ruler. ";
        if (self.stability > 60.0f)
            ss << "Political stability is sufficient. ";
        if (self.democratic_institution_strength > 60.0f)
            ss << "Institutions block a takeover. ";
        if (self.legitimacy > 70.0f)
            ss << "Ruler has strong legitimacy.";
        ss << "\n";
    }
    b.explanation = ss.str();

    return b;
}

TransitionScoreBreakdown GovernmentTransitionEngine::calculate_dictatorship_score(
    const AeonCivilization& self,
    const std::vector<AeonCivilization>& all_civs,
    const AeonCharacter* ruler,
    int current_year) const {

    TransitionScoreBreakdown b;
    b.action_type = "PROCLAIM_DICTATORSHIP";

    if (self.government == GovForm::DICTATORSHIP) return b;

    // Prerequisite for Military Junta -> Dictatorship:
    // Requires strong ruler authority, high military control, weak institutions, and minimum 5 years in power
    if (self.government == GovForm::MILITARY_JUNTA) {
        if (self.ruler_authority < 65.0f || self.military_loyalty < 70.0f ||
            self.democratic_institution_strength > 40.0f || self.years_current_gov < 5) {
            b.explanation = "Junta autocrat has not sufficiently consolidated power or duration.";
            return b;
        }
    }

    // Ruler Authority must be very high to seize total autocratic power
    float high_auth = std::clamp(self.ruler_authority / 100.0f, 0.0f, 1.0f);
    b.authority_factor = high_auth * config.ruler_authority_weight;

    float low_stab = std::clamp((50.0f - self.stability) / 50.0f, 0.0f, 1.0f);
    b.stability_factor = low_stab * config.stability_weight;

    float high_unrest = std::clamp(self.unrest / 100.0f, 0.0f, 1.0f);
    b.unrest_factor = high_unrest * config.unrest_weight;

    float mil_loyalty = std::clamp(self.military_loyalty / 100.0f, 0.0f, 1.0f);
    b.loyalty_factor = mil_loyalty * config.military_loyalty_weight;

    float trait_mod = 0.0f;
    if (ruler) {
        if (ruler->trait == RulerTrait::AUTHORITARIAN) trait_mod += 0.22f;
        if (ruler->trait == RulerTrait::TYRANT)        trait_mod += 0.20f;
        if (ruler->trait == RulerTrait::PARANOID)      trait_mod += 0.14f;
        if (ruler->trait == RulerTrait::DEMOCRATIC)    trait_mod -= 0.40f;
        if (ruler->trait == RulerTrait::REFORMER)      trait_mod -= 0.20f;
    }
    b.personality_factor = trait_mod;

    float inst_strength = std::clamp(self.democratic_institution_strength / 100.0f, 0.0f, 1.0f);
    b.institution_factor = -(inst_strength * (config.institution_weight * 1.4f));

    float raw = b.stability_factor + b.unrest_factor + b.authority_factor +
                b.loyalty_factor + b.personality_factor + b.institution_factor;

    // Transition Cooldown (10 Years) — now uses current_year parameter
    if (self.last_transition_year > -900) {
        int yrs = current_year - self.last_transition_year;
        if (yrs >= 0 && yrs < 10) {
            raw *= std::clamp((float)yrs / 10.0f, 0.10f, 0.85f);
        }
    }

    b.total_score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "Factors: +ruler_auth(+" << std::fixed << std::setprecision(2) << b.authority_factor
       << ") +loyalty(+" << b.loyalty_factor << ") +trait(" << (trait_mod >= 0 ? "+" : "") << trait_mod
       << ") +crisis(+" << (b.stability_factor + b.unrest_factor) << ") -inst(" << b.institution_factor << ")";
    b.explanation = ss.str();

    return b;
}

TransitionScoreBreakdown GovernmentTransitionEngine::calculate_emergency_laws_score(
    const AeonCivilization& self,
    const std::vector<AeonCivilization>& all_civs,
    const AeonCharacter* ruler) const {

    TransitionScoreBreakdown b;
    b.action_type = "EMERGENCY_LAWS";

    if (self.stability >= 60.0f && self.unrest < 40.0f && !self.at_war) return b;

    float crisis = std::clamp((70.0f - self.stability) / 70.0f, 0.0f, 1.0f);
    b.stability_factor = crisis * 0.35f;
    b.unrest_factor    = std::clamp(self.unrest / 100.0f, 0.0f, 1.0f) * 0.25f;

    float trait_mod = 0.0f;
    if (ruler) {
        if (ruler->trait == RulerTrait::PRAGMATIST || ruler->trait == RulerTrait::AUTHORITARIAN) trait_mod = 0.15f;
        if (ruler->trait == RulerTrait::WARMONGER || ruler->trait == RulerTrait::MILITARIST)     trait_mod = 0.12f;
    }
    b.personality_factor = trait_mod;

    float threat = self.at_war ? 0.20f : 0.0f;
    b.threat_factor = threat;

    float raw = b.stability_factor + b.unrest_factor + b.personality_factor + b.threat_factor + 0.10f;
    b.total_score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "Factors: +crisis(+" << std::fixed << std::setprecision(2) << (b.stability_factor + b.unrest_factor)
       << ") +threat(+" << b.threat_factor << ") +trait(" << trait_mod << ")";
    b.explanation = ss.str();

    return b;
}

TransitionScoreBreakdown GovernmentTransitionEngine::calculate_reform_score(
    const AeonCivilization& self,
    const AeonCharacter* ruler) const {

    TransitionScoreBreakdown b;
    b.action_type = "REFORM";

    float need = std::clamp((80.0f - self.stability) / 80.0f, 0.0f, 1.0f);
    b.stability_factor = need * 0.35f;

    float ruler_comp = ruler ? ruler->competence : 0.5f;
    b.authority_factor = ruler_comp * 0.25f;

    float trait_mod = 0.0f;
    if (ruler) {
        if (ruler->trait == RulerTrait::REFORMER)   trait_mod += 0.30f;
        if (ruler->trait == RulerTrait::DEMOCRATIC) trait_mod += 0.20f;
        if (ruler->trait == RulerTrait::TECHNOCRAT) trait_mod += 0.20f;
        if (ruler->trait == RulerTrait::TYRANT)     trait_mod -= 0.30f;
    }
    b.personality_factor = trait_mod;

    float inst = std::clamp(self.democratic_institution_strength / 100.0f, 0.0f, 1.0f);
    b.institution_factor = inst * 0.15f;

    float raw = b.stability_factor + b.authority_factor + b.personality_factor + b.institution_factor;
    b.total_score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "Factors: +reform_need(+" << std::fixed << std::setprecision(2) << b.stability_factor
       << ") +ruler_competence(+" << b.authority_factor << ") +trait(" << (trait_mod >= 0 ? "+" : "") << trait_mod
       << ") +institutions(+" << b.institution_factor << ")";
    b.explanation = ss.str();

    return b;
}

TransitionScoreBreakdown GovernmentTransitionEngine::calculate_concessions_score(
    const AeonCivilization& self,
    const AeonCharacter* ruler) const {

    TransitionScoreBreakdown b;
    b.action_type = "CONCESSIONS";

    if (self.unrest < 30.0f) return b;

    float unrest_press = std::clamp(self.unrest / 100.0f, 0.0f, 1.0f);
    b.unrest_factor = unrest_press * 0.40f;

    float trait_mod = 0.0f;
    if (ruler) {
        if (ruler->trait == RulerTrait::DIPLOMAT || ruler->trait == RulerTrait::DEMOCRATIC) trait_mod = 0.25f;
        if (ruler->trait == RulerTrait::TYRANT || ruler->trait == RulerTrait::AUTHORITARIAN) trait_mod = -0.35f;
    }
    b.personality_factor = trait_mod;

    float raw = b.unrest_factor + b.personality_factor + (self.stability < 30.0f ? 0.20f : 0.05f);
    b.total_score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "Factors: +unrest_pressure(+" << std::fixed << std::setprecision(2) << b.unrest_factor
       << ") +trait(" << (trait_mod >= 0 ? "+" : "") << trait_mod << ")";
    b.explanation = ss.str();

    return b;
}

TransitionScoreBreakdown GovernmentTransitionEngine::calculate_call_election_score(
    const AeonCivilization& self,
    const AeonCharacter* ruler) const {

    TransitionScoreBreakdown b;
    b.action_type = "CALL_ELECTION";

    // Valid in Republics, Democracies, Federations, or reforming Autocracies
    bool is_dem = GovernmentSystem::is_democratic(self.government);
    if (!is_dem && (self.democratic_institution_strength < 40.0f || self.government == GovForm::DICTATORSHIP))
        return b;

    float public_demand = std::clamp((100.0f - self.legitimacy + self.unrest) / 200.0f, 0.0f, 1.0f);
    b.unrest_factor = public_demand * 0.35f;

    float inst = std::clamp(self.democratic_institution_strength / 100.0f, 0.0f, 1.0f);
    b.institution_factor = inst * 0.30f;

    float trait_mod = (ruler && ruler->trait == RulerTrait::DEMOCRATIC) ? 0.25f :
                      (ruler && (ruler->trait == RulerTrait::AUTHORITARIAN || ruler->trait == RulerTrait::TYRANT)) ? -0.40f : 0.0f;
    b.personality_factor = trait_mod;

    float raw = b.unrest_factor + b.institution_factor + b.personality_factor + (is_dem ? 0.15f : 0.0f);
    b.total_score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "Factors: +election_demand(+" << std::fixed << std::setprecision(2) << b.unrest_factor
       << ") +institutions(+" << b.institution_factor << ") +trait(" << (trait_mod >= 0 ? "+" : "") << trait_mod << ")";
    b.explanation = ss.str();

    return b;
}

TransitionScoreBreakdown GovernmentTransitionEngine::calculate_suppress_unrest_score(
    const AeonCivilization& self,
    const AeonCharacter* ruler) const {

    TransitionScoreBreakdown b;
    b.action_type = "SUPPRESS_UNREST";

    if (self.unrest < 25.0f) return b;

    float unrest_press = std::clamp(self.unrest / 100.0f, 0.0f, 1.0f);
    b.unrest_factor = unrest_press * 0.35f;

    float mil_force = std::clamp(self.army_size / 10000.0f, 0.0f, 1.0f);
    b.military_factor = mil_force * 0.25f;

    float trait_mod = 0.0f;
    if (ruler) {
        if (ruler->trait == RulerTrait::TYRANT || ruler->trait == RulerTrait::AUTHORITARIAN) trait_mod = 0.25f;
        if (ruler->trait == RulerTrait::DEMOCRATIC || ruler->trait == RulerTrait::IDEALIST)  trait_mod = -0.30f;
    }
    b.personality_factor = trait_mod;

    float raw = b.unrest_factor + b.military_factor + b.personality_factor + (GovernmentSystem::is_authoritarian(self.government) ? 0.15f : 0.0f);
    b.total_score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "Factors: +unrest(+" << std::fixed << std::setprecision(2) << b.unrest_factor
       << ") +military_readiness(+" << b.military_factor << ") +trait(" << (trait_mod >= 0 ? "+" : "") << trait_mod << ")";
    b.explanation = ss.str();

    return b;
}

TransitionScoreBreakdown GovernmentTransitionEngine::calculate_restore_republic_score(
    const AeonCivilization& self,
    const AeonCharacter* ruler) const {

    TransitionScoreBreakdown b;
    b.action_type = "RESTORE_REPUBLIC";

    if (!GovernmentSystem::is_authoritarian(self.government)) return b;

    float opposition = std::clamp(self.opposition_strength / 100.0f, 0.0f, 1.0f);
    b.unrest_factor = opposition * 0.40f;

    float low_auth = std::clamp((60.0f - self.ruler_authority) / 60.0f, 0.0f, 1.0f);
    b.authority_factor = low_auth * 0.25f;

    float trait_mod = (ruler && ruler->trait == RulerTrait::DEMOCRATIC) ? 0.35f :
                      (ruler && ruler->trait == RulerTrait::REFORMER) ? 0.20f :
                      (ruler && (ruler->trait == RulerTrait::TYRANT || ruler->trait == RulerTrait::AUTHORITARIAN)) ? -0.45f : 0.0f;
    b.personality_factor = trait_mod;

    float raw = b.unrest_factor + b.authority_factor + b.personality_factor;
    b.total_score = std::clamp(raw, 0.0f, 1.0f);

    std::ostringstream ss;
    ss << "Factors: +democratic_opposition(+" << std::fixed << std::setprecision(2) << b.unrest_factor
       << ") +weak_autocracy(+" << b.authority_factor << ") +trait(" << (trait_mod >= 0 ? "+" : "") << trait_mod << ")";
    b.explanation = ss.str();

    return b;
}

TransitionScoreBreakdown GovernmentTransitionEngine::calculate_theocracy_transition_score(
    const AeonCivilization& self,
    const AeonCharacter* ruler) const {

    TransitionScoreBreakdown b;
    b.action_type = "TRANSITION_TO_THEOCRACY";

    if (self.government == GovForm::THEOCRACY) return b;
    if (self.religiosity < 0.65f) return b;

    float zeal = std::clamp(self.religiosity, 0.0f, 1.0f);
    float raw = (zeal * 0.50f) + (self.stability < 40.0f ? 0.20f : 0.0f);
    if (ruler && ruler->trait == RulerTrait::IDEALIST) raw += 0.20f;

    b.total_score = std::clamp(raw, 0.0f, 1.0f);
    b.explanation = "Factors: +religious_zeal(+" + std::to_string(int(zeal * 100)) + "%)";
    return b;
}

TransitionScoreBreakdown GovernmentTransitionEngine::calculate_empire_transition_score(
    const AeonCivilization& self,
    const AeonCharacter* ruler) const {

    TransitionScoreBreakdown b;
    b.action_type = "TRANSITION_TO_EMPIRE";

    if (self.government == GovForm::EMPIRE) return b;
    if (self.territory_tiles < 120.0f || self.army_size < 15000.0f) return b;

    float territorial_hegemony = std::clamp(self.territory_tiles / 200.0f, 0.0f, 1.0f);
    float auth = std::clamp(self.ruler_authority / 100.0f, 0.0f, 1.0f);
    float trait_mod = (ruler && (ruler->trait == RulerTrait::EXPANSIONIST || ruler->trait == RulerTrait::WARMONGER)) ? 0.25f : 0.0f;

    float raw = (territorial_hegemony * 0.40f) + (auth * 0.35f) + trait_mod;
    b.total_score = std::clamp(raw, 0.0f, 1.0f);
    b.explanation = "Factors: +imperial_territory(+" + std::to_string(int(self.territory_tiles)) + " tiles) +authority";
    return b;
}

// ─────────────────────────────────────────────────────────────────────────────
//  evaluate_government_actions  —  Collects all scored crisis/government choices
// ─────────────────────────────────────────────────────────────────────────────
std::vector<TransitionScoreBreakdown> GovernmentTransitionEngine::evaluate_government_actions(
    const AeonCivilization& self,
    const std::vector<AeonCivilization>& all_civs,
    const AeonCharacter* ruler,
    int current_year) const {

    std::vector<TransitionScoreBreakdown> actions;

    // 1. Military Coup — THRESHOLD GATE ENFORCED
    //    Score must reach config.coup_threshold (default 0.70) to enter candidate pool.
    //    Below threshold: logged and rejected, never competes with other actions.
    {
        auto coup = calculate_military_coup_score(self, all_civs, ruler, current_year);
        if (coup.total_score >= config.coup_threshold) {
            actions.push_back(coup);
        } else if (coup.total_score > 0.05f) {
            // Print rejection notice so AI decision log shows why coup was denied
            std::cout << "  [GOV] MILITARY_COUP rejected: score=" << std::fixed << std::setprecision(3)
                      << coup.total_score << " < threshold=" << config.coup_threshold << "\n";
        }
    }

    // 2. Dictatorship (passes current_year for correct cooldown)
    auto dict = calculate_dictatorship_score(self, all_civs, ruler, current_year);
    if (dict.total_score > 0.05f) actions.push_back(dict);

    // 3. Emergency Laws
    auto emerg = calculate_emergency_laws_score(self, all_civs, ruler);
    if (emerg.total_score > 0.05f) actions.push_back(emerg);

    // 4. Reform
    auto ref = calculate_reform_score(self, ruler);
    if (ref.total_score > 0.05f) actions.push_back(ref);

    // 5. Concessions
    auto conc = calculate_concessions_score(self, ruler);
    if (conc.total_score > 0.05f) actions.push_back(conc);

    // 6. Call Election
    auto elec = calculate_call_election_score(self, ruler);
    if (elec.total_score > 0.05f) actions.push_back(elec);

    // 7. Suppress Unrest
    auto supp = calculate_suppress_unrest_score(self, ruler);
    if (supp.total_score > 0.05f) actions.push_back(supp);

    // 8. Restore Republic / Democracy
    auto rest = calculate_restore_republic_score(self, ruler);
    if (rest.total_score > 0.05f) actions.push_back(rest);

    // 9. Empire & Theocracy
    auto emp = calculate_empire_transition_score(self, ruler);
    if (emp.total_score > 0.05f) actions.push_back(emp);

    auto theo = calculate_theocracy_transition_score(self, ruler);
    if (theo.total_score > 0.05f) actions.push_back(theo);

    // Sort descending
    std::sort(actions.begin(), actions.end(),
              [](const TransitionScoreBreakdown& a, const TransitionScoreBreakdown& b){
                  return a.total_score > b.total_score;
              });

    return actions;
}

// ─────────────────────────────────────────────────────────────────────────────
//  apply_transition  —  State Machine Execution
// ─────────────────────────────────────────────────────────────────────────────
bool GovernmentTransitionEngine::apply_transition(
    const std::string& action_type,
    AeonCivilization& civ,
    std::vector<AeonCivilization>& all_civs,
    std::vector<AeonCharacter>& characters,
    AeonEngine& engine,
    int current_year) {

    GovForm old_gov = civ.government;

    if (action_type == "MILITARY_COUP") {
        civ.government = GovForm::MILITARY_JUNTA;

        // ── COUP SHOCK: Realistic short-term destabilization ─────────────────
        // A coup is NOT an instant fix. It creates unrest, fear, and uncertainty.
        civ.stability  = std::clamp(civ.stability  - 10.0f,  5.0f, 100.0f); // Coup destabilizes
        civ.unrest     = std::clamp(civ.unrest     + 20.0f,  0.0f, 100.0f); // Coup shock
        civ.ruler_authority = 75.0f;  // Junta starts with partial control, must consolidate
        civ.military_loyalty = 85.0f; // Military now loyal to the new regime
        civ.legitimacy       = 35.0f; // Very low initial coup legitimacy
        civ.opposition_strength = std::clamp(civ.opposition_strength + 30.0f, 0.0f, 100.0f);
        civ.democratic_institution_strength = std::max(5.0f, civ.democratic_institution_strength - 45.0f);
        civ.military_discontent = 15.0f; // Residual discontent from the coup process
        civ.coup_support        = 0.0f;  // Reset: support has been spent
        civ.crisis_state        = CrisisState::MILITARY_REGIME;
        civ.recent_coup_year     = current_year;
        civ.coup_attempt_year    = current_year;
        civ.last_transition_year = current_year;
        civ.under_martial_law    = true;

        // Replace ruler with General
        std::string prev_ruler_name = "the Head of State";
        if (civ.ruler_id >= 0) {
            for (auto& ch : characters) {
                if (ch.id == civ.ruler_id) {
                    ch.is_ruler = false;
                    prev_ruler_name = ch.name;
                    break;
                }
            }
        }

        static const char* general_names[] = {
            "General Kael I", "Field Marshal Thorne", "General Darius II",
            "General Vaelor", "Admiral Cassian", "General Alaric"
        };
        int g_idx = (current_year + civ.id) % 6;
        auto junta_ruler = engine.make_ruler(engine.next_char_id++, general_names[g_idx], civ.id, current_year - 42);
        junta_ruler.title = "Chairman of the Military Council";
        junta_ruler.trait = RulerTrait::MILITARIST;
        junta_ruler.military_skill = 0.90f;
        junta_ruler.competence     = 0.70f;
        civ.ruler_id = junta_ruler.id;
        civ.character_ids.push_back(junta_ruler.id);
        characters.push_back(junta_ruler);

        // International fallout: Democracies and Republics distrust the coup regime
        for (auto& other : all_civs) {
            if (other.id != civ.id && GovernmentSystem::is_democratic(other.government)) {
                civ.bilateral_relations[other.id].trust = std::clamp(civ.bilateral_relations[other.id].trust - 25.0f, -100.0f, 100.0f);
                other.bilateral_relations[civ.id].trust = std::clamp(other.bilateral_relations[civ.id].trust - 25.0f, -100.0f, 100.0f);
            }
        }

        // Chronicle entry with narrative
        std::string chronicle_desc =
            junta_ruler.name + " overthrows " + prev_ruler_name +
            " in a military coup. The realm descends into martial order under the Military Council."
            " Institutions collapse. Unrest surges. Opposition movements go underground.";
        civ.history_memory.add_event("MILITARY_COUP", current_year, chronicle_desc, -1);
        civ.history_memory.recent_crisis_year = current_year;

        engine.history.record(current_year, 1, "GOVERNMENT",
            "Military Coup in " + civ.name,
            civ.name + " experiences a military coup. " + junta_ruler.name + " establishes a Military Junta.",
            civ.id, -1, {"instability_crisis", "military_takeover"}, 0.95f);

        std::cout << "\n[YEAR " << current_year << "] ⚔️ MILITARY COUP in " << civ.name << "!" << std::endl;
        std::cout << "  Government: " << gov_form_name(old_gov) << " → " << gov_form_name(civ.government) << std::endl;
        std::cout << "  Ruler: " << prev_ruler_name << " removed | " << junta_ruler.name << " takes state control." << std::endl;
        std::cout << "  Stability: -10  Unrest: +20  Institutions: -45  Legitimacy: 35%" << std::endl;
        std::cout << "  [CHRONICLE] " << chronicle_desc << "\n" << std::endl;

        return true;
    }
    else if (action_type == "PROCLAIM_DICTATORSHIP") {
        civ.government = GovForm::DICTATORSHIP;
        civ.stability  = std::clamp(civ.stability + 15.0f, 0.0f, 100.0f);
        civ.unrest     = std::clamp(civ.unrest - 25.0f, 0.0f, 100.0f);
        civ.ruler_authority = 95.0f; // Supreme autocrat
        civ.legitimacy       = std::clamp(civ.legitimacy - 15.0f, 20.0f, 100.0f);
        civ.democratic_institution_strength = 0.0f;
        civ.opposition_strength = std::clamp(civ.opposition_strength + 30.0f, 0.0f, 100.0f);
        civ.last_transition_year = current_year;
        civ.under_martial_law    = true;

        if (civ.ruler_id >= 0) {
            for (auto& ch : characters) {
                if (ch.id == civ.ruler_id && ch.is_alive) {
                    ch.title = "Supreme Autocrat";
                    ch.trait = RulerTrait::AUTHORITARIAN;
                    std::cout << "\n[YEAR " << current_year << "] 👑 DICTATORSHIP PROCLAIMED in " << civ.name << "!" << std::endl;
                    std::cout << "  Government: " << gov_form_name(old_gov) << " → " << gov_form_name(civ.government) << std::endl;
                    std::cout << "  " << ch.name << " consolidates total autocratic power.\n" << std::endl;

                    engine.history.record(current_year, 1, "GOVERNMENT",
                        ch.name + " proclaims Dictatorship in " + civ.name,
                        "Total executive command centralized under Supreme Autocrat " + ch.name + ".",
                        civ.id, -1, {"consolidation_of_power", "autocracy"}, 0.95f);
                    break;
                }
            }
        }
        return true;
    }
    else if (action_type == "RESTORE_REPUBLIC" || action_type == "RESTORE_DEMOCRACY") {
        civ.government = (action_type == "RESTORE_DEMOCRACY") ? GovForm::DEMOCRACY : GovForm::REPUBLIC;
        civ.stability  = std::clamp(civ.stability + 15.0f, 0.0f, 100.0f);
        civ.unrest     = std::clamp(civ.unrest - 35.0f, 0.0f, 100.0f);
        civ.legitimacy = 80.0f;
        civ.ruler_authority = 60.0f;
        civ.democratic_institution_strength = 75.0f;
        civ.opposition_strength = 15.0f;
        civ.under_martial_law = false;
        civ.emergency_powers_active = false;
        civ.last_transition_year = current_year;

        auto elected_leader = engine.make_ruler(engine.next_char_id++, "", civ.id, current_year - 40);
        elected_leader.title = "President";
        elected_leader.trait = RulerTrait::DEMOCRATIC;
        civ.ruler_id = elected_leader.id;
        civ.character_ids.push_back(elected_leader.id);
        characters.push_back(elected_leader);

        std::cout << "\n[YEAR " << current_year << "] 🏛️ DEMOCRATIC RESTORATION in " << civ.name << "!" << std::endl;
        std::cout << "  Government: " << gov_form_name(old_gov) << " → " << gov_form_name(civ.government) << std::endl;
        std::cout << "  President " << elected_leader.name << " elected under restored constitutional rule.\n" << std::endl;

        engine.history.record(current_year, 1, "GOVERNMENT",
            "Democratic Restoration in " + civ.name,
            civ.name + " transitions from authoritarian rule to a " + gov_form_name(civ.government) + ".",
            civ.id, -1, {"democratic_revolution", "rule_of_law"}, 0.90f);
        return true;
    }
    else if (action_type == "EMERGENCY_LAWS" || action_type == "DECLARE_MARTIAL_LAW") {
        civ.emergency_powers_active = true;
        civ.stability  = std::clamp(civ.stability + 12.0f, 0.0f, 100.0f);
        civ.unrest     = std::clamp(civ.unrest - 18.0f, 0.0f, 100.0f);
        civ.ruler_authority = std::clamp(civ.ruler_authority + 15.0f, 0.0f, 100.0f);
        civ.democratic_institution_strength = std::max(10.0f, civ.democratic_institution_strength - 15.0f);
        return true;
    }
    else if (action_type == "REFORM") {
        civ.stability = std::clamp(civ.stability + 18.0f, 0.0f, 100.0f);
        civ.unrest    = std::clamp(civ.unrest - 20.0f, 0.0f, 100.0f);
        civ.legitimacy = std::clamp(civ.legitimacy + 12.0f, 0.0f, 100.0f);
        civ.democratic_institution_strength = std::clamp(civ.democratic_institution_strength + 10.0f, 0.0f, 100.0f);
        return true;
    }
    else if (action_type == "CONCESSIONS") {
        civ.unrest = std::clamp(civ.unrest - 25.0f, 0.0f, 100.0f);
        civ.public_support = std::clamp(civ.public_support + 15.0f, 0.0f, 100.0f);
        civ.ruler_authority = std::max(20.0f, civ.ruler_authority - 10.0f);
        civ.economy.annual_income = std::max(0.0f, civ.economy.annual_income - 30.0f); // Welfare cost
        return true;
    }
    else if (action_type == "CALL_ELECTION") {
        civ.legitimacy = std::clamp(civ.legitimacy + 20.0f, 0.0f, 100.0f);
        civ.unrest     = std::clamp(civ.unrest - 15.0f, 0.0f, 100.0f);
        civ.democratic_institution_strength = std::clamp(civ.democratic_institution_strength + 15.0f, 0.0f, 100.0f);
        return true;
    }
    else if (action_type == "SUPPRESS_UNREST") {
        civ.unrest     = std::clamp(civ.unrest - 25.0f, 0.0f, 100.0f);
        civ.stability  = std::clamp(civ.stability + 10.0f, 0.0f, 100.0f);
        civ.public_support = std::max(10.0f, civ.public_support - 15.0f);
        civ.opposition_strength = std::clamp(civ.opposition_strength + 15.0f, 0.0f, 100.0f);
        civ.army_size  = std::max(200.0f, civ.army_size - 300.0f); // Policing losses
        return true;
    }
    else if (action_type == "TRANSITION_TO_EMPIRE") {
        civ.government = GovForm::EMPIRE;
        civ.legitimacy = 85.0f;
        civ.ruler_authority = 90.0f;
        civ.last_transition_year = current_year;
        if (civ.ruler_id >= 0) {
            for (auto& ch : characters) {
                if (ch.id == civ.ruler_id) { ch.title = "Emperor"; break; }
            }
        }
        return true;
    }
    else if (action_type == "TRANSITION_TO_THEOCRACY") {
        civ.government = GovForm::THEOCRACY;
        civ.legitimacy = 90.0f;
        civ.ruler_authority = 85.0f;
        civ.last_transition_year = current_year;
        if (civ.ruler_id >= 0) {
            for (auto& ch : characters) {
                if (ch.id == civ.ruler_id) { ch.title = "Grand Patriarch"; break; }
            }
        }
        return true;
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  tick_power_consolidation  —  Annual Dynamic Consolidation & Risk Loops
// ─────────────────────────────────────────────────────────────────────────────
void GovernmentTransitionEngine::tick_power_consolidation(
    AeonCivilization& civ,
    const AeonCharacter* ruler,
    int current_year,
    AeonEngine& engine) {

    if (!GovernmentSystem::is_authoritarian(civ.government)) return;

    civ.years_in_power++;
    civ.years_current_gov++;

    float ruler_comp = ruler ? ruler->competence : 0.5f;
    float econ_perf  = std::clamp(civ.economy.gdp / 2000.0f, 0.2f, 2.0f);

    // Dynamic Feedback:
    // Good economy + competent ruler -> gains authority, crushes opposition, gains legitimacy
    if (econ_perf > 1.0f && ruler_comp > 0.60f) {
        civ.ruler_authority     = std::clamp(civ.ruler_authority + 2.5f, 0.0f, 100.0f);
        civ.military_loyalty    = std::clamp(civ.military_loyalty + 2.0f, 0.0f, 100.0f);
        civ.opposition_strength = std::max(5.0f, civ.opposition_strength - 3.0f);
        civ.legitimacy          = std::clamp(civ.legitimacy + 1.5f, 0.0f, 100.0f);
        civ.unrest              = std::max(5.0f, civ.unrest - 2.0f);
    }
    // Bad economy / high corruption -> military loyalty drops, opposition explodes
    else if (econ_perf < 0.70f || civ.corruption > 50.0f || (ruler && ruler->trait == RulerTrait::TYRANT)) {
        civ.ruler_authority     = std::max(20.0f, civ.ruler_authority - 2.0f);
        civ.military_loyalty    = std::max(10.0f, civ.military_loyalty - 3.0f);
        civ.opposition_strength = std::clamp(civ.opposition_strength + 4.0f, 0.0f, 100.0f);
        civ.unrest              = std::clamp(civ.unrest + 3.0f, 0.0f, 100.0f);
        civ.stability           = std::max(10.0f, civ.stability - 2.0f);

        // Counter-revolution risk if opposition is supreme and military loyalty collapsed
        if (civ.opposition_strength > 75.0f && civ.military_loyalty < 40.0f) {
            std::cout << "\n[YEAR " << current_year << "] 🔥 POPULAR REVOLUTION in " << civ.name << "!" << std::endl;
            std::cout << "  The authoritarian regime collapses under public uprising and military mutiny!\n" << std::endl;

            apply_transition("RESTORE_REPUBLIC", civ, engine.civs, engine.characters, engine, current_year);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  handle_authoritarian_succession  —  Succession Crises for Dictators/Juntas
// ─────────────────────────────────────────────────────────────────────────────
void GovernmentTransitionEngine::handle_authoritarian_succession(
    AeonCivilization& civ,
    const AeonCharacter& dead_ruler,
    std::vector<AeonCharacter>& characters,
    AeonEngine& engine,
    int current_year) {

    std::cout << "\n[YEAR " << current_year << "] 💀 AUTHORITARIAN SUCCESSION CRISIS in " << civ.name
              << " following the demise of " << dead_ruler.name << "!" << std::endl;

    float inst_strength = civ.democratic_institution_strength;
    float mil_loyalty   = civ.military_loyalty;
    float opposition    = civ.opposition_strength;

    // Roll for succession crisis outcome
    int roll = engine.rng.uniform_int(0, 100);

    // Scenario A: Military Takeover / New General Seizes Power (45% base if high military)
    if (roll < 45 || mil_loyalty > 70.0f) {
        auto new_gen = engine.make_ruler(engine.next_char_id++, "", civ.id, current_year - 40);
        new_gen.title = (civ.government == GovForm::DICTATORSHIP) ? "Supreme Autocrat" : "General of the Realm";
        new_gen.trait = RulerTrait::MILITARIST;
        civ.ruler_id = new_gen.id;
        civ.character_ids.push_back(new_gen.id);
        characters.push_back(new_gen);

        civ.stability = std::clamp(civ.stability - 15.0f, 15.0f, 100.0f);
        civ.ruler_authority = 70.0f; // Must reconsolidate
        civ.opposition_strength = std::clamp(civ.opposition_strength + 15.0f, 0.0f, 100.0f);

        std::cout << "  [SUCCESSION RESULT] " << new_gen.name << " seizes autocratic command.\n" << std::endl;
        engine.history.record(current_year, 1, "SUCCESSION",
            "Autocratic succession in " + civ.name,
            new_gen.name + " assumes control of the authoritarian state.",
            civ.id, -1, {"military_succession"}, 0.85f);
    }
    // Scenario B: Democratic Restoration / Opposition Revolt (25% if opposition strong or institutions exist)
    else if (roll < 75 || opposition > 60.0f || inst_strength > 40.0f) {
        std::cout << "  [SUCCESSION RESULT] Power vacuum triggers democratic restoration!\n" << std::endl;
        apply_transition("RESTORE_REPUBLIC", civ, engine.civs, characters, engine, current_year);
    }
    // Scenario C: Elite Oligarchic Takeover
    else {
        civ.government = GovForm::OLIGARCHY;
        auto oligarch = engine.make_ruler(engine.next_char_id++, "High Chancellor", civ.id, current_year - 48);
        oligarch.trait = RulerTrait::MERCHANT;
        civ.ruler_id = oligarch.id;
        civ.character_ids.push_back(oligarch.id);
        characters.push_back(oligarch);

        civ.stability = std::clamp(civ.stability + 10.0f, 20.0f, 100.0f);
        std::cout << "  [SUCCESSION RESULT] Elite council takes power: Transition to Oligarchy.\n" << std::endl;
        engine.history.record(current_year, 1, "SUCCESSION",
            "Oligarchic council takes power in " + civ.name,
            "The merchant nobility and oligarchs seize the state apparatus.",
            civ.id, -1, {"oligarchic_takeover"}, 0.80f);
    }
}

} // namespace Aeon
