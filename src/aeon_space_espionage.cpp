#include "aeon_space_espionage.h"
#include "aeon_engine.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace Aeon {

void AeonSpaceEspionage::init() {
    space = SpaceProgramState();
    nuke = NuclearState();
    espionage = EspionageState();

    megaprojects.clear();
    // 1. APOLLO LUNAR PROGRAM ($280k Gold total across 4 IRL phases)
    {
        Megaproject m;
        m.type = MegaprojectType::MOON_LANDING;
        m.name = "Apollo Lunar Program 🌕";
        m.max_phases = 4;
        m.phase_cost[0] = 30000.0f; // Phase 1: Heavy Rocketry R&D
        m.phase_cost[1] = 60000.0f; // Phase 2: Launchpad & Saturn V Test
        m.phase_cost[2] = 80000.0f; // Phase 3: Lunar Module & Orbiters
        m.phase_cost[3] = 110000.0f;// Phase 4: Crewed Moon Landing
        m.failure_risk = 0.25f;
        megaprojects.push_back(m);
    }
    // 2. MANHATTAN ATOMIC PROJECT ($200k Gold total)
    {
        Megaproject m;
        m.type = MegaprojectType::MANHATTAN_PROJECT;
        m.name = "Manhattan Atomic Project ⚛️";
        m.max_phases = 3;
        m.phase_cost[0] = 40000.0f; // Theoretical Physics & Cyclotrons
        m.phase_cost[1] = 70000.0f; // Enrichment Facilities
        m.phase_cost[2] = 90000.0f; // Trinity Test & Detonators
        m.failure_risk = 0.20f;
        megaprojects.push_back(m);
    }
    // 3. FUSION ENERGY GRID ($350k Gold total)
    {
        Megaproject m;
        m.type = MegaprojectType::FUSION_POWER;
        m.name = "Net Fusion Energy Grid ⚡";
        m.max_phases = 4;
        m.phase_cost[0] = 50000.0f;
        m.phase_cost[1] = 80000.0f;
        m.phase_cost[2] = 100000.0f;
        m.phase_cost[3] = 120000.0f;
        m.failure_risk = 0.15f;
        megaprojects.push_back(m);
    }
}

bool AeonSpaceEspionage::invest_in_megaproject(AeonEngine& engine, int project_idx, float gold_amount) {
    if (project_idx < 0 || project_idx >= (int)megaprojects.size()) return false;
    auto& p = engine.president_game;
    if (p.treasury_gold < gold_amount) return false;

    auto& mp = megaprojects[project_idx];
    if (mp.completed || mp.current_phase >= mp.max_phases) return false;

    p.treasury_gold -= gold_amount;
    int phase = mp.current_phase;
    mp.phase_progress[phase] += gold_amount;

    if (mp.phase_progress[phase] >= mp.phase_cost[phase]) {
        mp.phase_progress[phase] = mp.phase_cost[phase];
    }
    return true;
}

bool AeonSpaceEspionage::attempt_phase_launch(AeonEngine& engine, int project_idx) {
    if (project_idx < 0 || project_idx >= (int)megaprojects.size()) return false;
    auto& mp = megaprojects[project_idx];
    if (mp.completed || mp.current_phase >= mp.max_phases) return false;

    int phase = mp.current_phase;
    if (mp.phase_progress[phase] < mp.phase_cost[phase]) return false; // Needs full funding

    auto& p = engine.president_game;

    // Failure Roll
    float roll = static_cast<float>(rand() % 100) / 100.0f;
    if (roll < mp.failure_risk) {
        // CATASTROPHIC TEST EXPLOSION!
        mp.phase_progress[phase] *= 0.50f; // Lost half progress
        p.approval_rating -= 8.0f;
        p.last_news_headline = "💥 DISASTER AT CAPE CANAVERAL: " + mp.name + " Test Rocket exploded on launchpad! Progress delayed.";
        engine.history.record(engine.year, engine.month, "TECH", "MEGAPROJECT EXPLOSION: " + mp.name + " test failed!", "Launchpad Disaster", p.player_civ_id);
        return false;
    }

    // Success! Advance phase
    mp.current_phase++;
    if (mp.current_phase >= mp.max_phases) {
        mp.completed = true;
        p.approval_rating += 35.0f;
        p.last_news_headline = "🏆 HISTORIC TRIUMPH: " + mp.name + " completed! A monumental era begins for humanity!";
        engine.history.record(engine.year, engine.month, "TECH", "HISTORIC TRIUMPH: " + mp.name + " COMPLETED!", "Global Epoch Milestone", p.player_civ_id);

        if (mp.type == MegaprojectType::MOON_LANDING) {
            space.moon_landing_achieved = true;
            space.level = 3;
            engine.civs[p.player_civ_id].tech.progress += 200.0f;
        }
    } else {
        p.approval_rating += 10.0f;
        p.last_news_headline = "🚀 MEGAPROJECT ADVANCE: " + mp.name + " Phase " + std::to_string(mp.current_phase) + " achieved successfully!";
    }
    return true;
}

