#include "aeon_disasters.h"
#include "aeon_engine.h"
#include <iostream>
#include <cstdlib>
#include <algorithm>

namespace Aeon {

void AeonDisasterEngine::init() {
    active_disasters.clear();
    disaster_history_log.clear();
}

void AeonDisasterEngine::tick_year(AeonEngine& engine) {
    // 1. Process Active Disasters
    for (auto& d : active_disasters) {
        if (!d.is_active) continue;

        d.duration_years--;
        if (d.duration_years <= 0) {
            d.is_active = false;
            std::string end_msg = "DISASTER ENDED: " + d.name + " in region (" + std::to_string(d.target_x) + ", " + std::to_string(d.target_y) + ") has subsided.";
            disaster_history_log.push_back(end_msg);
            continue;
        }

        // Apply ongoing year effects
        switch (d.type) {
            case DisasterType::VOLCANIC_ERUPTION:
                // Drop agricultural output globally
                for (auto& civ : engine.civs) {
                    if (civ.is_alive <= 0.0f) continue;
                    civ.resources.food = std::max(0.0f, civ.resources.food - 500.0f * d.severity);
                    civ.stability = std::max(0.0f, civ.stability - 2.0f * d.severity);
                }
                break;

            case DisasterType::BIO_PLAGUE:
                // Reduce population in cities
                for (auto& civ : engine.civs) {
                    if (civ.is_alive <= 0.0f) continue;
                    civ.population.total = static_cast<long long>(civ.population.total * (1.0f - 0.04f * d.severity));
                    civ.stability = std::max(0.0f, civ.stability - 5.0f * d.severity);
                }
                break;

            case DisasterType::SOLAR_FLARE:
                // Disable tech output temporarily
                for (auto& civ : engine.civs) {
                    if (civ.is_alive <= 0.0f) continue;
                    civ.tech.progress = std::max(0.0f, civ.tech.progress - 10.0f);
                }
                break;

            case DisasterType::METEOR_STRIKE:
                // Single event, no additional turn ticks required
                break;

            case DisasterType::MEGA_EARTHQUAKE:
                for (auto& civ : engine.civs) {
                    if (civ.is_alive <= 0.0f) continue;
                    civ.stability = std::max(0.0f, civ.stability - 3.5f * d.severity);
                }
                break;

            case DisasterType::LOCUST_SWARM:
                for (auto& civ : engine.civs) {
                    if (civ.is_alive <= 0.0f) continue;
                    civ.resources.food = std::max(0.0f, civ.resources.food - 350.0f * d.severity);
                }
                break;
        }
    }

    // 2. Random Disaster Trigger Check (12% chance per year)
    if (rand() % 100 < 12) {
        int r = rand() % 4;
        DisasterType dt = static_cast<DisasterType>(r);
        trigger_disaster(engine, dt);
    }
}

void AeonDisasterEngine::trigger_disaster(AeonEngine& engine, DisasterType type, int target_x, int target_y) {
    ActiveDisaster d;
    d.id = static_cast<int>(active_disasters.size() + 1);
    d.type = type;
    d.start_year = engine.year;

    if (target_x < 0 || target_y < 0) {
        d.target_x = 10 + rand() % 40;
        d.target_y = 10 + rand() % 30;
    } else {
        d.target_x = target_x;
        d.target_y = target_y;
    }

    std::string log_entry;
    switch (type) {
        case DisasterType::VOLCANIC_ERUPTION:
            d.name = "Supervolcano Eruption";
            d.duration_years = 4;
            d.severity = 0.8f;
            log_entry = "🌋 CATACLYSM: Supervolcano erupts at (" + std::to_string(d.target_x) + ", " + std::to_string(d.target_y) + ")! Volcanic ash darkens global skies.";
            break;

        case DisasterType::METEOR_STRIKE: {
            d.name = "Meteor Strike";
            d.duration_years = 1;
            d.severity = 1.0f;
            log_entry = "☄️ METEOR IMPACT: Asteroid strikes world coordinate (" + std::to_string(d.target_x) + ", " + std::to_string(d.target_y) + ")! Crater creates Star-Metal Deposit.";
            // Add strategic resource to map
            StrategicNode node{ static_cast<int>(engine.caravan_engine.nodes.size() + 1), glm::vec2(d.target_x, d.target_y), StrategicResourceType::GOLD_VEIN, 25000.0f, -1 };
            engine.caravan_engine.nodes.push_back(node);
            break;
        }

        case DisasterType::BIO_PLAGUE:
            d.name = "Global Bio-Plague";
            d.duration_years = 3;
            d.severity = 0.9f;
            log_entry = "☣️ PANDEMIC OUTBREAK: Contagious viral plague spreads across international trade routes!";
            break;

        case DisasterType::SOLAR_FLARE:
            d.name = "Solar EMP Flare Storm";
            d.duration_years = 2;
            d.severity = 0.7f;
            log_entry = "⚡ SOLAR FLARE: Massive EMP solar storm disables space satellites & missile guidance systems.";
            break;

        case DisasterType::MEGA_EARTHQUAKE:
            d.name = "Mega Earthquake";
            d.duration_years = 2;
            d.severity = 0.85f;
            log_entry = "🫨 SEISMIC CATACLYSM: Massive earthquake strikes at (" + std::to_string(d.target_x) + ", " + std::to_string(d.target_y) + ")!";
            break;

        case DisasterType::LOCUST_SWARM:
            d.name = "Locust Swarm";
            d.duration_years = 2;
            d.severity = 0.65f;
            log_entry = "🦗 LOCUST SWARM: Billions of locusts decimate crops near (" + std::to_string(d.target_x) + ", " + std::to_string(d.target_y) + ")!";
            break;
    }

    active_disasters.push_back(d);
    disaster_history_log.push_back(log_entry);
    engine.history.record(engine.year, engine.month, "DISASTER", d.name, log_entry);
}

void AeonDisasterEngine::check_disaster_cascades(AeonEngine& engine) {
    for (size_t i = 0; i < active_disasters.size(); ++i) {
        auto& d = active_disasters[i];
        if (!d.is_active || d.triggered_cascade) continue;

        // Mega-earthquake triggers industrial breakdown & plague cascade
        if (d.type == DisasterType::MEGA_EARTHQUAKE && d.severity > 0.7f) {
            d.triggered_cascade = true;
            trigger_disaster(engine, DisasterType::BIO_PLAGUE, d.target_x, d.target_y);
            engine.history.record(engine.year, engine.month, "DISASTER_CASCADE",
                "Secondary Cataclysm: Disease Outbreak",
                "Broken sanitation and contaminated water from the earthquake trigger a virulent bio-plague cascade!", -1);
        }

        // Volcanic eruption causes global ash cloud dropping agricultural output
        if (d.type == DisasterType::VOLCANIC_ERUPTION && !d.triggered_cascade) {
            d.triggered_cascade = true;
            for (auto& cell : engine.gis_climate_engine.grid) {
                cell.temperature_c -= 4.0f; // 4°C Volcanic Winter
            }
        }
    }
}

void AeonDisasterEngine::contribute_to_relief_fund(int donor_civ_id, float amount, AeonEngine& engine) {
    if (donor_civ_id < 0 || donor_civ_id >= (int)engine.civs.size()) return;
    auto& donor = engine.civs[donor_civ_id];
    if (donor.economy.annual_income < amount) return;

    donor.economy.annual_income -= amount;
    international_relief_fund_gold += amount;
    donor.cultural_prestige = std::min(100.0f, donor.cultural_prestige + 4.0f);

    // Boost relations with all living civs
    for (auto& civ : engine.civs) {
        if (civ.id != donor_civ_id && civ.is_alive > 0.0f) {
            donor.diplomacy_pref = std::min(1.0f, donor.diplomacy_pref + 0.05f);
        }
    }
    engine.history.record(engine.year, engine.month, "DIPLOMACY",
        donor.name + " Contributes to Global Relief Fund",
        "Dispatched " + std::to_string((int)amount) + " Gold in emergency humanitarian aid.", donor_civ_id);
}

bool AeonDisasterEngine::enact_quarantine(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 10000.0f) return false;

