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

    // ── 9. Provincial Yields & Resistance Tick ───────────────────────────────
    if (provinces.empty()) {
        init_default_provinces();
    }
    calculate_provincial_yields();

    // ── 10. Historical Memory & Revanchism Decay ──────────────────────────────
    for (auto& kv : bilateral_relations) {
        auto& rel = kv.second;
        rel.hatred = std::max(0.0f, rel.hatred - 0.35f);
        rel.fear   = std::max(0.0f, rel.fear - 0.70f);
        rel.war_memory_weight = std::max(0.0f, rel.war_memory_weight - 0.50f);
        if (rel.border_claim_score > 0.0f) {
            // Revanchism persists as long as provinces remain lost
            rel.border_claim_score = std::min(100.0f, rel.border_claim_score + 0.2f);
        }
    }
}

// ─── init_default_provinces ──────────────────────────────────────────────────
void AeonCivilization::init_default_provinces() {
    provinces.clear();

    auto add_prov = [&](int pid, const std::string& pname, long long pop, float pgdp, int pfac,
                        bool port, bool iron, bool oil, bool uranium, int mx, int my) {
        Province p;
        p.id = pid;
        p.name = pname;
        p.civ_id = id;
        p.original_civ_id = id;
        p.population = pop;
        p.gdp = pgdp;
        p.factories = pfac;
        p.stability = stability;
        p.unrest = unrest;
        p.loyalty = 85.0f;
        p.dominant_culture = name;
        p.has_port = port;
        p.has_strategic_iron = iron;
        p.has_oil_field = oil;
        p.has_uranium = uranium;
        p.map_x = mx;
        p.map_y = my;
        p.resource_yield.food = float(pop) * 0.0001f;
        p.resource_yield.iron = iron ? 40.0f : 5.0f;
        p.resource_yield.oil = oil ? 30.0f : 2.0f;
        p.resource_yield.uranium = uranium ? 15.0f : 0.0f;
        provinces.push_back(p);
    };

    if (id == 0) { // NORDRA
        add_prov(1, "Arkan Valley", 140000, 250.0f, 6, false, true, false, false, capital_x - 3, capital_y + 1);
        add_prov(2, "Nordran Heartland", 180000, 320.0f, 8, false, false, false, false, capital_x, capital_y);
        add_prov(3, "Krag Coast", 110000, 210.0f, 4, true, false, false, false, capital_x + 4, capital_y - 2);
        add_prov(4, "Frostmark Highlands", 83000, 140.0f, 3, false, false, false, true, capital_x - 2, capital_y - 4);
    } else if (id == 1) { // ELDORIA
        add_prov(5, "Eldorian Cradle", 170000, 300.0f, 7, true, false, false, false, capital_x, capital_y);
        add_prov(6, "Silverwood", 130000, 240.0f, 5, false, false, false, false, capital_x + 2, capital_y + 3);
        add_prov(7, "High Terrace", 120000, 210.0f, 4, false, true, false, false, capital_x - 3, capital_y - 2);
        add_prov(8, "Whispering Vale", 93000, 180.0f, 3, false, false, false, false, capital_x + 4, capital_y);
    } else if (id == 2) { // VALORIA
        add_prov(9, "Valorian Glade", 180000, 290.0f, 6, false, false, false, false, capital_x, capital_y);
        add_prov(10, "Mistral Pass", 110000, 200.0f, 4, false, true, false, false, capital_x - 2, capital_y + 3);
        add_prov(11, "Azure Shore", 130000, 260.0f, 5, true, false, false, false, capital_x + 3, capital_y - 2);
        add_prov(12, "Greenfell", 93000, 170.0f, 3, false, false, false, false, capital_x - 4, capital_y - 1);
    } else if (id == 3) { // DRAKOR
        add_prov(13, "Drakor Caldera", 160000, 280.0f, 7, false, true, false, false, capital_x, capital_y);
        add_prov(14, "Obsidian Peaks", 120000, 210.0f, 5, false, false, false, true, capital_x - 3, capital_y + 2);
        add_prov(15, "Iron Basin", 140000, 230.0f, 6, false, true, false, false, capital_x + 3, capital_y - 3);
        add_prov(16, "Bloodmarsh", 93000, 150.0f, 3, false, false, true, false, capital_x + 2, capital_y + 4);
    } else if (id == 4) { // SOLARIA
        add_prov(17, "Solarian Plains", 190000, 310.0f, 7, false, false, false, false, capital_x, capital_y);
        add_prov(18, "Sunspire", 160000, 290.0f, 6, true, false, false, false, capital_x + 4, capital_y + 1);
        add_prov(19, "Golden Reach", 130000, 220.0f, 4, false, false, true, false, capital_x - 3, capital_y - 2);
        add_prov(20, "Amber Coast", 106000, 190.0f, 4, true, false, false, false, capital_x - 1, capital_y + 4);
    } else if (is_commons) { // THE COMMONS
        add_prov(21, "Grand Financial District", 40000000LL, 3500.0f, 30, true, false, false, false, capital_x, capital_y);
        add_prov(22, "Common Market Basin", 35000000LL, 2800.0f, 25, false, false, false, false, capital_x + 2, capital_y + 2);
        add_prov(23, "Free Port Terminals", 27929000LL, 2200.0f, 20, true, false, false, false, capital_x - 2, capital_y - 2);
    } else {
        // Splinter or newly emerged realm
        add_prov(id * 10 + 1, name + " Heartland", 150000, 250.0f, 5, false, false, false, false, capital_x, capital_y);
        add_prov(id * 10 + 2, name + " Borderland", 90000, 160.0f, 3, false, true, false, false, capital_x + 2, capital_y + 1);
        add_prov(id * 10 + 3, name + " Outskirts", 60000, 110.0f, 2, false, false, true, false, capital_x - 2, capital_y - 1);
    }
}