void AeonSpaceEspionage::tick_year(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (!p.active || p.player_civ_id < 0 || p.player_civ_id >= (int)engine.civs.size()) return;

    auto& civ = engine.civs[p.player_civ_id];

    // Passive Intel & Satellite bonuses
    if (space.satellites_launched > 0) {
        civ.tech.progress += space.satellites_launched * 2.0f;
    }

    // Produce ICBMs automatically if silos exist and funds available
    if (nuke.silos_built > 0 && p.treasury_gold >= 150.0f && nuke.icbm_stockpile < nuke.silos_built * 3) {
        p.treasury_gold -= 150.0f;
        nuke.icbm_stockpile++;
        std::cout << "[YEAR " << engine.year << "] [NUCLEAR] ICBM Warhead assembled. Arsenal: "
                  << nuke.icbm_stockpile << " ICBMs.\n";
    }
}

// ─── Space Program ─────────────────────────────────────────────────────────────
bool AeonSpaceEspionage::launch_satellite(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 25000.0f) return false;

    p.treasury_gold -= 25000.0f;
    space.satellites_launched++;
    space.level = std::max(space.level, 2);
    p.approval_rating += 5.0f;

    p.last_news_headline = "🚀 SPACE RACE: National Space Agency successfully launches Recon Satellite #" + std::to_string(space.satellites_launched) + " into orbit!";

    PresidentialRecord rec;
    rec.year = engine.year;
    rec.title = "Satellite Launch Successful";
    rec.summary = "Placed orbital reconnaissance satellite into space ($25,000 cost). Enhanced tech research & global intelligence.";
    rec.approval_delta = 5.0f;
    p.decree_history.push_back(rec);

    engine.history.record(engine.year, engine.month, "TECH",
        "Nordra launches Recon Satellite #" + std::to_string(space.satellites_launched),
        "Space Exploration Milestone", p.player_civ_id);
    return true;
}

bool AeonSpaceEspionage::launch_moon_mission(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 150000.0f || space.satellites_launched < 2) return false;

    p.treasury_gold -= 150000.0f;
    space.moon_landing_achieved = true;
    space.level = std::max(space.level, 3);
    p.approval_rating += 25.0f;

    auto& civ = engine.civs[p.player_civ_id];
    civ.tech.progress += 200.0f;

    p.last_news_headline = "🌕 GIANT LEAP FOR MANKIND: President Alex Sterling announces historic Moon Landing!";

    PresidentialRecord rec;
    rec.year = engine.year;
    rec.title = "Historic Lunar Moon Landing";
    rec.summary = "Landed astronauts on the Moon ($150,000 cost). Global prestige and scientific leadership established.";
    rec.approval_delta = 25.0f;
    p.decree_history.push_back(rec);

    engine.history.record(engine.year, engine.month, "TECH",
        "Nordra achieves historic Lunar Moon Landing!",
        "Ultimate Technological Triumph", p.player_civ_id);
    return true;
}

bool AeonSpaceEspionage::build_orbital_defense(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 180000.0f || !space.moon_landing_achieved) return false;

    p.treasury_gold -= 180000.0f;
    space.orbital_defense_active = true;
    space.level = 4;

    p.last_news_headline = "🛰️ DEFENSE GRID ONLINE: Orbital Defense Laser Network activated to intercept incoming missiles!";

    PresidentialRecord rec;
    rec.year = engine.year;
    rec.title = "Orbital Defense Network Activated";
    rec.summary = "Deployed orbital space lasers ($180,000 cost) capable of shooting down incoming enemy ballistic missiles.";
    rec.approval_delta = 10.0f;
    p.decree_history.push_back(rec);
    return true;
}

