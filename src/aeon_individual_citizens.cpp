#include "aeon_individual_citizens.h"
#include "aeon_engine.h"
#include <algorithm>
#include <iostream>

namespace Aeon {

std::string IndividualCitizen::get_role_string() const {
    switch (role) {
        case CitizenRole::GENERAL:          return "General";
        case CitizenRole::GRAND_MERCHANT:   return "Grand Merchant";
        case CitizenRole::CHIEF_SCIENTIST:  return "Chief Scientist";
        case CitizenRole::IMPERIAL_SPY:     return "Imperial Spy";
        case CitizenRole::GOVERNOR:         return "Governor";
        case CitizenRole::HIGH_CLERIC:      return "High Cleric";
        case CitizenRole::MASTER_ARTISAN:   return "Master Artisan";
        default:                            return "Citizen";
    }
}

AeonCitizenEngine::AeonCitizenEngine() {
}

void AeonCitizenEngine::init_citizens(const AeonEngine& engine) {
    citizens.clear();

    const char* first_names[] = { "Marcus", "Elena", "Valerius", "Kaelen", "Aurelia", "Darius", "Lyra", "Cassian" };
    const char* last_names[]  = { "Vane", "Sterling", "Ironheart", "Blackwood", "Sunstrider", "Drakos", "Aethel" };

    int next_id = 1;
    for (const auto& civ : engine.civs) {
        if (civ.is_commons) continue;

        for (int i = 0; i < 4; ++i) {
            IndividualCitizen c;
            c.id = next_id++;
            c.civ_id = civ.id;
            c.name = std::string(first_names[(next_id + i) % 8]) + " " + last_names[(next_id + civ.id) % 7];
            c.role = static_cast<CitizenRole>(i % 7);
            c.age = 22 + (i * 6);
            c.health = 100.0f;
            c.wealth_gold = 1500.0 + (i * 800.0);
            c.loyalty = 75.0f + (rand() % 20);
            c.ambition = 40.0f + (rand() % 50);
            c.influence = 30.0f + (i * 15);
            c.map_x = std::max(0, std::min(MAP_WIDTH - 1, civ.capital_x + ((i % 2 == 0) ? 2 : -2)));
            c.map_y = std::max(0, std::min(MAP_HEIGHT - 1, civ.capital_y + ((i > 1) ? 2 : -2)));
            c.journal_log.push_back("Entered service of " + civ.name + " as " + c.get_role_string() + ".");
            c.is_alive = true;
            citizens.push_back(c);
        }
    }
}

void AeonCitizenEngine::update_citizens_tick(AeonEngine& engine) {
    std::vector<int> dead_civs_to_replace;
    for (auto& c : citizens) {
        if (!c.is_alive) continue;

        c.age++;
        c.wealth_gold += (c.influence * 12.0);

        // Movement on 2D map
        c.map_x = (c.map_x + (rand() % 3 - 1) + MAP_WIDTH) % MAP_WIDTH;
        c.map_y = (c.map_y + (rand() % 3 - 1) + MAP_HEIGHT) % MAP_HEIGHT;

        // Coup Plotting check
        if (c.loyalty < 35.0f && c.ambition > 65.0f && !c.has_active_coup_plot) {
            c.has_active_coup_plot = true;
            c.coup_plot_stage = 1;
            c.journal_log.push_back("Formed a secret cabal to overthrow the ruler.");
        }

        if (c.has_active_coup_plot) {
            progress_coup_plot(c, engine);
        }

        // Master Artisan artifact creation
        if (c.role == CitizenRole::MASTER_ARTISAN && (rand() % 100) < 10) {
            create_artisan_artifact(c.id, engine);
        }

        // Natural mortality check
        if (c.age > 75 && (rand() % 100) < (c.age - 70)) {
            c.is_alive = false;
            int dead_civ = c.civ_id;
            if (c.civ_id >= 0 && c.civ_id < (int)engine.civs.size()) {
                engine.history.record(engine.year, engine.month, "CITIZEN",
                    c.name + " Has Passed Away",
                    c.get_role_string() + " of " + engine.civs[c.civ_id].name + " died of natural causes aged " + std::to_string(c.age) + ".", c.civ_id);
            }
            dead_civs_to_replace.push_back(dead_civ);
        }
    }
    for (int d_civ : dead_civs_to_replace) {
        spawn_replacement_citizen(d_civ, engine);
    }
}

void AeonCitizenEngine::trigger_coup(int citizen_id, AeonEngine& engine) {
    for (auto& c : citizens) {
        if (c.id == citizen_id && c.is_alive) {
            c.has_active_coup_plot = false;
            if (c.civ_id >= 0 && c.civ_id < (int)engine.civs.size()) {
                auto& civ = engine.civs[c.civ_id];
                civ.stability = std::max(0.0f, civ.stability - 30.0f);
                c.influence += 25.0f;
                c.journal_log.push_back("Launched a violent military coup in " + civ.name + "!");
                engine.history.record(engine.year, engine.month, "COUP",
                    "COUP DETAT IN " + civ.name + "!",
                    c.name + " (" + c.get_role_string() + ") staged an armed uprising against the regime!", c.civ_id);
                std::cout << "[YEAR " << engine.year << "] ⚡ COUP DETAT: " << c.name << " staged a coup in " << civ.name << "!" << std::endl;
            }
            break;
        }
    }
}

void AeonCitizenEngine::progress_coup_plot(IndividualCitizen& citizen, AeonEngine& engine) {
    if (!citizen.has_active_coup_plot || !citizen.is_alive) return;
    if (citizen.civ_id < 0 || citizen.civ_id >= (int)engine.civs.size()) return;
    auto& civ = engine.civs[citizen.civ_id];

    citizen.coup_plot_stage++;
    switch (citizen.coup_plot_stage) {
        case 1:
            citizen.journal_log.push_back("Approached sympathetic garrison commanders in secret.");
            break;
        case 2:
            citizen.journal_log.push_back("Secured heavy armaments and clandestine foreign funding.");
            civ.stability = std::max(10.0f, civ.stability - 5.0f);
            break;
        case 3:
            citizen.journal_log.push_back("Executed midnight assault on the executive palace!");
            trigger_coup(citizen.id, engine);
            citizen.has_active_coup_plot = false;
            citizen.coup_plot_stage = 0;
            break;
    }
}

void AeonCitizenEngine::check_oligarch_monopolies(AeonEngine& engine) {
    for (auto& c : citizens) {
        if (!c.is_alive || c.role != CitizenRole::GRAND_MERCHANT) continue;
        if (c.civ_id < 0 || c.civ_id >= (int)engine.civs.size()) continue;

        auto& civ = engine.civs[c.civ_id];
        if (c.wealth_gold > civ.economy.annual_income * 0.20 && civ.economy.annual_income > 500.0) {
            civ.economy.wealth_inequality = std::min(0.95f, civ.economy.wealth_inequality + 0.02f);
            civ.population.happiness = std::max(0.0f, civ.population.happiness - 1.5f);
            civ.economy.annual_income -= 20.0f;
            c.wealth_gold += 20.0f;
            c.journal_log.push_back("Bribed ministers to secure tariff exemptions and regional monopoly.");
        }
    }
}

void AeonCitizenEngine::create_artisan_artifact(int citizen_id, AeonEngine& engine) {
    for (auto& c : citizens) {
        if (c.id == citizen_id && c.is_alive) {
            c.artifacts_created++;
            c.influence += 10.0f;
            c.wealth_gold += 3000.0;
            const char* artifacts[] = { "Crown of Sunfire", "Relic of Ancient Kings", "Golden Obelisk", "Chronicle Tapestry", "Imperial Scepter" };
            std::string item_name = artifacts[c.artifacts_created % 5];
            c.journal_log.push_back("Crafted legendary relic: " + item_name);

            if (c.civ_id >= 0 && c.civ_id < (int)engine.civs.size()) {
                auto& civ = engine.civs[c.civ_id];
                civ.stability = std::min(100.0f, civ.stability + 5.0f);
                engine.history.record(engine.year, engine.month, "ARTIFACT",
                    "Legendary Artifact Created: " + item_name,
                    c.name + " crafted a glorious wonder for " + civ.name + ".", c.civ_id);
            }
            break;
        }
    }
}

void AeonCitizenEngine::spawn_replacement_citizen(int civ_id, AeonEngine& engine) {
    if (civ_id < 0 || civ_id >= (int)engine.civs.size()) return;
    const auto& civ = engine.civs[civ_id];
    if (civ.is_commons) return;

    const char* firsts[] = { "Theron", "Aurelius", "Soren", "Cassandra", "Lyria", "Vesper", "Zephyr" };
    const char* lasts[]  = { "Valarian", "Nightshade", "Ironclad", "Sunblade", "Stormrider" };

    IndividualCitizen nc;
    nc.id = ++next_citizen_id;
    nc.civ_id = civ_id;
    nc.name = std::string(firsts[rand() % 7]) + " " + lasts[rand() % 5];
    nc.role = static_cast<CitizenRole>(rand() % 7);
    nc.age = 20 + rand() % 10;
    nc.health = 100.0f;
    nc.wealth_gold = 1000.0;
    nc.loyalty = 80.0f;
    nc.ambition = 40.0f + (rand() % 40);
    nc.influence = 25.0f;
    nc.map_x = civ.capital_x;
    nc.map_y = civ.capital_y;
    nc.journal_log.push_back("Arose to prominence in " + civ.name + " as " + nc.get_role_string() + ".");
    citizens.push_back(nc);
}

void AeonCitizenEngine::promote_citizen(int citizen_id, CitizenRole new_role, AeonEngine& engine) {
    for (auto& c : citizens) {
        if (c.id == citizen_id && c.is_alive) {
            c.role = new_role;
            c.loyalty = std::min(100.0f, c.loyalty + 15.0f);
            c.influence += 20.0f;
            c.journal_log.push_back("Promoted to " + c.get_role_string() + " by Imperial Decree.");

            if (c.civ_id >= 0 && c.civ_id < (int)engine.civs.size()) {
                engine.history.record(engine.year, engine.month, "PROMOTION",
                    c.name + " Promoted to " + c.get_role_string(),
                    "Appointed by royal decree in " + engine.civs[c.civ_id].name + ".", c.civ_id);
            }
            std::cout << "[YEAR " << engine.year << "] 👤 CITIZEN PROMOTION: " << c.name << " promoted to " << c.get_role_string() << "!" << std::endl;
            break;
        }
    }
}

void AeonCitizenEngine::exile_citizen(int citizen_id, AeonEngine& engine) {
    for (auto& c : citizens) {
        if (c.id == citizen_id && c.is_alive) {
            int old_civ = c.civ_id;
            c.civ_id = 5; // Exile to The Commons
            c.loyalty = 10.0f;
            c.journal_log.push_back("Exiled from home realm by Imperial Edict.");

            engine.history.record(engine.year, engine.month, "EXILE",
                c.name + " Exiled to The Commons",
                "Banished following political disputes.", old_civ);
            std::cout << "[YEAR " << engine.year << "] 👤 EXILE: " << c.name << " banished to The Commons!" << std::endl;
            break;
        }
    }
}

void AeonCitizenEngine::assassinate_citizen(int citizen_id, AeonEngine& engine) {
    for (auto& c : citizens) {
        if (c.id == citizen_id && c.is_alive) {
            c.is_alive = false;
            c.journal_log.push_back("Assassinated by covert operative.");

            engine.history.record(engine.year, engine.month, "ASSASSINATION",
                c.name + " Assassinated in Secret Plot!",
                "Covert strike carried out against key imperial figure.", c.civ_id);
            std::cout << "[YEAR " << engine.year << "] 👤 ASSASSINATION: " << c.name << " eliminated!" << std::endl;
            break;
        }
    }
}

} // namespace Aeon
