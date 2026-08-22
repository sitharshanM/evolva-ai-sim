#include "aeon_dynasty.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

AeonDynastyEngine::AeonDynastyEngine() {
}

void AeonDynastyEngine::init_dynasties(const AeonEngine& engine) {
    royals.clear();
    dynastic_marriages.clear();
    active_pretenders.clear();

    for (const auto& civ : engine.civs) {
        if (civ.is_commons) continue;
        RoyalPerson crown_prince;
        crown_prince.id = (int)royals.size() + 1;
        crown_prince.civ_id = civ.id;
        crown_prince.name = "Crown Prince of " + civ.name;
        crown_prince.title = "Heir Apparent";
        crown_prince.age = 22;
        crown_prince.is_heir = true;
        crown_prince.married = false;
        crown_prince.legitimacy = 85.0f;
        royals.push_back(crown_prince);
    }
}

void AeonDynastyEngine::arrange_royal_marriage(int civ1_id, int civ2_id, AeonEngine& engine) {
    if (civ1_id < 0 || civ1_id >= (int)engine.civs.size()) return;
    if (civ2_id < 0 || civ2_id >= (int)engine.civs.size()) return;

    auto& civ1 = engine.civs[civ1_id];
    auto& civ2 = engine.civs[civ2_id];

    DynasticAlliance da;
    da.civ1_id = civ1_id;
    da.civ2_id = civ2_id;
    da.marriage_description = "Royal Union between House of " + civ1.name + " & House of " + civ2.name;
    da.trust_bonus = 40.0f;
    da.allows_inheritance_claim = true;
    dynastic_marriages.push_back(da);

    engine.history.record(engine.year, engine.month, "DYNASTY",
        "Royal Marriage Sealed: " + civ1.name + " & " + civ2.name,
        "Dynastic blood alliance guarantees non-aggression and royal trade privileges.", civ1_id);

    std::cout << "[YEAR " << engine.year << "] 👑 ROYAL MARRIAGE: House of " << civ1.name
              << " unites with House of " << civ2.name << "!" << std::endl;
}

void AeonDynastyEngine::update_dynasties_tick(AeonEngine& engine) {
    for (auto& r : royals) {
        if (!r.is_alive) continue;
        r.age++;
        if (r.age > 70 && engine.rng.chance(0.20f)) {
            r.is_alive = false;
            trigger_succession_crisis(r.civ_id, engine);
        }
    }

    // Progress active pretender wars
    for (auto& p : active_pretenders) {
        resolve_pretender_war(p.civ_id, engine);
    }
    active_pretenders.erase(
        std::remove_if(active_pretenders.begin(), active_pretenders.end(),
            [](const PretenderClaimant& p){ return !p.war_declared; }),
        active_pretenders.end());

    // Occasional dynastic marriage between friendly civs
    int num_civs = static_cast<int>(engine.civs.size());
    if (engine.rng.chance(0.15f) && num_civs >= 2) {
        int c1 = engine.rng.uniform_int(0, num_civs - 1);
        int c2 = (c1 + 1 + engine.rng.uniform_int(0, num_civs - 2)) % num_civs;
        if (!engine.civs[c1].at_war && !engine.civs[c2].at_war) {
            arrange_royal_marriage(c1, c2, engine);
        }
    }
}

