// =============================================================================
//  aeon_test_government.cpp  —  Comprehensive Government Transition Engine Tests
// =============================================================================
#include "aeon_test_government.h"
#include "aeon_government.h"
#include "aeon_civilization.h"
#include "aeon_character.h"
#include "aeon_engine.h"
#include <iostream>
#include <cassert>
#include <iomanip>

namespace Aeon {

bool GovernmentTestSuite::run_all_tests() {
    std::cout << "\n========================================================================\n";
    std::cout << "  🧪 AEON GOVERNMENT TRANSITION & POLITICAL CRISIS TEST SUITE\n";
    std::cout << "========================================================================\n";

    GovernmentTransitionEngine engine;
    int passed = 0;
    int total = 10;

    // ─────────────────────────────────────────────────────────────────────────
    // SCENARIO A — TEST 1:
    // High military + HIGH loyalty + stable realm -> NO COUP
    // Expected: Military coup score = 0.0 (hard-suppressed by stable pillars)
    // Military power alone NEVER causes a coup.
    // ─────────────────────────────────────────────────────────────────────────
    {
        std::cout << "\n[TEST 1] SCENARIO A: High military + high loyalty + stable realm..." << std::endl;
        AeonCivilization civ;
        civ.id = 0;
        civ.name = "ELDORIA";
        civ.government = GovForm::DEMOCRACY;
        civ.stability = 88.0f;
        civ.unrest = 8.0f;
        civ.army_size = 90000.0f;      // Massive army — but loyal and in a stable state
        civ.population.total = 500000;
        civ.military_loyalty = 92.0f;  // Highly loyal: REDUCES coup probability
        civ.military_discontent = 0.0f;
        civ.ruler_authority = 70.0f;
        civ.legitimacy = 88.0f;
        civ.democratic_institution_strength = 85.0f;

        AeonCharacter ruler;
        ruler.name = "Queen Lyra";
        ruler.trait = RulerTrait::DEMOCRATIC;
        ruler.competence = 0.90f;

        std::vector<AeonCivilization> all_civs = { civ };
        auto actions = engine.evaluate_government_actions(civ, all_civs, &ruler, 2100);

        float coup_score = 0.0f;
        bool coup_in_pool = false;
        for (const auto& a : actions) {
            if (a.action_type == "MILITARY_COUP") { coup_score = a.total_score; coup_in_pool = true; }
        }

        std::cout << "  -> Coup Score: " << std::fixed << std::setprecision(3) << coup_score
                  << " (threshold: " << engine.config.coup_threshold << ")" << std::endl;
        std::cout << "  -> Actions in pool: " << actions.size() << std::endl;

        if (!coup_in_pool && coup_score < engine.config.coup_threshold) {
            std::cout << "  ✅ PASSED: High military + high loyalty + stable realm = NO COUP." << std::endl;
            passed++;
        } else {
            std::cout << "  ❌ FAILED: Coup entered candidate pool despite strong stability pillars." << std::endl;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // SCENARIO B — TEST 2:
    // High military + LOW loyalty + severe unrest + weak institutions -> COUP LIKELY
    // Expected: Coup score >= threshold (0.70), enters candidate pool
    // ─────────────────────────────────────────────────────────────────────────
    {
        std::cout << "\n[TEST 2] SCENARIO B: High military + low loyalty + severe unrest..." << std::endl;
        AeonCivilization civ;
        civ.id = 0;
        civ.name = "DRAKOR";
        civ.government = GovForm::REPUBLIC;
        civ.stability = 14.0f;
        civ.unrest = 88.0f;
        civ.army_size = 40000.0f;
        civ.population.total = 300000;
        civ.military_loyalty = 18.0f;  // Very low: INCREASES coup probability
        civ.military_discontent = 75.0f;
        civ.coup_support = 60.0f;
        civ.ruler_authority = 30.0f;
        civ.legitimacy = 22.0f;
        civ.democratic_institution_strength = 12.0f;

        AeonCharacter ruler;
        ruler.name = "President Moran";
        ruler.trait = RulerTrait::TYRANT;
        ruler.competence = 0.25f;

        std::vector<AeonCivilization> all_civs = { civ };
        auto actions = engine.evaluate_government_actions(civ, all_civs, &ruler, 2100);

        float coup_score = 0.0f;
        bool coup_in_pool = false;
        for (const auto& a : actions) {
            if (a.action_type == "MILITARY_COUP") { coup_score = a.total_score; coup_in_pool = true; }
        }

        std::cout << "  -> Coup Score: " << std::fixed << std::setprecision(3) << coup_score
                  << " (threshold: " << engine.config.coup_threshold << ")" << std::endl;

        if (coup_in_pool && coup_score >= engine.config.coup_threshold) {
            std::cout << "  ✅ PASSED: Combined crisis conditions produce valid coup probability." << std::endl;
            passed++;
        } else {
            std::cout << "  ❌ FAILED: Coup score too low despite multi-factor crisis (score="
                      << coup_score << ")." << std::endl;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // SCENARIO C — TEST 3:
    // Low military + severe unrest -> political crisis, NOT military coup
    // Expected: Reform/Emergency laws win; coup below threshold due to tiny army
    // ─────────────────────────────────────────────────────────────────────────
    {
        std::cout << "\n[TEST 3] SCENARIO C: Low military + severe unrest -> no military coup..." << std::endl;
        AeonCivilization civ;
        civ.id = 0;
        civ.name = "THALOR";
        civ.government = GovForm::REPUBLIC;
        civ.stability = 20.0f;
        civ.unrest = 82.0f;
        civ.army_size = 1800.0f;       // Very small army — insufficient for coup
        civ.population.total = 200000;
        civ.military_loyalty = 55.0f;
        civ.ruler_authority = 45.0f;
        civ.legitimacy = 40.0f;
        civ.democratic_institution_strength = 45.0f;

        AeonCharacter ruler;
        ruler.name = "Consul Vera";
        ruler.trait = RulerTrait::REFORMER;
        ruler.competence = 0.70f;

        std::vector<AeonCivilization> all_civs = { civ };
        auto actions = engine.evaluate_government_actions(civ, all_civs, &ruler, 2100);

        float coup_score = 0.0f, reform_score = 0.0f, emerg_score = 0.0f;
        bool coup_in_pool = false;
        for (const auto& a : actions) {
            if (a.action_type == "MILITARY_COUP") { coup_score = a.total_score; coup_in_pool = true; }
            if (a.action_type == "REFORM") reform_score = a.total_score;
            if (a.action_type == "EMERGENCY_LAWS") emerg_score = a.total_score;
        }

        std::cout << "  -> Coup Score: " << coup_score
                  << " | Reform: " << reform_score
                  << " | Emergency: " << emerg_score << std::endl;

        if (!coup_in_pool && (reform_score > coup_score || emerg_score > coup_score)) {
            std::cout << "  ✅ PASSED: Low military blocks coup; political crisis handled by reform/emergency." << std::endl;
            passed++;
        } else {
            std::cout << "  ❌ FAILED: Coup should be blocked by insufficient military (army=1800)." << std::endl;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // TEST 4: Reformer ruler with crisis — reform takes priority over coup
    // ─────────────────────────────────────────────────────────────────────────
    {
        std::cout << "\n[TEST 4] Reformer ruler with crisis — reform over coup..." << std::endl;
        AeonCivilization civ;
        civ.id = 0;
        civ.name = "VALORIA";
        civ.government = GovForm::REPUBLIC;
        civ.stability = 20.0f;
        civ.unrest = 75.0f;
        civ.army_size = 30000.0f;
        civ.population.total = 300000;
        civ.military_loyalty = 80.0f;  // Loyal military — suppresses coup
        civ.ruler_authority = 70.0f;
        civ.democratic_institution_strength = 60.0f;

        AeonCharacter ruler;
        ruler.name = "Consul Aurelius";
        ruler.trait = RulerTrait::REFORMER;
        ruler.competence = 0.85f;

        std::vector<AeonCivilization> all_civs = { civ };
        auto actions = engine.evaluate_government_actions(civ, all_civs, &ruler, 2100);

        float reform_score = 0.0f, coup_score = 0.0f;
        for (const auto& a : actions) {
            if (a.action_type == "REFORM") reform_score = a.total_score;
            if (a.action_type == "MILITARY_COUP") coup_score = a.total_score;
        }

        std::cout << "  -> Reform Score: " << reform_score << " vs Coup Score: " << coup_score << std::endl;
        if (reform_score > coup_score) {
            std::cout << "  ✅ PASSED: Reformer ruler prioritizes democratic reforms over military coup." << std::endl;
            passed++;
        } else {
            std::cout << "  ❌ FAILED: Reform score should exceed coup score for reformer." << std::endl;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // SCENARIO E/F — TEST 5:
    // Coup Cooldown Enforcement (recent coup 3 years ago)
    // Expected: Coup score drastically reduced, below threshold
    // ─────────────────────────────────────────────────────────────────────────
    {
        std::cout << "\n[TEST 5] SCENARIO E/F: Coup cooldown enforcement (recent coup 3 years ago)..." << std::endl;
        AeonCivilization civ;
        civ.id = 0;
        civ.name = "THALASSA";
        civ.government = GovForm::MILITARY_JUNTA;
        civ.stability = 20.0f;
        civ.unrest = 80.0f;
        civ.army_size = 25000.0f;
        civ.population.total = 300000;
        civ.military_loyalty = 15.0f; // Low (mutinous)
        civ.military_discontent = 80.0f;
        civ.recent_coup_year = 2097;  // 3 years ago (cooldown = 20 years)

        AeonCharacter ruler;
        ruler.trait = RulerTrait::PARANOID;

        std::vector<AeonCivilization> all_civs = { civ };
        auto actions = engine.evaluate_government_actions(civ, all_civs, &ruler, 2100);

        float coup_score = 0.0f;
        for (const auto& a : actions) {
            if (a.action_type == "MILITARY_COUP") coup_score = a.total_score;
        }

        std::cout << "  -> Coup Score during 20-year cooldown: " << coup_score << std::endl;
        if (coup_score < 0.20f) {
            std::cout << "  ✅ PASSED: Coup cooldown successfully prevents immediate coup loops." << std::endl;
            passed++;
        } else {
            std::cout << "  ❌ FAILED: Coup cooldown failed to suppress score (score=" << coup_score << ")." << std::endl;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // TEST 6: Dictatorship + economic growth
    // ─────────────────────────────────────────────────────────────────────────
    {
        std::cout << "\n[TEST 6] Dictatorship + economic growth power consolidation..." << std::endl;
        AeonEngine aeon;
        AeonCivilization civ;
        civ.id = 0;
        civ.name = "DRAKOR";
        civ.government = GovForm::DICTATORSHIP;
        civ.ruler_authority = 60.0f;
        civ.military_loyalty = 75.0f;
        civ.opposition_strength = 45.0f;
        civ.legitimacy = 40.0f;
        civ.unrest = 35.0f;
        civ.economy.gdp = 3500.0f;

        AeonCharacter ruler;
        ruler.competence = 0.85f;

        engine.tick_power_consolidation(civ, &ruler, 2100, aeon);

        std::cout << "  -> After growth: Authority=" << civ.ruler_authority
                  << " (was 60), Opposition=" << civ.opposition_strength
                  << " (was 45), Legitimacy=" << civ.legitimacy << " (was 40)" << std::endl;

        if (civ.ruler_authority > 60.0f && civ.opposition_strength < 45.0f && civ.legitimacy > 40.0f) {
            std::cout << "  ✅ PASSED: Competent economic rule successfully consolidates power." << std::endl;
            passed++;
        } else {
            std::cout << "  ❌ FAILED: Power consolidation feedback failed." << std::endl;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // TEST 7: Dictatorship + economic collapse & high corruption
    // ─────────────────────────────────────────────────────────────────────────
    {
        std::cout << "\n[TEST 7] Dictatorship + economic collapse & high corruption..." << std::endl;
        AeonEngine aeon;
        aeon.civs.resize(1);
        aeon.civs[0].name = "DRAKOR";

        AeonCivilization civ;
        civ.id = 0;
        civ.name = "DRAKOR";
        civ.government = GovForm::DICTATORSHIP;
        civ.ruler_authority = 40.0f;
        civ.military_loyalty = 35.0f;
        civ.opposition_strength = 78.0f;
        civ.corruption = 65.0f;
        civ.economy.gdp = 400.0f;

        AeonCharacter ruler;
        ruler.trait = RulerTrait::TYRANT;
        ruler.competence = 0.30f;

        engine.tick_power_consolidation(civ, &ruler, 2100, aeon);

        std::cout << "  -> Government after uprising: " << gov_form_name(civ.government) << std::endl;

        if (civ.government == GovForm::REPUBLIC || civ.government == GovForm::DEMOCRACY) {
            std::cout << "  ✅ PASSED: Popular revolution overthrew corrupt dictatorship." << std::endl;
            passed++;
        } else {
            std::cout << "  ❌ FAILED: Revolution did not trigger despite conditions." << std::endl;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // TEST 8: Dictator dies + authoritarian succession crisis
    // ─────────────────────────────────────────────────────────────────────────
    {
        std::cout << "\n[TEST 8] Dictator dies + authoritarian succession crisis..." << std::endl;
        AeonEngine aeon;
        aeon.init(42);

        AeonCivilization civ;
        civ.id = 0;
        civ.name = "NORDRA";
        civ.government = GovForm::DICTATORSHIP;
        civ.military_loyalty = 85.0f;
        civ.democratic_institution_strength = 10.0f;

        AeonCharacter dead_dictator;
        dead_dictator.name = "Supreme Autocrat Thorne I";

        engine.handle_authoritarian_succession(civ, dead_dictator, aeon.characters, aeon, 2100);

        std::cout << "  -> Ruler count: " << aeon.characters.size()
                  << " | Current Ruler ID: " << civ.ruler_id << std::endl;

        if (civ.ruler_id >= 0) {
            std::cout << "  ✅ PASSED: Succession crisis executed and new leadership established." << std::endl;
            passed++;
        } else {
            std::cout << "  ❌ FAILED: Succession crisis left realm without ruler." << std::endl;
        }
    }


    // ─────────────────────────────────────────────────────────────────────────
    // TEST 9: Junta to Dictatorship prerequisite validation
    // ─────────────────────────────────────────────────────────────────────────
    {
        std::cout << "\n[TEST 9] Junta to Dictatorship prerequisite validation..." << std::endl;
        AeonCivilization civ;
        civ.id = 0;
        civ.name = "KORROTH";
        civ.government = GovForm::MILITARY_JUNTA;
        civ.years_current_gov = 1; // Only 1 year in power (requires >= 5)
        civ.ruler_authority = 75.0f;
        civ.military_loyalty = 85.0f;
        civ.democratic_institution_strength = 20.0f;

        AeonCharacter ruler;
        ruler.trait = RulerTrait::AUTHORITARIAN;

        std::vector<AeonCivilization> all_civs = { civ };
        auto actions = engine.evaluate_government_actions(civ, all_civs, &ruler, 2100);

        float dict_score = 0.0f;
        for (const auto& a : actions) {
            if (a.action_type == "PROCLAIM_DICTATORSHIP") dict_score = a.total_score;
        }

        std::cout << "  -> Dictatorship Score for 1-year Junta: " << dict_score << std::endl;
        if (dict_score <= 0.001f) {
            std::cout << "  ✅ PASSED: Junta must consolidate for >= 5 years before proclaiming dictatorship." << std::endl;
            passed++;
        } else {
            std::cout << "  ❌ FAILED: Dictatorship allowed too early without prerequisite duration." << std::endl;
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // TEST 10: SCENARIO D — Successful coup consequences
    // Expected: After coup, unrest UP, stability DOWN, crisis_state = MILITARY_REGIME
    // The old code gave +20 stability: this test verifies the correct realistic consequences.
    // ─────────────────────────────────────────────────────────────────────────
    {
        std::cout << "\n[TEST 10] SCENARIO D: Successful coup consequence verification..." << std::endl;
        AeonEngine aeon;
        aeon.init(42);

        auto& target_civ = aeon.civs[0];
        target_civ.government = GovForm::REPUBLIC;
        target_civ.stability = 30.0f;
        target_civ.unrest = 60.0f;
        target_civ.army_size = 20000.0f;
        target_civ.military_loyalty = 85.0f;
        target_civ.legitimacy = 40.0f;
        target_civ.democratic_institution_strength = 30.0f;

        float pre_stability = target_civ.stability;
        float pre_unrest    = target_civ.unrest;

        GovernmentTransitionEngine gtrans;
        bool applied = gtrans.apply_transition("MILITARY_COUP", target_civ, aeon.civs, aeon.characters, aeon, 2100);

        AeonCivilization& post = aeon.civs[0];
        std::cout << "  -> Applied: " << (applied ? "YES" : "NO") << std::endl;
        std::cout << "  -> Government: " << gov_form_name(post.government) << std::endl;
        std::cout << "  -> Stability: " << pre_stability << " -> " << post.stability << " (expected decrease)" << std::endl;
        std::cout << "  -> Unrest:    " << pre_unrest << " -> " << post.unrest << " (expected increase)" << std::endl;
        std::cout << "  -> Crisis State: " << crisis_state_name(post.crisis_state) << std::endl;

        bool correct = applied
            && post.government == GovForm::MILITARY_JUNTA
            && post.stability < pre_stability   // Coup shocks stability down
            && post.unrest > pre_unrest         // Coup raises unrest
            && post.crisis_state == CrisisState::MILITARY_REGIME
            && post.recent_coup_year == 2100;

        if (correct) {
            std::cout << "  ✅ PASSED: Coup produces realistic destabilization, not a stability boost." << std::endl;
            passed++;
        } else {
            std::cout << "  ❌ FAILED: Coup consequences incorrect (check stability/unrest direction and crisis_state)." << std::endl;
        }
    }

    std::cout << "\n========================================================================\n";
    std::cout << "  🎯 TEST RESULTS: " << passed << " / " << total << " PASSED (" << (passed * 100 / total) << "%)\n";
    std::cout << "========================================================================\n\n";

    return (passed == total);
}

} // namespace Aeon