// ─── Nuclear Program ──────────────────────────────────────────────────────────
bool AeonSpaceEspionage::build_nuke_silo(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 120000.0f) return false;

    p.treasury_gold -= 120000.0f;
    nuke.silos_built++;
    nuke.icbm_stockpile += 1;

    p.last_news_headline = "⚛️ NUCLEAR DETERRENT: Underground ICBM Silo Complex #" + std::to_string(nuke.silos_built) + " commissioned ($120,000 cost).";

    PresidentialRecord rec;
    rec.year = engine.year;
    rec.title = "ICBM Nuclear Silo Construction";
    rec.summary = "Commissioned strategic nuclear missile silo facility ($120,000 cost). Armed first ICBM warhead.";
    rec.approval_delta = 3.0f;
    p.decree_history.push_back(rec);
    return true;
}

bool AeonSpaceEspionage::construct_icbm(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 35000.0f || nuke.silos_built <= 0) return false;

    p.treasury_gold -= 35000.0f;
    nuke.icbm_stockpile++;

    p.last_news_headline = "⚛️ ARSENAL EXPANDED: New thermonuclear ICBM warhead assembled ($35,000 cost). Stockpile: " + std::to_string(nuke.icbm_stockpile);
    return true;
}

bool AeonSpaceEspionage::launch_icbm_strike(AeonEngine& engine, int target_civ_id) {
    auto& p = engine.president_game;
    if (nuke.icbm_stockpile <= 0) return false;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;
    if (target_civ_id == p.player_civ_id) return false;

    auto& target = engine.civs[target_civ_id];
    if (target.is_alive <= 0.0f) return false;

    nuke.icbm_stockpile--;

    // 50% devastation on target civ
    target.population.total = static_cast<long long>(target.population.total * 0.50f);
    target.army_size = target.army_size * 0.40f;
    target.military_power = target.army_size * 0.10f;
    target.stability = std::max(0.0f, target.stability - 40.0f);
    target.at_war = true;
    target.war_with_civ = p.player_civ_id;

    auto& player_civ = engine.civs[p.player_civ_id];
    player_civ.at_war = true;
    player_civ.war_with_civ = target_civ_id;

    p.last_news_headline = "💥 NUCLEAR HOLOCAUST: ICBM Thermonuclear Strike detonates over " + target.name + "!";

    PresidentialRecord rec;
    rec.year = engine.year;
    rec.title = "Nuclear ICBM Strike Authorized";
    rec.summary = "Launched strategic nuclear missile strike against " + target.name + ". Devastated population & army.";
    rec.approval_delta = -10.0f;
    p.decree_history.push_back(rec);

    engine.history.record(engine.year, engine.month, "WAR",
        "Nordra launches ICBM Nuclear Strike on " + target.name + "!",
        "Catastrophic Thermonuclear Detonation", p.player_civ_id, target_civ_id);
    return true;
}

// ─── Covert Operations ────────────────────────────────────────────────────────
bool AeonSpaceEspionage::stage_foreign_coup(AeonEngine& engine, int target_civ_id) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 200000.0f) return false;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;
    if (target_civ_id == p.player_civ_id) return false;

    auto& target = engine.civs[target_civ_id];
    if (target.is_alive <= 0.0f) return false;

    p.treasury_gold -= 200000.0f;
    target.stability = 15.0f;
    target.government = GovForm::REPUBLIC;
    target.relations[p.player_civ_id] = DiplomacyStatus::ALLY;

    std::string log_msg = "Covert Op: Staged military coup in " + target.name + " ($200,000 cost). Installed friendly regime.";
    espionage.covert_ops_log.push_back(log_msg);

    p.last_news_headline = "🕵️ COVERT ACTION: Intelligence agency overthrows government of " + target.name + " and installs puppet regime!";

    PresidentialRecord rec;
    rec.year = engine.year;
    rec.title = "Foreign Coup d'État Authorized";
    rec.summary = "Financed covert military coup in " + target.name + " ($200,000 cost). Replaced hostile ruler with allied puppet.";
    rec.approval_delta = 8.0f;
    p.decree_history.push_back(rec);
    return true;
}

