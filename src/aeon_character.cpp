#include "aeon_character.h"
#include <algorithm>

namespace Aeon {

void AeonCharacter::age_one_year() {
    age++;
    // Health slowly degrades after age 60
    if (age > 60) {
        float decay = (age - 60) * 0.8f;
        health = std::max(0.0f, health - decay * 0.1f);
    }
}

bool AeonCharacter::check_natural_death(int year) const {
    if (age >= MAX_LIFESPAN) return true;
    if (health <= 0.0f) return true;
    // Probabilistic death after 70
    if (age > 70) {
        float death_chance = (age - 70) * 0.03f;
        float roll = float(year * id % 100) / 100.0f; // deterministic pseudo-chance
        if (roll < death_chance) return true;
    }
    return false;
}

bool AeonCharacter::will_refuse_order(float realm_stability, float ruler_popularity) const {
    if (!is_general) return false;
    // Loyalty drops under state collapse or widely despised rulers
    float effective_loyalty = loyalty_to_ruler;
    if (realm_stability < 20.0f) effective_loyalty -= (20.0f - realm_stability) * 0.02f;
    if (ruler_popularity < 25.0f) effective_loyalty -= (25.0f - ruler_popularity) * 0.015f;
    if (political_ambition > 0.70f) effective_loyalty -= 0.15f;
    if (has_trait("Politically Ambitious")) effective_loyalty -= 0.10f;
    return effective_loyalty < 0.30f;
}

bool AeonCharacter::has_trait(const std::string& trait_name) const {
    for (const auto& t : military_traits) {
        if (t == trait_name) return true;
    }
    return false;
}

float AeonCharacter::get_tactical_combat_multiplier() const {
    if (!is_general) return 1.0f;
    float mult = 0.8f + (command_skill * 0.4f); // 0.8 to 1.2
    if (has_trait("Brilliant Strategist")) mult += 0.15f;
    if (has_trait("Popular Commander")) mult += 0.10f;
    if (has_trait("Cautious Tactician")) mult += 0.05f;
    return mult;
}

} // namespace Aeon