// ─── calculate_provincial_yields ─────────────────────────────────────────────
void AeonCivilization::calculate_provincial_yields() {
    float bonus_gdp = 0.0f;
    float bonus_iron = 0.0f;
    float bonus_oil = 0.0f;
    float bonus_uranium = 0.0f;

    for (auto& prov : provinces) {
        if (!prov.is_occupied) {
            // Province produces normally for its sovereign
            bonus_gdp += prov.factories * 12.0f + (prov.has_port ? 30.0f : 0.0f);
            if (prov.has_strategic_iron) bonus_iron += 25.0f;
            if (prov.has_oil_field)      bonus_oil += 20.0f;
            if (prov.has_uranium)        bonus_uranium += 10.0f;

            // Natural resistance decay in peaceful home provinces
            prov.occupation_resistance = std::max(0.0f, prov.occupation_resistance - 5.0f);
            prov.stability = std::clamp(prov.stability + 0.5f, 0.0f, 100.0f);
        } else {
            // Under foreign occupation!
            // Resistance grows if occupation persists
            prov.occupation_resistance = std::min(100.0f, prov.occupation_resistance + 4.0f);
            prov.stability = std::max(5.0f, prov.stability - 3.0f);
            prov.unrest = std::min(100.0f, prov.unrest + 5.0f);
        }
    }

    economy.manufacturing_output = 100.0f + bonus_gdp;
    economy.gdp += bonus_gdp * 0.15f;
    resources.iron = std::min(10000.0f, resources.iron + bonus_iron);
    resources.oil  = std::min(10000.0f, resources.oil + bonus_oil);
    resources.uranium = std::min(10000.0f, resources.uranium + bonus_uranium);
}


// ─── check_civil_war ─────────────────────────────────────────────────────────
bool AeonCivilization::check_civil_war(int year) {
    (void)year;
    if (is_commons || is_alive <= 0.0f) return false;
    // Condition-driven crisis: Extremely low stability + high exhaustion/unrest/military discontent
    if (stability < 15.0f && (unrest > 55.0f || war_exhaustion > 50.0f || military_discontent > 40.0f)) {
        return true;
    }
    return false;
}

} // namespace Aeon