SuccessionOutcome AeonDynastyEngine::evaluate_succession(int civ_id, AeonEngine& engine, int& out_new_ruler_id) {
    if (civ_id < 0 || civ_id >= (int)engine.civs.size()) return SuccessionOutcome::PEACEFUL;
    auto& civ = engine.civs[civ_id];

    // Find candidate heir from royals or create candidate
    float heir_legitimacy = 80.0f;
    for (const auto& r : royals) {
        if (r.civ_id == civ_id && r.is_alive && r.is_heir) {
            heir_legitimacy = r.legitimacy;
            break;
        }
    }

    float mil_loyalty = civ.military_loyalty;
    float elite_sup   = civ.elite_support;
    float stab        = civ.stability;

    // Check for foreign-backed claimants
    int foreign_patron = -1;
    for (const auto& other : engine.civs) {
        if (other.id != civ_id && other.is_alive > 0.0f) {
            const auto* rel = engine.history.relation_view(other.id, civ_id);
            if (rel && rel->hatred > 50.0f && other.military_power > civ.military_power) {
                foreign_patron = other.id;
                break;
            }
        }
    }

    SuccessionOutcome outcome = SuccessionOutcome::PEACEFUL;

    if (stab < 20.0f && mil_loyalty < 30.0f) {
        outcome = SuccessionOutcome::COUP;
    } else if (stab < 30.0f && foreign_patron >= 0 && engine.rng.chance(0.50f)) {
        outcome = SuccessionOutcome::FOREIGN_BACKED_CLAIMANT;
    } else if (heir_legitimacy < 45.0f && elite_sup < 40.0f) {
        outcome = SuccessionOutcome::CIVIL_WAR;
    } else if (mil_loyalty > 80.0f && elite_sup < 35.0f) {
        outcome = SuccessionOutcome::MILITARY_BACKED;
    } else if (elite_sup > 80.0f && mil_loyalty < 40.0f) {
        outcome = SuccessionOutcome::NOBLE_BACKED;
    } else if (heir_legitimacy < 60.0f) {
        outcome = SuccessionOutcome::DISPUTED;
    } else if (civ.provinces.size() >= 3 && stab < 25.0f && engine.rng.chance(0.30f)) {
        outcome = SuccessionOutcome::REGIONAL_INDEPENDENCE;
    } else {
        outcome = SuccessionOutcome::PEACEFUL;
    }

    // Spawn new ruler character
    auto new_ruler = engine.make_ruler(engine.next_char_id++, "", civ.id, engine.year - engine.rng.uniform_int(22, 50));
    out_new_ruler_id = new_ruler.id;
    civ.ruler_id = new_ruler.id;
    civ.character_ids.push_back(new_ruler.id);
    engine.characters.push_back(new_ruler);

    // Apply consequences
    switch (outcome) {
        case SuccessionOutcome::PEACEFUL:
            civ.stability = std::clamp(civ.stability - 5.0f, 10.0f, 100.0f);
            break;
        case SuccessionOutcome::DISPUTED:
            civ.stability = std::clamp(civ.stability - 15.0f, 5.0f, 100.0f);
            civ.unrest += 15.0f;
            break;
        case SuccessionOutcome::MILITARY_BACKED:
            civ.stability = std::clamp(civ.stability - 10.0f, 5.0f, 100.0f);
            civ.military_loyalty += 10.0f;
            civ.elite_support -= 15.0f;
            break;
        case SuccessionOutcome::NOBLE_BACKED:
            civ.stability = std::clamp(civ.stability - 10.0f, 5.0f, 100.0f);
            civ.elite_support += 15.0f;
            civ.military_loyalty -= 10.0f;
            break;
        case SuccessionOutcome::CIVIL_WAR: {
            civ.stability = std::max(5.0f, civ.stability - 40.0f);
            civ.unrest += 40.0f;
            PretenderClaimant pretender;
            pretender.claimant_id = out_new_ruler_id;
            pretender.civ_id = civ_id;
            pretender.name = "House Rival " + new_ruler.name;
            pretender.military_support_pct = 40.0f;
            pretender.war_declared = true;
            active_pretenders.push_back(pretender);
            break;
        }
        case SuccessionOutcome::COUP:
            civ.government = GovForm::MILITARY_JUNTA;
            civ.stability = std::max(10.0f, civ.stability - 25.0f);
            civ.crisis_state = CrisisState::MILITARY_REGIME;
            break;
        case SuccessionOutcome::FOREIGN_BACKED_CLAIMANT:
            civ.stability = std::max(5.0f, civ.stability - 30.0f);
            if (foreign_patron >= 0) {
                engine.history.relation(foreign_patron, civ_id).record_aid_given();
            }
            break;
        case SuccessionOutcome::REGIONAL_INDEPENDENCE:
            if (!civ.provinces.empty()) {
                civ.provinces.back().in_rebellion = true;
                civ.provinces.back().autonomy_demand = 100.0f;
            }
            civ.stability = std::max(10.0f, civ.stability - 25.0f);
            break;
    }

    std::cout << "[YEAR " << engine.year << "] 👑 SUCCESSION OUTCOME: " << civ.name
              << " executes " << succession_outcome_name(outcome)
              << " -> " << new_ruler.name << " (" << ruler_trait_name(new_ruler.trait) << ") takes the realm!" << std::endl;

    engine.history.record(engine.year, engine.month, "DYNASTY",
        civ.name + " Dynastic Transition: " + succession_outcome_name(outcome),
        new_ruler.name + " ascends throne amid " + succession_outcome_name(outcome),
        civ_id, foreign_patron, {"succession_crisis"}, 0.80f);

    return outcome;
}

void AeonDynastyEngine::trigger_succession_crisis(int civ_id, AeonEngine& engine) {
    if (civ_id < 0 || civ_id >= (int)engine.civs.size()) return;
    auto& civ = engine.civs[civ_id];
    civ.stability = std::max(0.0f, civ.stability - 35.0f);

    engine.history.record(engine.year, engine.month, "DYNASTY_CRISIS",
        "SUCCESSION CRISIS IN " + civ.name + "!",
        "Death of the sovereign triggered a bitter civil war among rival pretenders to the throne.", civ_id);

    std::cout << "[YEAR " << engine.year << "] 👑 DYNASTIC CRISIS: Succession struggle in " << civ.name << "!" << std::endl;
}

void AeonDynastyEngine::resolve_pretender_war(int civ_id, AeonEngine& engine) {
    if (civ_id < 0 || civ_id >= (int)engine.civs.size()) return;
    auto& civ = engine.civs[civ_id];

    for (auto& p : active_pretenders) {
        if (p.civ_id == civ_id && p.war_declared) {
            if (engine.rng.chance(0.35f)) {
                p.war_declared = false;
                civ.stability = std::min(100.0f, civ.stability + 20.0f);
                civ.at_war = false;
                engine.history.record(engine.year, engine.month, "DYNASTY",
                    "Pretender War Resolved", civ.name + " loyalist legions crushed pretender forces and restored order.", civ_id);
            }
            break;
        }
    }
}

} // namespace Aeon

