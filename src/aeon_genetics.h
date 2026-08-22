#pragma once
#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

enum class GeneticTrait {
    HIGH_ALTITUDE_HYPOXIA = 0,
    POLAR_COLD_RESISTANCE  = 1,
    IMMUNE_SUPER_RESILIENCE= 2,
    COGNITIVE_AMPLIFICATION= 3,
    CYBERNETIC_SYNTHESIS   = 4
};

const char* genetic_trait_name(GeneticTrait trait);

struct EmpireGeneticsProfile {
    int civ_id = 0;
    std::string civ_name;
    float bio_tech_level = 1.0f;
    float mutation_rate = 0.02f;
    float lifespan_years = 75.0f;
    float cognitive_research_mult = 1.0f;
    float disease_resistance_pct = 50.0f;
    float soldier_gene_power_mult = 1.0f;
    std::vector<GeneticTrait> active_traits;
};

class AeonGeneticsEngine {
public:
    AeonGeneticsEngine() = default;

    void init(const AeonEngine& engine);
    void tick_year(AeonEngine& engine);
    bool unlock_trait(int civ_id, GeneticTrait trait, AeonEngine& engine);

    std::vector<EmpireGeneticsProfile> profiles;
};

} // namespace Aeon
