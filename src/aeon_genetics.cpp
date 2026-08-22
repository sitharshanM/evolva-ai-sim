#include "aeon_genetics.h"
#include "aeon_engine.h"
#include <algorithm>

namespace Aeon {

const char* genetic_trait_name(GeneticTrait trait) {
    switch (trait) {
        case GeneticTrait::HIGH_ALTITUDE_HYPOXIA: return "🏔️ High-Altitude Hypoxia Resistance";
        case GeneticTrait::POLAR_COLD_RESISTANCE:  return "❄️ Polar Thermal Tolerance";
        case GeneticTrait::IMMUNE_SUPER_RESILIENCE:return "🛡️ Immune Super-Resilience";
        case GeneticTrait::COGNITIVE_AMPLIFICATION:return "🧠 Cognitive Amplification";
        case GeneticTrait::CYBERNETIC_SYNTHESIS:   return "🦾 Cybernetic Bio-Synthesis";
    }
    return "Unknown Trait";
}

void AeonGeneticsEngine::init(const AeonEngine& engine) {
    profiles.clear();
    for (const auto& civ : engine.civs) {
        EmpireGeneticsProfile prof;
        prof.civ_id = civ.id;
        prof.civ_name = civ.name;
        prof.bio_tech_level = 1.0f + (civ.id * 0.1f);
        prof.lifespan_years = 75.0f;
        prof.cognitive_research_mult = 1.0f;
        prof.disease_resistance_pct = 50.0f;
        prof.soldier_gene_power_mult = 1.0f;
        profiles.push_back(prof);
    }
}

void AeonGeneticsEngine::tick_year(AeonEngine& engine) {
    for (auto& prof : profiles) {
        if (prof.civ_id < 0 || prof.civ_id >= (int)engine.civs.size()) continue;
        auto& civ = engine.civs[prof.civ_id];
        if (civ.is_alive <= 0.0f) continue;

        // Advance bio-tech level if science is high
        if (civ.science_pref > 0.6f) {
            prof.bio_tech_level += 0.05f;
        }

        // Apply bio-engineering perks
        if (prof.cognitive_research_mult > 1.0f) {
            civ.tech.progress += (prof.cognitive_research_mult - 1.0f) * 2.5f;
        }
        if (prof.soldier_gene_power_mult > 1.0f) {
            civ.military_power *= prof.soldier_gene_power_mult;
        }

        // Random beneficial mutation event
        if (prof.bio_tech_level > 2.0f && prof.active_traits.size() < 3) {
            if ((engine.year % 7) == prof.civ_id) {
                unlock_trait(prof.civ_id, GeneticTrait::IMMUNE_SUPER_RESILIENCE, engine);
            }
        }
    }
}

bool AeonGeneticsEngine::unlock_trait(int civ_id, GeneticTrait trait, AeonEngine& engine) {
    for (auto& prof : profiles) {
        if (prof.civ_id == civ_id) {
            for (auto t : prof.active_traits) {
                if (t == trait) return false; // already unlocked
            }
            prof.active_traits.push_back(trait);
            switch (trait) {
                case GeneticTrait::HIGH_ALTITUDE_HYPOXIA:
                    prof.soldier_gene_power_mult += 0.15f;
                    break;
                case GeneticTrait::POLAR_COLD_RESISTANCE:
                    prof.lifespan_years += 5.0f;
                    break;
                case GeneticTrait::IMMUNE_SUPER_RESILIENCE:
                    prof.disease_resistance_pct = 95.0f;
                    break;
                case GeneticTrait::COGNITIVE_AMPLIFICATION:
                    prof.cognitive_research_mult += 0.35f;
                    break;
                case GeneticTrait::CYBERNETIC_SYNTHESIS:
                    prof.soldier_gene_power_mult += 0.40f;
                    prof.cognitive_research_mult += 0.50f;
                    break;
            }
            engine.history.record(engine.year, engine.month, "SCIENCE",
                prof.civ_name + " Unlocks " + std::string(genetic_trait_name(trait)),
                "Bio-engineering breakthrough modifies human genome for enhanced survival.", civ_id);
            return true;
        }
    }
    return false;
}

} // namespace Aeon
