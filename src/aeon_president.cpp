#include "aeon_president.h"
#include "aeon_engine.h"
#include "aeon_ollama.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <random>
#include <nlohmann/json.hpp>

namespace Aeon {

void AeonPresidentGame::init() {
    active = true;
    player_civ_id = 0;
    approval_rating = 65.0f;
    coup_risk = 5.0f;
    years_in_office = 0;
    term_counter = 0;
    elections_won = 1;
    is_overthrown = false;
    election_loss = false;
    last_news_headline = "President Alex Sterling sworn in as Chief Executive of the Realm.";

    decree_history.clear();
    current_crisis.active = false;
    crisis_pending = false;
}

void AeonPresidentGame::tick_year(AeonEngine& engine) {
    if (!active || player_civ_id < 0 || player_civ_id >= (int)engine.civs.size()) return;

    auto& civ = engine.civs[player_civ_id];
    if (civ.is_alive <= 0.0f) {
        is_overthrown = true;
        approval_rating = 0.0f;
        return;
    }

    years_in_office++;
    term_counter++;

    // 1. Fiscal Calculations based on Tax Rates (Real-world scaling)
    float total_tax_rate = (income_tax + corporate_tax + import_tariff) / 100.0f;
    float annual_tax_revenue = civ.economy.gdp * total_tax_rate * 0.40f;
    treasury_gold += annual_tax_revenue;

    // High taxes penalty on approval
    if (total_tax_rate > 0.60f) {
        approval_rating -= 4.0f;
    } else if (total_tax_rate < 0.20f) {
        approval_rating += 1.0f;
    }

    // 2. Department Spending Expenditures (Deducted from Treasury)
    float total_budget_pct = budget_defense + budget_healthcare + budget_education + budget_infrastructure + budget_welfare;
    if (total_budget_pct > 0.0f) {
        budget_defense /= total_budget_pct;
        budget_healthcare /= total_budget_pct;
        budget_education /= total_budget_pct;
        budget_infrastructure /= total_budget_pct;
        budget_welfare /= total_budget_pct;
    }
    float annual_expenditures = annual_tax_revenue * 0.85f;
    treasury_gold -= annual_expenditures;

    // National Debt Interest if Treasury is in deficit
    if (treasury_gold < 0.0f) {
        treasury_gold *= 1.06f; // 6% annual debt interest
    }

    // Defense -> Military Army growth & Maintenance
    civ.army_size += budget_defense * 1500.0f;
    civ.military_power = civ.army_size * 0.15f;

    // Healthcare & Welfare -> Approval & Population
    approval_rating += (budget_healthcare + budget_welfare - 0.30f) * 5.0f;

    // Education & Infrastructure -> Tech & GDP
    civ.tech.progress += budget_education * 12.0f;
    civ.economy.gdp += civ.economy.gdp * (budget_infrastructure * 0.06f);

    // 3. Faction Risk & Coup Meter
    float avg_faction_loyalty = 0.0f;
    if (!civ.factions.empty()) {
        for (const auto& f : civ.factions) {
            avg_faction_loyalty += f.loyalty;
        }
        avg_faction_loyalty /= civ.factions.size();
    } else {
        avg_faction_loyalty = 70.0f;
    }

    // Coup risk inversely proportional to stability & loyalty
    coup_risk = std::max(0.0f, std::min(100.0f, (100.0f - civ.stability) * 0.5f + (100.0f - avg_faction_loyalty) * 0.5f - (approval_rating * 0.2f)));

    // Clamp approval rating
    approval_rating = std::max(0.0f, std::min(100.0f, approval_rating));
    civ.stability = std::max(0.0f, std::min(100.0f, civ.stability * 0.95f + approval_rating * 0.05f));

    // Coup Check
    if (coup_risk > 85.0f && (rand() % 100 < 30)) {
        is_overthrown = true;
        last_news_headline = "BREAKING: Military Junta overthrows the Administration in violent coup!";
        return;
    }

    // 4. Trigger Crises every 2 years
    if (term_counter % 2 == 1 && !current_crisis.active) {
        trigger_ollama_crisis(engine);
    }

    // 5. Check Elections every 4 years
    if (term_counter >= 4) {
        trigger_election(engine);
        term_counter = 0;
    }
}

void AeonPresidentGame::trigger_ollama_crisis(AeonEngine& engine) {
    (void)engine;
    current_crisis.options.clear();

    // Fast procedural crisis generator (0ms UI latency)
    int r = rand() % 4;
        if (r == 0) {
            current_crisis.title = "🌾 National Grain Supply Deficit";
            current_crisis.description = "Drought across key agrarian provinces threatens food supplies and consumer inflation.";
            current_crisis.advisor_defense = "SecDef: Secure grain reserves with army transports.";
            current_crisis.advisor_economy = "SecTreasury: Authorize emergency food imports from global market.";
            current_crisis.advisor_opposition = "Opposition: The President's agricultural tariffs ruined our farmers!";
            current_crisis.options = {
                "Purchase Emergency Grain Imports (-$300 Gold, +10% Approval)",
                "Ration Food Supplies (-10% Approval, +15% Stability)",
                "Impose Price Controls on Farmers (+5% Approval, -10% Economy)"
            };
        } else if (r == 1) {
            current_crisis.title = "Border Security & Incursion";
            current_crisis.description = "Unidentified armed skirmishers crossed northern territory borders.";
            current_crisis.advisor_defense = "SecDef: Mobilize 2000 troops and construct border fortifications.";
            current_crisis.advisor_economy = "SecTreasury: Diplomatic resolution is cheaper than military deployment.";
            current_crisis.advisor_opposition = "Opposition: Weak presidential leadership is inviting foreign aggression!";
            current_crisis.options = {
                "Deploy Border Regiments (-$200 Gold, +1000 Army, +5% Approval)",
                "File Diplomatic Protest (+10 Trust, -5% Approval)",
                "Establish Demilitarized Buffer Zone (-5% Territory, +10% Stability)"
            };
        } else if (r == 2) {
            current_crisis.title = "⚒️ Workers General Strike Threat";
            current_crisis.description = "Trade unions demand wage increases and safer industrial work standards.";
            current_crisis.advisor_defense = "SecDef: Deploy national guard to keep factories open.";
            current_crisis.advisor_economy = "SecTreasury: Meet union demands to prevent industrial stagnation.";
            current_crisis.advisor_opposition = "Opposition: Workers stand against government corporate favoritism!";
            current_crisis.options = {
                "Sign Labor Protection Act (+15% Approval, -5% Corporate Tax Revenue)",
                "Outlaw General Strike (-15% Approval, +10% Industrial Output)",
                "Grant One-Time Wage Subsidies (-$250 Gold, +5% Approval)"
            };
        } else {
            current_crisis.title = "🔬 High-Tech Research Opportunity";
            current_crisis.description = "National Academy of Sciences requests federal funding for steam power research.";
            current_crisis.advisor_defense = "SecDef: Direct innovation toward military weapons.";
            current_crisis.advisor_economy = "SecTreasury: Commercial tech will skyrocket national GDP.";
            current_crisis.advisor_opposition = "Opposition: Deficit spending on vanity projects!";
            current_crisis.options = {
                "Fund Industrial R&D Grant (-$400 Gold, +25% Tech Progress)",
                "Direct Funding to Military Innovation (-$300 Gold, +500 Army Power)",
                "Decline Funding Request ($0, -5% Approval)"
            };
        }

    current_crisis.year_triggered = engine.year;
    current_crisis.active = true;
    crisis_pending = true;
}

void AeonPresidentGame::resolve_crisis_option(AeonEngine& engine, int option_idx) {
    if (!current_crisis.active || option_idx < 0 || option_idx >= (int)current_crisis.options.size()) return;

    auto& civ = engine.civs[player_civ_id];
    std::string chosen_text = current_crisis.options[option_idx];

    // Apply stat changes based on choice
    if (option_idx == 0) {
        treasury_gold = std::max(0.0f, treasury_gold - 250.0f);
        approval_rating += 8.0f;
        civ.stability += 5.0f;
    } else if (option_idx == 1) {
        approval_rating -= 5.0f;
        civ.stability += 10.0f;
        civ.army_size += 500.0f;
    } else {
        civ.economy.gdp += 100.0f;
        approval_rating += 3.0f;
    }

    PresidentialRecord rec;
    rec.year = engine.year;
    rec.title = current_crisis.title;
    rec.summary = "Executive Action: " + chosen_text;
    rec.approval_delta = (option_idx == 0) ? 8.0f : ((option_idx == 1) ? -5.0f : 3.0f);
    decree_history.push_back(rec);

    last_news_headline = "PRESIDENTIAL DECREE: " + chosen_text;
    current_crisis.active = false;
    crisis_pending = false;
}

void AeonPresidentGame::enact_decree(AeonEngine& engine, DecreeType decree, const std::string& custom_prompt) {
    if (!active || player_civ_id < 0 || player_civ_id >= (int)engine.civs.size()) return;

    auto& civ = engine.civs[player_civ_id];
    PresidentialRecord rec;
    rec.year = engine.year;

    switch (decree) {
        case DecreeType::ECONOMIC_STIMULUS:
            if (treasury_gold >= 25000.0f) {
                treasury_gold -= 25000.0f;
                civ.economy.gdp += 50000.0f;
                approval_rating += 6.0f;
                rec.title = "Economic Stimulus Package Passed";
                rec.summary = "Injected $25,000 gold into infrastructure and commercial business grants.";
                rec.approval_delta = 6.0f;
                last_news_headline = "PRESIDENTIAL ORDER: Government authorizes massive $25,000 economic stimulus!";
            } else {
                last_news_headline = "TREASURY ERROR: Insufficient gold funds for $25,000 Economic Stimulus!";
                return;
            }
            break;

        case DecreeType::EMERGENCY_DRAFT:
            if (treasury_gold >= 15000.0f) {
                treasury_gold -= 15000.0f;
                civ.army_size += 5000.0f;
                approval_rating -= 8.0f;
                civ.stability -= 4.0f;
                rec.title = "Emergency Conscription Ordered";
                rec.summary = "Drafted and equipped 5,000 citizens into active military service ($15,000 cost).";
                rec.approval_delta = -8.0f;
                last_news_headline = "MILITARY MANDATE: President orders emergency conscription of 5,000 troops.";
            } else {
                last_news_headline = "TREASURY ERROR: Insufficient gold funds ($15,000 needed for Draft)!";
                return;
            }
            break;

        case DecreeType::RESEARCH_SUBSIDY:
            if (treasury_gold >= 20000.0f) {
                treasury_gold -= 20000.0f;
                civ.tech.progress += 50.0f;
                approval_rating += 4.0f;
                rec.title = "National Science Grant Approved";
                rec.summary = "Allocated $20,000 gold to federal research laboratories.";
                rec.approval_delta = 4.0f;
                last_news_headline = "SCIENCE DIRECTIVE: Administration grants $20,000 gold to national laboratories.";
            } else {
                last_news_headline = "TREASURY ERROR: Insufficient gold funds for Science Grant!";
                return;
            }
            break;

        case DecreeType::DIPLOMATIC_ENVOY:
            for (auto& pair : civ.relations) {
                pair.second = DiplomacyStatus::NEUTRAL;
            }
            approval_rating += 5.0f;
            rec.title = "Global Peace Envoy Dispatched";
            rec.summary = "Sent diplomatic ambassadors to foreign powers to preserve peace.";
            rec.approval_delta = 5.0f;
            last_news_headline = "DIPLOMACY: President dispatches peace delegates across foreign capitals.";
            break;

        case DecreeType::FOOD_RELIEF:
            if (treasury_gold >= 18000.0f) {
                treasury_gold -= 18000.0f;
                civ.resources.food += 2500.0f;
                approval_rating += 7.0f;
                rec.title = "Federal Food Relief Distributed";
                rec.summary = "Purchased and distributed $18,000 emergency food stockpiles to regional cities.";
                rec.approval_delta = 7.0f;
                last_news_headline = "RELIEF EFFORT: Emergency food convoys arrive in provincial capitals.";
            } else {
                last_news_headline = "TREASURY ERROR: Insufficient gold funds for Food Relief!";
                return;
            }
            break;

        case DecreeType::BORDER_LOCKOUT:
            civ.stability += 8.0f;
            approval_rating -= 3.0f;
            rec.title = "Border Fortifications & Lockout";
            rec.summary = "Closed national borders and enhanced border guard presence.";
            rec.approval_delta = -3.0f;
            last_news_headline = "SECURITY ALERT: President seals international borders to curb illegal entries.";
            break;

        case DecreeType::CUSTOM_DECREE:
            if (custom_prompt.empty()) return;
            // Instant execution (0ms UI latency)
            approval_rating += 3.0f;
            civ.economy.gdp += 150.0f;
            last_news_headline = "PRESIDENTIAL DECREE: " + custom_prompt;
            rec.title = "Custom Executive Order";
            rec.summary = custom_prompt;
            rec.approval_delta = 3.0f;
            break;
    }

    decree_history.push_back(rec);
}

void AeonPresidentGame::declare_war(AeonEngine& engine, int target_civ_id) {
    if (!active || player_civ_id < 0 || player_civ_id >= (int)engine.civs.size()) return;
    if (target_civ_id < 0 || target_civ_id >= (int)engine.civs.size()) return;
    if (target_civ_id == player_civ_id) return;

    auto& player_civ = engine.civs[player_civ_id];
    auto& target_civ = engine.civs[target_civ_id];

    if (target_civ.is_alive <= 0.0f) return;

    player_civ.at_war = true;
    player_civ.war_with_civ = target_civ_id;
    player_civ.war_year_start = engine.year;

    target_civ.at_war = true;
    target_civ.war_with_civ = player_civ_id;

    player_civ.relations[target_civ_id] = DiplomacyStatus::AT_WAR;
    target_civ.relations[player_civ_id] = DiplomacyStatus::AT_WAR;

    last_news_headline = "⚔️ WAR DECLARED! President Alex Sterling orders full military invasion of " + target_civ.name + "!";

    PresidentialRecord rec;
    rec.year = engine.year;
    rec.title = "Declaration of War against " + target_civ.name;
    rec.summary = "Authorized full military invasion of " + target_civ.name + ". Army mobilized.";
    rec.approval_delta = 5.0f;
    decree_history.push_back(rec);

    engine.history.record(engine.year, engine.month, "WAR",
        player_civ.name + " declares war on " + target_civ.name,
        "Presidential Military Mandate", player_civ_id, target_civ_id);
}

void AeonPresidentGame::trigger_election(AeonEngine& engine) {
    (void)engine;
    // Election check based on Approval Rating
    float win_chance = approval_rating;
    int r = rand() % 100;

    if (r < (int)win_chance) {
        elections_won++;
        last_news_headline = "ELECTION VICTORY! President Alex Sterling re-elected for Term " + std::to_string(elections_won) + " with " + std::to_string((int)approval_rating) + "% vote mandate!";
    } else {
        election_loss = true;
        last_news_headline = "ELECTION LOSS: Opposition party wins national presidential election. Term ending.";
    }
}

std::string AeonPresidentGame::get_status_summary() const {
    std::ostringstream ss;
    ss << "Administration: " << administration_name << "\n"
       << "Leader: " << president_name << "\n"
       << "Approval Rating: " << approval_rating << "%\n"
       << "Coup Risk: " << coup_risk << "%\n"
       << "Years in Office: " << years_in_office << " (Term " << elections_won << ", Year " << term_counter << "/4)\n";
    return ss.str();
}

} // namespace Aeon
