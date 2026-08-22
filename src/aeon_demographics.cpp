#include "aeon_demographics.h"
#include "aeon_engine.h"
#include <algorithm>
#include <cmath>

namespace Aeon {

AeonDemographicsEngine::AeonDemographicsEngine() {
    init_default_demographics();
}

void AeonDemographicsEngine::init_default_demographics() {
    pyramids.clear();
    refugees.clear();

    PopulationPyramid p1;
    p1.civ_id = 0;
    p1.civ_name = "United Republic of Sol";
    p1.youth_count = 1450000;
    p1.working_count = 4100000;
    p1.senior_count = 950000;
    p1.birth_rate_per_1000 = 13.8f;
    p1.death_rate_per_1000 = 8.1f;

    PopulationPyramid p2;
    p2.civ_id = 1;
    p2.civ_name = "Nordra Dominion";
    p2.youth_count = 980000;
    p2.working_count = 2800000;
    p2.senior_count = 1100000;
    p2.birth_rate_per_1000 = 10.2f;
    p2.death_rate_per_1000 = 9.8f;

    pyramids.push_back(p1);
    pyramids.push_back(p2);

    global_pandemic.active = false;
    global_pandemic.pathogen_name = "None";
}

void AeonDemographicsEngine::trigger_outbreak(const std::string& pathogen, long long initial_cases) {
    global_pandemic.active = true;
    global_pandemic.pathogen_name = pathogen;
    global_pandemic.R0 = 2.8f;
    global_pandemic.susceptible = 10000000;
    global_pandemic.exposed = initial_cases / 2;
    global_pandemic.infectious = initial_cases;
    global_pandemic.recovered = 0;
    global_pandemic.mortality_rate = 0.035f;
    global_pandemic.quarantine_active = false;
    global_pandemic.vaccine_progress = 0.0f;
}

void AeonDemographicsEngine::update_demographics_year(AeonEngine& engine) {
    // Ensure all civs have a population pyramid record
    for (const auto& civ : engine.civs) {
        if (civ.is_commons) continue;
        bool exists = false;
        for (const auto& p : pyramids) {
            if (p.civ_id == civ.id) { exists = true; break; }
        }
        if (!exists) {
            PopulationPyramid np;
            np.civ_id = civ.id;
            np.civ_name = civ.name;
            np.working_count = static_cast<long long>(civ.population.total * 0.60f);
            np.youth_count   = static_cast<long long>(civ.population.total * 0.25f);
            np.senior_count  = static_cast<long long>(civ.population.total * 0.15f);
            np.birth_rate_per_1000 = 12.0f;
            np.death_rate_per_1000 = 8.5f;
            pyramids.push_back(np);
        }
    }

    // 1. Natural births & aging transitions
    for (auto& p : pyramids) {
        long long new_births = (long long)((p.working_count + p.youth_count) * (p.birth_rate_per_1000 / 1000.0f));
        long long youth_aging = (long long)(p.youth_count / 18.0f);
        long long working_aging = (long long)(p.working_count / 46.0f);
        long long senior_deaths = (long long)(p.senior_count * (p.death_rate_per_1000 / 1000.0f) * 1.5f);

        p.youth_count += new_births - youth_aging;
        p.working_count += youth_aging - working_aging;
        p.senior_count += working_aging - senior_deaths;

        p.youth_count = std::max(10000LL, p.youth_count);
        p.working_count = std::max(50000LL, p.working_count);
        p.senior_count = std::max(5000LL, p.senior_count);

        if (p.working_count > 0) {
            p.dependency_ratio = (float)(p.youth_count + p.senior_count) / (float)p.working_count;
        }
    }

    // 2. Refugee Waves caused by war or disaster events
    refugees.clear();
    for (const auto& civ : engine.civs) {
        if (civ.is_commons) continue;
        if (civ.at_war) {
            // Find neighbor civ to receive refugees
            int target_id = (civ.id + 1) % std::max(1, (int)engine.civs.size());
            if (target_id == civ.id) target_id = (civ.id + 2) % (int)engine.civs.size();

            RefugeeWave rw;
            rw.origin_civ_id = civ.id;
            rw.target_civ_id = target_id;
            rw.displacement_count = 45000 + (rand() % 30000);
            rw.reason = "Conflict Displacement";
            refugees.push_back(rw);

            // Shift population from origin to target
            for (auto& p : pyramids) {
                if (p.civ_id == civ.id) {
                    p.working_count = std::max(10000LL, p.working_count - rw.displacement_count);
                } else if (p.civ_id == target_id) {
                    p.working_count += rw.displacement_count;
                }
            }
        }
    }

    // 3. SEIR Pandemic Dynamics
    if (global_pandemic.active) {
        float beta = global_pandemic.R0 * 0.15f; // transmission rate
        if (global_pandemic.quarantine_active) beta *= 0.40f; // quarantine lowers transmission

        float gamma = 0.20f; // recovery rate

        long long new_exposed = (long long)(beta * global_pandemic.susceptible * global_pandemic.infectious / std::max(1.0f, (float)(global_pandemic.susceptible + global_pandemic.exposed + global_pandemic.infectious + global_pandemic.recovered)));
        long long new_infectious = (long long)(0.33f * global_pandemic.exposed);
        long long new_recovered = (long long)(gamma * global_pandemic.infectious);
        long long new_deaths = (long long)(new_recovered * global_pandemic.mortality_rate);

        global_pandemic.susceptible = std::max(0LL, global_pandemic.susceptible - new_exposed);
        global_pandemic.exposed = std::max(0LL, global_pandemic.exposed + new_exposed - new_infectious);
        global_pandemic.infectious = std::max(0LL, global_pandemic.infectious + new_infectious - new_recovered - new_deaths);
        global_pandemic.recovered += new_recovered;

        // Vaccine progress tick
        global_pandemic.vaccine_progress += 0.20f;
        if (global_pandemic.vaccine_progress >= 1.0f) {
            global_pandemic.active = false; // Disease eradicated by vaccine
        }
    }
}

float AeonDemographicsEngine::calculate_transit_delay_hours(float distance_km, float terrain_cost_mult) const {
    float base_speed_kmh = 60.0f; // 60 km/h commercial transit speed
    float effective_speed = base_speed_kmh / std::max(0.1f, terrain_cost_mult);
    return distance_km / effective_speed;
}

// ─── update_urbanization ──────────────────────────────────────────────────────
void AeonDemographicsEngine::update_urbanization(AeonEngine& engine) {
    for (auto& civ : engine.civs) {
        if (civ.is_alive <= 0.0f) continue;
        auto& pop = civ.population;
        // Industry proxy: more cities + higher tech era = more industry
        int num_cities = (int)civ.city_ids.size();
        float industry_proxy = num_cities * 5.0f
            + (int)civ.tech.era * 10.0f;

        // Pull factor: industry level drives rural → urban migration
        // Push factor: food insecurity pushes rural people off the land
        float migration_rate = (industry_proxy / 100.0f) * 0.01f
                             + (100.0f - pop.food_security) * 0.0005f;
        migration_rate = std::min(migration_rate, 0.05f); // cap 5% per year

        long long migrants = static_cast<long long>(pop.rural_population * migration_rate);
        migrants = std::min(migrants, pop.rural_population / 10); // cap 10%/yr
        pop.rural_population  -= migrants;
        pop.urban_population  += migrants;
        pop.urban_pct = (float)pop.urban_population / (float)std::max(1LL, pop.total) * 100.0f;
        pop.rural_pct = 100.0f - pop.urban_pct;

        // Urbanization feedback: cities generate more research, more crime
        float urban_edu_boost = pop.urban_pct * 0.005f;
        pop.education_lvl = std::min(100.0f, pop.education_lvl + urban_edu_boost);
    }
}

// ─── apply_inequality_feedback ────────────────────────────────────────────────
void AeonDemographicsEngine::apply_inequality_feedback(AeonEngine& engine) {
    for (auto& civ : engine.civs) {
        if (civ.is_alive <= 0.0f) continue;
        float gini = civ.economy.wealth_inequality; // 0-1
        // High inequality suppresses happiness and boosts protest risk
        civ.population.happiness -= (gini - 0.35f) * 20.0f;
        civ.population.happiness = std::max(0.0f, civ.population.happiness);
        // Inequality also suppresses birth rates in upper class but raises in poor
        if (gini > 0.5f) {
            civ.population.birth_rate += 0.002f;  // higher fertility in poverty
            civ.population.death_rate += 0.001f;  // and higher mortality
        } else {
            civ.population.birth_rate -= 0.001f;  // demographic transition
        }
        // Instability from extreme inequality
        if (gini > 0.65f) civ.stability -= 3.0f;
    }
}

// ─── apply_generational_education ────────────────────────────────────────────
void AeonDemographicsEngine::apply_generational_education(AeonEngine& engine) {
    for (auto& civ : engine.civs) {
        if (civ.is_alive <= 0.0f) continue;
        auto& pop = civ.population;
        // Each generation: educated parents → higher baseline for children
        // Education investment is compounded slowly (30-year cycle)
        float gen_compound = pop.education_lvl * 0.003f;
        pop.education_lvl = std::min(100.0f, pop.education_lvl + gen_compound);
        // Education reduces birth rate (demographic transition)
        if (pop.education_lvl > 60.0f) {
            pop.birth_rate = std::max(0.008f, pop.birth_rate - 0.001f);
        }
        // Educated workforce boosts research
        civ.tech.research_pts += pop.working_count * (pop.education_lvl / 5000.0f);
        // Infant mortality falls with education and health access
        pop.infant_mortality = std::max(2.0f, pop.infant_mortality - (pop.education_lvl * 0.1f));
    }
}

// ─── process_refugee_intake ───────────────────────────────────────────────────
void AeonDemographicsEngine::process_refugee_intake(AeonEngine& engine) {
    for (auto& wave : refugees) {
        if (wave.displacement_count <= 0) continue;
        // Receiving civ gets population boost but also strain
        if (wave.target_civ_id < 0 || wave.target_civ_id >= (int)engine.civs.size()) continue;
        auto& recv = engine.civs[wave.target_civ_id];
        recv.population.total        += wave.displacement_count;
        recv.population.urban_population += wave.displacement_count; // refugees crowd cities
        recv.population.happiness    -= (float)(wave.displacement_count / 100000) * 2.0f;
        recv.economy.unemployment    += (float)(wave.displacement_count) / (float)recv.population.total * 50.0f;
        recv.population.ethnic_tension += 5.0f;
        // Drain the wave
        wave.displacement_count = 0;
    }
    refugees.erase(std::remove_if(refugees.begin(), refugees.end(),
        [](const RefugeeWave& w){ return w.displacement_count <= 0; }), refugees.end());
}

// ─── advance_pandemic_season ──────────────────────────────────────────────────
void AeonDemographicsEngine::advance_pandemic_season(AeonEngine& engine, int month) {
    if (!global_pandemic.active) return;
    // Seasonal spread: disease spreads faster in winter/autumn (months 9-12, 1-2)
    int season = (month / 3) % 4; // 0=Spring 1=Summer 2=Autumn 3=Winter
    const float season_mults[4] = { 0.9f, 0.7f, 1.1f, 1.4f };
    global_pandemic.seasonal_spread_mult = season_mults[season];
    float effective_R0 = global_pandemic.R0 * global_pandemic.seasonal_spread_mult;

    for (auto& civ : engine.civs) {
        if (civ.is_alive <= 0.0f) continue;
        if (global_pandemic.infectious <= 0) continue;
        // SEIR tick
        long long new_exposed = static_cast<long long>(
            global_pandemic.infectious * effective_R0 * 0.1f);
        new_exposed = std::min(new_exposed, global_pandemic.susceptible);
        long long new_infectious = global_pandemic.exposed / 5;
        long long new_recovered  = global_pandemic.infectious / 8;
        long long deaths = static_cast<long long>(
            global_pandemic.infectious * global_pandemic.mortality_rate * 0.01f);

        global_pandemic.susceptible -= new_exposed;
        global_pandemic.exposed     += new_exposed - new_infectious;
        global_pandemic.infectious  += new_infectious - new_recovered - deaths;
        global_pandemic.recovered   += new_recovered;
        if (global_pandemic.infectious < 0) global_pandemic.infectious = 0;

        // Vaccine progress suppresses spread
        if (global_pandemic.vaccine_progress > 0.5f) {
            global_pandemic.infectious = static_cast<long long>(
                global_pandemic.infectious * (1.0f - global_pandemic.vaccine_progress * 0.3f));
        }

        // Deaths hit civ population
        civ.population.total -= deaths;
        civ.population.health -= (float)deaths / (float)std::max(1LL, civ.population.total) * 500.0f;
        civ.population.happiness -= 3.0f;

        if (global_pandemic.infectious < 100) {
            global_pandemic.active = false;
            engine.history.record(engine.year, engine.month, "DISASTER",
                global_pandemic.pathogen_name + " pandemic ends",
                "Disease finally contained after widespread casualties.", -1);
        }
        break; // Single global pandemic shared across civs
    }
}

} // namespace Aeon
