#ifndef AEON_DYNASTY_H
#define AEON_DYNASTY_H

#include "aeon_character.h"
#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct RoyalPerson {
    int id = 0;
    int civ_id = 0;
    std::string name;
    std::string title;
    int age = 25;
    bool is_heir = false;
    bool married = false;
    int spouse_civ_id = -1;
    bool is_alive = true;
    float claim_strength = 100.0f; // 0..100
    float legitimacy = 85.0f;     // 0..100; low legitimacy triggers pretender wars
    bool is_pretender = false;
};

struct PretenderClaimant {
    int claimant_id = 0;
    int civ_id = 0;
    std::string name;
    float military_support_pct = 35.0f; // % of army defecting to claimant
    bool war_declared = false;
};

struct DynasticAlliance {
    int civ1_id = -1;
    int civ2_id = -1;
    std::string marriage_description;
    float trust_bonus = 30.0f;
    bool allows_inheritance_claim = true;
};

class AeonDynastyEngine {
public:
    std::vector<RoyalPerson> royals;
    std::vector<DynasticAlliance> dynastic_marriages;
    std::vector<PretenderClaimant> active_pretenders;

    AeonDynastyEngine();
    void init_dynasties(const AeonEngine& engine);
    void update_dynasties_tick(AeonEngine& engine);
    void arrange_royal_marriage(int civ1_id, int civ2_id, AeonEngine& engine);
    
    SuccessionOutcome evaluate_succession(int civ_id, AeonEngine& engine, int& out_new_ruler_id);
    void trigger_succession_crisis(int civ_id, AeonEngine& engine);
    void resolve_pretender_war(int civ_id, AeonEngine& engine);
};

} // namespace Aeon

#endif // AEON_DYNASTY_H

