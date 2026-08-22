#include "aeon_civilization.h"
#include <algorithm>
#include <iostream>
#include <cmath>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  City tick
// ─────────────────────────────────────────────────────────────────────────────
void AeonCity::tick_year(float food_bonus, float tech_mod) {
    if (is_ruins) return;

    // Organic population growth bounded by city capacity
    float growth_rate = std::clamp(0.010f + food_bonus * 0.005f + tech_mod * 0.003f, -0.05f, 0.03f);
    population = static_cast<long long>(population * (1.0f + growth_rate));
    population = std::clamp(population, 100LL, 15000000LL); // Megacity cap 15M

    // Happiness affected by food and crime
    happiness = std::min(100.0f, happiness + (food_supply - 70.0f) * 0.02f);
    happiness = std::max(20.0f, happiness - crime_rate * 0.05f);

    upgrade_tier();
}

void AeonCity::upgrade_tier() {
    if      (population >= 1000000 && !is_ruins) tier = CityTier::MEGACITY;
    else if (population >= 200000)               tier = CityTier::INDUSTRIAL;
    else if (population >= 50000)                tier = CityTier::CITY;
    else if (population >= 5000)                 tier = CityTier::TOWN;
    else                                         tier = CityTier::VILLAGE;
}

char AeonCity::map_symbol() const {
    if (is_ruins)             return 'R';
    if (is_capital)           return '@';
    switch (tier) {
        case CityTier::MEGACITY:   return 'M';
        case CityTier::INDUSTRIAL: return 'C';
        case CityTier::CITY:       return 'c';
        case CityTier::TOWN:       return 't';
        default:                   return '.';
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Carrying capacity calculation based on territory, technology, and food
// ─────────────────────────────────────────────────────────────────────────────
long long AeonCivilization::get_carrying_capacity() const {
    if (is_commons) return 2000000000LL; // 2 Billion global commons cap

    double base_per_tile = 35000.0; // 35,000 people per territory tile
    double tech_mult = 1.0 + double(int(tech.era)) * 0.35; // Technology increases agricultural/urban efficiency
    double food_factor = std::clamp(double(resources.food) / 500.0, 0.3, 2.5);

    double capacity = double(std::max(10.0f, territory_tiles)) * base_per_tile * tech_mult * food_factor;
    return static_cast<long long>(std::clamp(capacity, 10000.0, 800000000.0)); // Cap at 800 Million max
}

// ─────────────────────────────────────────────────────────────────────────────
//  Civilization tick
// ─────────────────────────────────────────────────────────────────────────────
void AeonCivilization::tick_year(int year, float /*dt_years*/) {
    if (is_alive <= 0.0f) return;

    // ── 1. Realistic Logistic Population Dynamics ─────────────────────────────
    long long K = get_carrying_capacity();
    double current_pop = std::max(100.0, (double)population.total);

    // Logistic growth rate: r * N * (1 - N / K)
    double base_r = std::clamp((double)population.birth_rate - (double)population.death_rate, 0.005, 0.025);
    if (resources.food < 200.0f) base_r -= 0.015; // Famine shock

    // Logistic factor
    double logistic_factor = 1.0 - (current_pop / (double)K);
    double pop_delta = base_r * current_pop * logistic_factor;

    // If over carrying capacity, starvation and overcrowding reduce population
    if (current_pop > (double)K) {
        double excess = current_pop - (double)K;
        pop_delta = -excess * 0.08; // 8% attrition per year until equilibrium
    }

    population.total = static_cast<long long>(std::clamp(current_pop + pop_delta, 0.0, 800000000.0));

    if (population.total <= 0) {
        is_alive = 0.0f;
        std::cout << "[YEAR " << year << "] EXTINCTION: " << name << " has collapsed and become extinct." << std::endl;
        return;
    }

    // ── 2. Resource production & consumption ──────────────────────────────────
    float food_produced = float(std::min(population.total, 5000000LL)) * 0.00015f + territory_tiles * 2.0f;
    float food_consumed = float(std::min(population.total, 5000000LL)) * 0.00014f;
    resources.food      = std::clamp(resources.food + food_produced - food_consumed, 0.0f, 10000.0f);
    resources.wood      = std::clamp(resources.wood + territory_tiles * 0.1f, 0.0f, 10000.0f);
    resources.stone     = std::clamp(resources.stone + territory_tiles * 0.05f, 0.0f, 10000.0f);

    // ── 3. Trade Agreement Lifecycle & Income ─────────────────────────────────
    // ── 3. Trade Agreement Lifecycle & Income ─────────────────────────────────
    std::vector<int> expired_trades;
    for (auto& kv : active_trade_agreements) {
        kv.second--; // Decrement years remaining
        if (kv.second <= 0) {
            expired_trades.push_back(kv.first);
        } else {
            // Annual trade dividend
            economy.gdp += 15.0f;
            economy.annual_income += 5.0f;
        }
    }
    for (int partner_id : expired_trades) {
        active_trade_agreements.erase(partner_id);
    }

    // ── 4. Dynamic Economy Growth with Logistic Diminishing Returns ───────────
    // Diminishing returns curve: growth efficiency smoothly drops as GDP reaches industrial scale
    float gdp_efficiency = 1.0f / (1.0f + std::pow(economy.gdp / 100000.0f, 0.85f));
    float effective_growth = std::clamp(economy.gdp_growth * gdp_efficiency, -0.05f, 0.05f);

    // Population labor contribution: GDP per capita scaling
    float pop_labor = float(std::min(population.total, 20000000LL)) * 0.00002f;
    economy.gdp += pop_labor * (1.0f + float(int(tech.era)) * 0.25f);
    economy.gdp *= (1.0f + effective_growth);
    economy.gdp = std::max(50.0f, economy.gdp); // Bounded naturally without hard saturation clamp
    economy.inflation = std::clamp(economy.inflation + (resources.food < 300.0f ? 0.005f : -0.002f), 0.0f, 0.30f);

    // ── 5. Military Mobilization & Reserves Dynamics ──────────────────────────
    reserve_pool = float(std::min(population.total, 10000000LL)) * 0.04f; // 4% of pop is eligible reserve
    if (at_war) {
        mobilization_level = std::min(1.0f, mobilization_level + 0.35f); // Mobilize reserves for war
        war_exhaustion = std::min(100.0f, war_exhaustion + 4.5f);
    } else {
        mobilization_level = std::max(0.0f, mobilization_level - 0.25f); // Peacetime demobilization
        war_exhaustion = std::max(0.0f, war_exhaustion - 5.0f);
    }
    army_size = standing_army + (reserve_pool * mobilization_level);

    // ── 6. Technology research accumulates ────────────────────────────────────
    float research_bonus = population.education_lvl * 0.01f;
    tech.research_pts += 10.0f + economy.gdp * 0.003f + research_bonus;
    float era_cost = 600.0f + float(int(tech.era)) * 1200.0f;
    tech.progress = std::clamp((tech.research_pts / era_cost) * 100.0f, 0.0f, 100.0f);

    if (tech.progress >= 100.0f && tech.era != TechEra::ADVANCED) {
        tech.era = TechEra(int(tech.era) + 1);
        tech.research_pts = 0.0f;
        tech.progress = 0.0f;
        std::cout << "[YEAR " << year << "] 🔬 TECH BREAKTHROUGH: " << name
                  << " has entered the " << tech_era_name(tech.era) << " era!" << std::endl;
    }

    // ── 7. Dynamic Unrest & Stability Dynamics ────────────────────────────────
    float unrest_delta = 0.0f;
    if (war_exhaustion > 30.0f) unrest_delta += (war_exhaustion - 30.0f) * 0.08f;
    if (corruption > 30.0f)     unrest_delta += (corruption - 30.0f) * 0.05f;
    if (legitimacy < 50.0f)     unrest_delta += (50.0f - legitimacy) * 0.06f;
    if (!at_war && resources.food > 400.0f && population.happiness > 60.0f) {
        unrest_delta -= 3.0f; // Peacetime prosperity quells unrest
    }
    unrest = std::clamp(unrest + unrest_delta, 0.0f, 100.0f);

    // ── 8. Happiness & Stability Dynamics ─────────────────────────────────────
    float happiness_delta = 0.0f;
    happiness_delta += (economy.gdp > 1000.0f) ? 0.4f : -0.2f;
    happiness_delta += (resources.food > 500.0f) ? 0.4f : -0.5f;
    if (at_war) happiness_delta -= 0.8f;
    population.happiness = std::clamp(population.happiness + happiness_delta, 15.0f, 100.0f);

    float stability_delta = 0.0f;
    if (!at_war)                       stability_delta += 1.5f; // Peacetime healing
    if (population.happiness > 60.0f)  stability_delta += 0.8f;
    if (population.happiness < 40.0f)  stability_delta -= 1.2f;
    if (unrest > 50.0f)                stability_delta -= (unrest - 50.0f) * 0.05f;
    if (at_war)                        stability_delta -= 1.8f;

    stability = std::clamp(stability + stability_delta, 10.0f, 100.0f);

    // Determine Crisis State
    if (stability < 25.0f || unrest > 75.0f) crisis_state = CrisisState::POLITICAL_CRISIS;
    else if (at_war && war_exhaustion > 60.0f) crisis_state = CrisisState::WAR_CRISIS;
    else if (economy.gdp < 300.0f || resources.food < 150.0f) crisis_state = CrisisState::ECONOMIC_CRISIS;
    else if (unrest > 50.0f) crisis_state = CrisisState::CIVIL_UNREST;
    else crisis_state = CrisisState::NORMAL;
}

} // namespace Aeon
