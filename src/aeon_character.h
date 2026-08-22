#pragma once
// =============================================================================
//  aeon_character.h  —  Individually simulated characters and rulers
//
//  UPGRADES:
//   • Full numeric ruler skills: competence, military_skill, diplomatic_skill,
//     economic_skill, scientific_skill (0.0 to 1.0)
//   • Personality traits (WARMONGER, DIPLOMAT, SCHOLAR, MERCHANT, etc.)
//   • Succession dynamics with legitimacy and impact on nation stability
//   • Culturally diverse name generation
// =============================================================================
#include "aeon_config.h"
#include "aeon_world_types.h"
#include <string>
#include <vector>
#include <deque>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  Ruler Trait
// ─────────────────────────────────────────────────────────────────────────────
enum class RulerTrait {
    WARMONGER,      // High aggression, favors military conquest
    MILITARIST,     // Prioritizes army buildup and armed force solutions
    AUTHORITARIAN,  // Centralizes power, favors decrees, decrees and martial law
    DEMOCRATIC,     // Respects rule of law, elections, and public consensus
    DIPLOMAT,       // High diplomacy, seeks alliances and federations
    SCHOLAR,        // High science, accelerates technology research
    TECHNOCRAT,     // Scientific efficiency, data-driven governance
    MERCHANT,       // High trade, maximizes commerce and gold
    EXPANSIONIST,   // High expansion, colonizes and settles territory
    REFORMER,       // Balances stability, enacts institutional reforms
    TYRANT,         // High military control, ruthless crackdowns
    PARANOID,       // Sees conspiracies everywhere, purges rivals, high surveillance
    IDEALIST,       // Driven by ideology and popular revolution
    PRAGMATIST,     // Flexible, adopts whatever policy preserves power
    ISOLATIONIST    // Avoids foreign entanglements, focuses inward
};

