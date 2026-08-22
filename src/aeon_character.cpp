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

} // namespace Aeon