    p.treasury_gold -= 10000.0f;
    for (auto& d : active_disasters) {
        if (d.type == DisasterType::BIO_PLAGUE && d.is_active) {
            d.severity *= 0.4f; // 60% reduction in mortality
        }
    }
    p.last_news_headline = "HEALTH MANDATE: Federal Administration orders national quarantine zone ($10,000 cost)!";
    return true;
}

bool AeonDisasterEngine::research_plague_vaccine(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 25000.0f) return false;

    p.treasury_gold -= 25000.0f;
    for (auto& d : active_disasters) {
        if (d.type == DisasterType::BIO_PLAGUE && d.is_active) {
            d.is_active = false; // Cured immediately
        }
    }
    p.last_news_headline = "MEDICAL TRIUMPH: Federal Laboratories develop & distribute Bio-Plague Vaccine ($25,000 cost)!";
    return true;
}

bool AeonDisasterEngine::deploy_disaster_relief(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 15000.0f) return false;

    p.treasury_gold -= 15000.0f;
    p.approval_rating += 10.0f;
    auto& civ = engine.civs[p.player_civ_id];
    civ.stability = std::min(100.0f, civ.stability + 15.0f);
    p.last_news_headline = "DISASTER RELIEF: Emergency convoys deliver food & medical aid to affected regions ($15,000 cost).";
    return true;
}

} // namespace Aeon