bool AeonSpaceEspionage::assassinate_ruler(AeonEngine& engine, int target_civ_id) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 60000.0f) return false;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;
    if (target_civ_id == p.player_civ_id) return false;

    auto& target = engine.civs[target_civ_id];
    if (target.is_alive <= 0.0f) return false;

    p.treasury_gold -= 60000.0f;
    target.stability = std::max(0.0f, target.stability - 30.0f);

    // Trigger succession
    for (auto& ch : engine.characters) {
        if (ch.id == target.ruler_id && ch.is_alive) {
            ch.is_alive = false;
            ch.death_year = engine.year;
            break;
        }
    }

    p.last_news_headline = "🗡️ ASSASSINATION: Ruler of " + target.name + " assassinated ($60,000 cost)! Nation thrown into succession chaos.";

    PresidentialRecord rec;
    rec.year = engine.year;
    rec.title = "Targeted Assassination Executed";
    rec.summary = "Deployed covert assassins to eliminate ruler of " + target.name + " ($60,000 cost). Caused total political chaos.";
    rec.approval_delta = 4.0f;
    p.decree_history.push_back(rec);
    return true;
}

bool AeonSpaceEspionage::inject_disinformation(AeonEngine& engine, int target_civ_id) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 15000.0f) return false;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;

    auto& target = engine.civs[target_civ_id];
    p.treasury_gold -= 15000.0f;
    target.stability = std::max(0.0f, target.stability - 15.0f);

    p.last_news_headline = "📜 DISINFORMATION: Psychological ops ($15,000 cost) degrade social cohesion in " + target.name + ".";
    return true;
}

bool AeonSpaceEspionage::steal_tech_blueprints(AeonEngine& engine, int target_civ_id) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 35000.0f) return false;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;
    if (target_civ_id == p.player_civ_id) return false;

    auto& target = engine.civs[target_civ_id];
    if (target.is_alive <= 0.0f) return false;

    p.treasury_gold -= 35000.0f;
    float stolen_pts = std::min(target.tech.research_pts * 0.4f, 500.0f);
    target.tech.research_pts = std::max(0.0f, target.tech.research_pts - stolen_pts);
    
    if (p.player_civ_id >= 0 && p.player_civ_id < (int)engine.civs.size()) {
        engine.civs[p.player_civ_id].tech.research_pts += stolen_pts * 1.2f;
    }

    espionage.infiltration_levels[target_civ_id] = std::min(100.0f, get_infiltration(target_civ_id) + 15.0f);
    p.last_news_headline = "💾 TECH THEFT: Industrial espionage extracts vital R&D schematics from " + target.name + "!";
    engine.history.record(engine.year, engine.month, "ESPIONAGE",
        "Blueprint Heist", "Covert operatives exfiltrated advanced research data.", p.player_civ_id);
    return true;
}

bool AeonSpaceEspionage::execute_power_grid_blackout(AeonEngine& engine, int target_civ_id) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 40000.0f) return false;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;

    auto& target = engine.civs[target_civ_id];
    if (target.is_alive <= 0.0f) return false;

    p.treasury_gold -= 40000.0f;
    target.resources.energy = std::max(0.0f, target.resources.energy - 100.0f);
    target.economy.annual_income *= 0.75f; // 25% revenue loss from grid failure
    target.stability = std::max(0.0f, target.stability - 12.0f);

    p.last_news_headline = "⚡ CYBER ATTACK: Critical infrastructure shutdown plunges " + target.name + " into darkness!";
    engine.history.record(engine.year, engine.month, "ESPIONAGE",
        "Grid Sabotage", "SCADA malware crippled electrical substation network.", p.player_civ_id);
    return true;
}

bool AeonSpaceEspionage::execute_financial_hack(AeonEngine& engine, int target_civ_id) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 25000.0f) return false;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;

    auto& target = engine.civs[target_civ_id];
    if (target.is_alive <= 0.0f) return false;

    p.treasury_gold -= 25000.0f;
    float siphon_gold = std::min(target.economy.annual_income * 0.25f, 50000.0f);
    target.economy.annual_income -= siphon_gold;
    p.treasury_gold += siphon_gold;
    target.economy.inflation += 0.045f;

    p.last_news_headline = "🏦 FINANCIAL SYSTEM BREACH: Banking algorithms in " + target.name + " manipulated, siphoning reserves!";
    return true;
}