inline const char* ruler_trait_name(RulerTrait t) {
    switch (t) {
        case RulerTrait::WARMONGER:     return "Warmonger";
        case RulerTrait::MILITARIST:    return "Militarist";
        case RulerTrait::AUTHORITARIAN: return "Authoritarian";
        case RulerTrait::DEMOCRATIC:    return "Democratic";
        case RulerTrait::DIPLOMAT:      return "Diplomat";
        case RulerTrait::SCHOLAR:       return "Scholar";
        case RulerTrait::TECHNOCRAT:    return "Technocrat";
        case RulerTrait::MERCHANT:      return "Merchant";
        case RulerTrait::EXPANSIONIST:  return "Expansionist";
        case RulerTrait::REFORMER:      return "Reformer";
        case RulerTrait::TYRANT:        return "Tyrant";
        case RulerTrait::PARANOID:      return "Paranoid";
        case RulerTrait::IDEALIST:      return "Idealist";
        case RulerTrait::PRAGMATIST:    return "Pragmatist";
        case RulerTrait::ISOLATIONIST:  return "Isolationist";
    }
    return "Balanced";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Ruler Personal Agenda / Goal
// ─────────────────────────────────────────────────────────────────────────────
enum class RulerAgenda {
    SURVIVE,                // Ensure realm & personal survival at all costs
    EXPAND_EMPIRE,          // Conquer territory, annex neighbours
    BECOME_HEGEMON,         // Military superiority, intimidate rivals
    TECH_LEADER,            // Scientific breakthroughs, futuristic infrastructure
    ENRICH_REALM,           // Accumulate gold, dominate global trade routes
    PRESERVE_PEACE,         // Neutrality, diplomacy, avoid bloodshed
    CENTRALIZE_POWER,       // Subjugate elites, crush opposition, autocracy
    DEMOCRATIZE,            // Strengthen parliament, free elections, civil rights
    INSTITUTIONAL_REFORM    // Anti-corruption, efficient judiciary, stable economy
};

inline const char* ruler_agenda_name(RulerAgenda a) {
    switch (a) {
        case RulerAgenda::SURVIVE:              return "Survive";
        case RulerAgenda::EXPAND_EMPIRE:        return "Expand Empire";
        case RulerAgenda::BECOME_HEGEMON:       return "Become Hegemon";
        case RulerAgenda::TECH_LEADER:          return "Tech Dominance";
        case RulerAgenda::ENRICH_REALM:         return "Enrich Realm";
        case RulerAgenda::PRESERVE_PEACE:       return "Preserve Peace";
        case RulerAgenda::CENTRALIZE_POWER:     return "Centralize Power";
        case RulerAgenda::DEMOCRATIZE:          return "Democratize";
        case RulerAgenda::INSTITUTIONAL_REFORM: return "Institutional Reform";
    }
    return "Balanced";
}

// ─────────────────────────────────────────────────────────────────────────────
//  AeonCharacter  —  Individually simulated important person
// ─────────────────────────────────────────────────────────────────────────────
struct AeonCharacter {
    int         id       = 0;
    std::string name;
    std::string title;       // King, Emperor, Archon, President, Grand Duke...
    int         age      = 30;
    float       health   = 100.0f;
    int         civ_id   = -1;

    // Core Skills (0.0 to 1.0)
    float competence        = 0.5f; // Overall administrative capability
    float military_skill    = 0.5f; // Tactical and martial prowess
    float diplomatic_skill  = 0.5f; // Charisma and negotiation ability
    float economic_skill    = 0.5f; // Fiscal management and trade insight
    float scientific_skill  = 0.5f; // Innovation and philosophical openness

    // Primary archetype trait and personal agenda
    RulerTrait  trait  = RulerTrait::REFORMER;
    RulerAgenda agenda = RulerAgenda::SURVIVE;

    // ── 14 Deep Ruler Personality Parameters (0.0 to 1.0) ────────────────────
    float ambition               = 0.5f; // Desire for expansion / dominance
    float intelligence           = 0.5f; // Analytical foresight
    float diplomacy              = 0.5f; // Alias / nuance for diplomatic skill
    float patience               = 0.5f; // Willingness to delay war/conquest
    float paranoia               = 0.2f; // Distrust of neighbors & factions, espionage focus
    float greed                  = 0.4f; // Focus on wealth & tax extraction
    float charisma               = 0.6f; // Public approval boost & speechcraft
    float cruelty                = 0.2f; // Ruthlessness in crackdowns & rebellions
    float risk_tolerance         = 0.4f; // Willingness to take aggressive gambles
    float loyalty                = 0.7f; // Honor towards allies & treaties
    float reform_tendency        = 0.5f; // Desire for institutional evolution
    float authoritarian_tendency = 0.3f; // Desire for centralized autocracy
    float ego                    = 0.5f; // Pride and resistance to insults
    float morality               = 0.5f; // Ethical constraints in diplomacy/war

    // Status & Political Leverage
    float wealth         = 100.0f;
    float reputation     = 50.0f;
    float influence      = 30.0f;
    float military_clout = 0.0f;
    float religious_clout= 0.0f;

    // Faction Alignment (which faction this character is allied with)
    FactionType favored_faction = FactionType::NOBILITY;
    float       faction_loyalty = 0.7f; // 0..1

    // Dynastic & Succession Fields
    float claim_legitimacy      = 1.0f; // 0..1 recognition as rightful heir
    int   patron_civ_id         = -1;   // Foreign civ backing this claimant (-1 if none)
    bool  is_pretender_claimant = false;

    // Relationships (character_id -> trust score -100 to +100)
    std::vector<std::pair<int,float>> relationships;

    // Memory of key events
    std::vector<std::string> memory_log;

    bool is_ruler    = false;
    bool is_alive    = true;
    int  birth_year  = 0;
    int  death_year  = -1;

    // Heirs / family
    std::vector<int> heirs; // character IDs

    void age_one_year();
    bool check_natural_death(int year) const;

    // Get dominant skill score (0.0 to 1.0)
    float get_effective_skill() const {
        return (competence * 0.35f +
                military_skill * 0.15f +
                diplomatic_skill * 0.15f +
                economic_skill * 0.15f +
                scientific_skill * 0.10f +
                intelligence * 0.10f);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Succession  —  When a ruler dies, pick the next
// ─────────────────────────────────────────────────────────────────────────────
enum class SuccessionOutcome {
    PEACEFUL,
    DISPUTED,
    MILITARY_BACKED,
    NOBLE_BACKED,
    CIVIL_WAR,
    COUP,
    FOREIGN_BACKED_CLAIMANT,
    REGIONAL_INDEPENDENCE
};

inline const char* succession_outcome_name(SuccessionOutcome s) {
    switch (s) {
        case SuccessionOutcome::PEACEFUL:               return "Peaceful Succession";
        case SuccessionOutcome::DISPUTED:               return "Disputed Succession";
        case SuccessionOutcome::MILITARY_BACKED:        return "Military-Backed Succession";
        case SuccessionOutcome::NOBLE_BACKED:           return "Noble-Backed Succession";
        case SuccessionOutcome::CIVIL_WAR:              return "Succession Civil War";
        case SuccessionOutcome::COUP:                   return "Preemptive Military Coup";
        case SuccessionOutcome::FOREIGN_BACKED_CLAIMANT:return "Foreign-Backed Claimant Takeover";
        case SuccessionOutcome::REGIONAL_INDEPENDENCE:  return "Regional Independence Breakaway";
    }
    return "Succession";
}

struct SuccessionRecord {
    int   year        = 0;
    int   civ_id      = 0;
    int   old_ruler   = -1;
    int   new_ruler   = -1;
    std::string cause; // Natural death, Assassination, Coup, Election
    float legitimacy = 1.0f; // 0.0 to 1.0
    SuccessionOutcome outcome = SuccessionOutcome::PEACEFUL;
};

} // namespace Aeon