bool AeonSpaceEspionage::stage_false_flag_incident(AeonEngine& engine, int victim_civ_id, int framed_civ_id) {
    auto& p = engine.president_game;
    if (p.treasury_gold < 80000.0f) return false;
    if (victim_civ_id < 0 || victim_civ_id >= (int)engine.civs.size()) return false;
    if (framed_civ_id < 0 || framed_civ_id >= (int)engine.civs.size()) return false;
    if (victim_civ_id == framed_civ_id) return false;

    p.treasury_gold -= 80000.0f;
    auto& victim = engine.civs[victim_civ_id];
    auto& framed = engine.civs[framed_civ_id];

    // Trigger war between the two nations
    victim.relations[framed_civ_id] = DiplomacyStatus::AT_WAR;
    framed.relations[victim_civ_id] = DiplomacyStatus::AT_WAR;
    victim.at_war = true;
    framed.at_war = true;

    p.last_news_headline = "🔥 BORDER INCIDENT: Outrage erupts as " + victim.name + " declares war on " + framed.name + " following covert sabotage!";
    engine.history.record(engine.year, engine.month, "WAR",
        victim.name + " vs " + framed.name, "Border conflict escalates into full armed war.", victim_civ_id);
    return true;
}

bool AeonSpaceEspionage::fund_proxy_rebellion(AeonEngine& engine, int target_civ_id, float funding_gold) {
    auto& p = engine.president_game;
    if (p.treasury_gold < funding_gold || funding_gold < 20000.0f) return false;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return false;

    auto& target = engine.civs[target_civ_id];
    if (target.is_alive <= 0.0f) return false;

    p.treasury_gold -= funding_gold;
    target.stability = std::max(0.0f, target.stability - (funding_gold / 5000.0f));

    // Spawn guerrilla cell
    GuerrillaCell cell;
    cell.native_civ_id = target_civ_id;
    cell.occupier_civ_id = target_civ_id;
    cell.strength = (int)(funding_gold / 100.0f);
    cell.sabotage_rate = 0.08f;
    engine.military_engine.guerrilla_cells.push_back(cell);

    p.last_news_headline = "⚔️ PROXY WAR: Insurgency funded in " + target.name + "!";
    return true;
}

bool AeonSpaceEspionage::execute_mission(int actor_civ_id, int target_civ_id, EspionageMissionType mission, AeonEngine& engine, bool& out_detected) {
    out_detected = false;
    if (actor_civ_id < 0 || actor_civ_id >= (int)engine.civs.size()) return false;
    auto& actor = engine.civs[actor_civ_id];

    if (mission == EspionageMissionType::COUNTER_INTELLIGENCE) {
        // Boost internal security & expose covert foreign spies
        actor.stability = std::clamp(actor.stability + 3.0f, 0.0f, 100.0f);
        std::cout << "[YEAR " << engine.year << "] 🕵️ COUNTER-INTEL: " << actor.name
                  << " conducts counter-espionage sweeps, securing state secrets." << std::endl;
        return true;
    }

    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size() || target_civ_id == actor_civ_id) return false;
    auto& target = engine.civs[target_civ_id];
    if (target.is_alive <= 0.0f) return false;

    // Calculate success and detection probabilities based on ruler intelligence and tech era
    float base_success = 0.65f;
    float base_detect  = 0.25f;

    const AeonCharacter* actor_ruler = nullptr;
    if (actor.ruler_id >= 0) {
        for (const auto& ch : engine.characters) {
            if (ch.id == actor.ruler_id && ch.is_alive) { actor_ruler = &ch; break; }
        }
    }
    if (actor_ruler) {
        base_success += actor_ruler->intelligence * 0.20f + actor_ruler->paranoia * 0.10f;
    }

    const AeonCharacter* target_ruler = nullptr;
    if (target.ruler_id >= 0) {
        for (const auto& ch : engine.characters) {
            if (ch.id == target.ruler_id && ch.is_alive) { target_ruler = &ch; break; }
        }
    }
    if (target_ruler) {
        base_detect += target_ruler->intelligence * 0.15f + target_ruler->paranoia * 0.20f;
    }

    bool success = engine.rng.chance(std::clamp(base_success, 0.20f, 0.90f));
    out_detected = engine.rng.chance(std::clamp(base_detect, 0.10f, 0.80f));

    if (success) {
        switch (mission) {
            case EspionageMissionType::SPY:
                std::cout << "[YEAR " << engine.year << "] 🕵️ ESPIONAGE: " << actor.name
                          << " successfully infiltrates " << target.name << "'s government." << std::endl;
                break;
            case EspionageMissionType::STEAL_TECHNOLOGY:
                if (target.tech.research_pts > 50.0f) {
                    float stolen = target.tech.research_pts * 0.25f;
                    target.tech.research_pts -= stolen;
                    actor.tech.research_pts += stolen;
                    std::cout << "[YEAR " << engine.year << "] 💾 TECH THEFT: " << actor.name
                              << " secretly steals advanced research from " << target.name << "!" << std::endl;
                }
                break;
            case EspionageMissionType::SABOTAGE:
                target.economy.gdp *= 0.93f;
                target.resources.energy = std::max(0.0f, target.resources.energy - 30.0f);
                std::cout << "[YEAR " << engine.year << "] 💥 SABOTAGE: Covert operatives destroy industrial plants in "
                          << target.name << "!" << std::endl;
                break;
            case EspionageMissionType::INFILTRATE_MILITARY:
                target.military_loyalty = std::max(10.0f, target.military_loyalty - 15.0f);
                target.military_power *= 0.90f;
                std::cout << "[YEAR " << engine.year << "] 🎖️ MILITARY INFILTRATION: " << actor.name
                          << " compromises command communications in " << target.name << "." << std::endl;
                break;
            case EspionageMissionType::FUND_REBELS:
                target.unrest = std::min(100.0f, target.unrest + 25.0f);
                target.stability = std::max(0.0f, target.stability - 15.0f);
                if (!target.factions.empty()) {
                    target.factions[0].rebellion_risk = std::min(100.0f, target.factions[0].rebellion_risk + 20.0f);
                }
                std::cout << "[YEAR " << engine.year << "] 💰 REBEL FUNDING: Secret arms deliveries ignite unrest in "
                          << target.name << "!" << std::endl;
                break;
            case EspionageMissionType::STEAL_RESOURCES:
                if (target.resources.iron > 20.0f) { target.resources.iron -= 20.0f; actor.resources.iron += 20.0f; }
                if (target.resources.oil > 20.0f)  { target.resources.oil -= 20.0f;  actor.resources.oil += 20.0f; }
                actor.economy.gdp += 100.0f;
                target.economy.gdp = std::max(50.0f, target.economy.gdp - 100.0f);
                std::cout << "[YEAR " << engine.year << "] 🛢️ RESOURCE HEIST: Operatives siphon strategic stockpiles from "
                          << target.name << "!" << std::endl;
                break;
            case EspionageMissionType::DISCOVER_SECRET:
                target.secret_goal.is_revealed = true;
                std::cout << "[YEAR " << engine.year << "] 📜 SECRET EXPOSED: " << actor.name
                          << " uncovers " << target.name << "'s hidden goal: "
                          << secret_goal_name(target.secret_goal.type) << "!" << std::endl;
                break;
            default:
                break;
        }
    }

    if (out_detected) {
        // Severe diplomatic fallout
        engine.history.relation(actor_civ_id, target_civ_id).record_espionage();
        std::cout << "[YEAR " << engine.year << "] 🚨 ESPIONAGE DISCOVERED: " << target.name
                  << " catches " << actor.name << "'s secret agents red-handed! Relations collapse." << std::endl;

        engine.history.record(engine.year, engine.month, "DIPLOMACY",
            target.name + " uncovers " + actor.name + " espionage ring",
            actor.name + " was caught executing " + espionage_mission_name(mission) + " inside sovereign territory.",
            actor_civ_id, target_civ_id, {"espionage_detected"}, 0.85f);
    }

    return success;
}

float AeonSpaceEspionage::get_infiltration(int civ_id) const {
    auto it = espionage.infiltration_levels.find(civ_id);
    return (it != espionage.infiltration_levels.end()) ? it->second : 0.0f;
}

} // namespace Aeon
