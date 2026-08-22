#include "aeon_tech_tree.h"
#include "aeon_civilization.h"
#include "aeon_engine.h"
#include <algorithm>

namespace Aeon {

TechTreeEngine::TechTreeEngine() {
    init_default_tree();
}

void TechTreeEngine::init_default_tree() {
    nodes.clear();
    policies.clear();

    // ── AGRICULTURE ──────────────────────────────────────────────────────────
    {
        TechNode n;
        n.id = "crop_rotation"; n.name = "Crop Rotation"; n.era = TechEra::AGRICULTURE; n.branch = TechBranch::AGRICULTURE;
        n.description = "Systematic soil rotation boosts food production by 30%.";
        n.research_cost = 80.0f; n.food_yield_mult = 1.30f;
        nodes.push_back(n);
    }
    {
        TechNode n;
        n.id = "mechanized_agriculture"; n.name = "Mechanized Agriculture"; n.era = TechEra::INDUSTRIALIZATION; n.branch = TechBranch::AGRICULTURE;
        n.description = "Tractors and combine harvesters (+50% food production).";
        n.research_cost = 400.0f; n.prerequisites = {"crop_rotation"}; n.food_yield_mult = 1.50f;
        nodes.push_back(n);
    }
    {
        TechNode n;
        n.id = "genetic_crops"; n.name = "Biotech Agriculture"; n.era = TechEra::MODERN; n.branch = TechBranch::AGRICULTURE;
        n.description = "Drought-resistant crops and hydroponics (+80% food production).";
        n.research_cost = 1000.0f; n.prerequisites = {"mechanized_agriculture"}; n.food_yield_mult = 1.80f;
        nodes.push_back(n);
    }

    // ── MILITARY ─────────────────────────────────────────────────────────────
    {
        TechNode n;
        n.id = "bronze_smelting"; n.name = "Bronze Smelting"; n.era = TechEra::METALLURGY; n.branch = TechBranch::MILITARY;
        n.description = "Allows creation of bronze armor and weapons (+15% military).";
        n.research_cost = 100.0f; n.military_mult = 1.15f;
        nodes.push_back(n);
    }
    {
        TechNode n;
        n.id = "iron_metallurgy"; n.name = "Iron Metallurgy"; n.era = TechEra::METALLURGY; n.branch = TechBranch::MILITARY;
        n.description = "Forges high-durability iron armor and blades (+25% military).";
        n.research_cost = 180.0f; n.prerequisites = {"bronze_smelting"}; n.military_mult = 1.25f;
        nodes.push_back(n);
    }
    {
        TechNode n;
        n.id = "gunpowder"; n.name = "Gunpowder Warfare"; n.era = TechEra::INDUSTRIALIZATION; n.branch = TechBranch::MILITARY;
        n.description = "Cannons and muskets revolutionize army combat (+40% military).";
        n.research_cost = 450.0f; n.prerequisites = {"iron_metallurgy"}; n.military_mult = 1.40f;
        nodes.push_back(n);
    }
    {
        TechNode n;
        n.id = "combined_arms"; n.name = "Combined Arms Doctrine"; n.era = TechEra::MODERN; n.branch = TechBranch::MILITARY;
        n.description = "Armor, artillery, and air coordination (+60% military).";
        n.research_cost = 1100.0f; n.prerequisites = {"gunpowder"}; n.military_mult = 1.60f;
        nodes.push_back(n);
    }

    // ── INDUSTRY ─────────────────────────────────────────────────────────────
    {
        TechNode n;
        n.id = "steam_engine"; n.name = "Steam Engine"; n.era = TechEra::INDUSTRIALIZATION; n.branch = TechBranch::INDUSTRY;
        n.description = "Mechanized factories and locomotives (+50% industry).";
        n.research_cost = 400.0f; n.prerequisites = {"iron_metallurgy"}; n.industry_mult = 1.50f;
        nodes.push_back(n);
    }
    {
        TechNode n;
        n.id = "industrial_automation"; n.name = "Industrial Automation"; n.era = TechEra::COMPUTING; n.branch = TechBranch::INDUSTRY;
        n.description = "Assembly line robotics double heavy industrial output (+100% industry).";
        n.research_cost = 900.0f; n.prerequisites = {"steam_engine"}; n.industry_mult = 2.00f;
        nodes.push_back(n);
    }

    // ── SCIENCE & MEDICINE ───────────────────────────────────────────────────
    {
        TechNode n;
        n.id = "scientific_method"; n.name = "Scientific Method"; n.era = TechEra::SCIENCE; n.branch = TechBranch::SCIENCE;
        n.description = "Rigorous empirical experimentation (+35% science research).";
        n.research_cost = 300.0f; n.science_mult = 1.35f;
        nodes.push_back(n);
    }
    {
        TechNode n;
        n.id = "antibiotics"; n.name = "Antibiotics & Public Health"; n.era = TechEra::MODERN; n.branch = TechBranch::MEDICINE;
        n.description = "Eradicates lethal bacterial plagues (+40% population health, +20% life expectancy).";
        n.research_cost = 600.0f; n.health_mult = 1.40f;
        nodes.push_back(n);
    }
    {
        TechNode n;
        n.id = "gene_therapy"; n.name = "Cellular Gene Therapy"; n.era = TechEra::ADVANCED; n.branch = TechBranch::MEDICINE;
        n.description = "Genetic immortality research and disease immunization (+70% health).";
        n.research_cost = 1600.0f; n.prerequisites = {"antibiotics"}; n.health_mult = 1.70f;
        nodes.push_back(n);
    }

    // ── ENERGY & NUCLEAR ─────────────────────────────────────────────────────
    {
        TechNode n;
        n.id = "electricity"; n.name = "Electrical Power"; n.era = TechEra::ELECTRICITY; n.branch = TechBranch::ENERGY;
        n.description = "Electrification enables modern communications and automation (+40% industry, +30% science).";
        n.research_cost = 800.0f; n.prerequisites = {"steam_engine"}; n.industry_mult = 1.40f; n.science_mult = 1.30f; n.energy_mult = 1.50f;
        nodes.push_back(n);
    }
    {
        TechNode n;
        n.id = "atom_splitting"; n.name = "Nuclear Fission"; n.era = TechEra::MODERN; n.branch = TechBranch::ENERGY;
        n.description = "Unlocks nuclear reactors and strategic atomic deterrents (+80% military, +100% energy).";
        n.research_cost = 1300.0f; n.prerequisites = {"electricity"}; n.unlocks_nukes = true; n.military_mult = 1.80f; n.energy_mult = 2.00f;
        nodes.push_back(n);
    }
    {
        TechNode n;
        n.id = "fusion_power"; n.name = "Nuclear Fusion"; n.era = TechEra::ADVANCED; n.branch = TechBranch::ENERGY;
        n.description = "Limitless clean energy from sustained hydrogen plasma (+300% energy).";
        n.research_cost = 2200.0f; n.prerequisites = {"atom_splitting"}; n.energy_mult = 3.00f;
        nodes.push_back(n);
    }

    // ── AI & CYBERNETICS ─────────────────────────────────────────────────────
    {
        TechNode n;
        n.id = "ai_synthesis"; n.name = "AI Neural Synthesis"; n.era = TechEra::ADVANCED; n.branch = TechBranch::AI;
        n.description = "Autonomous AI management optimizes all production and science (+100% science, +60% industry).";
        n.research_cost = 2000.0f; n.prerequisites = {"electricity"}; n.science_mult = 2.0f; n.industry_mult = 1.60f;
        nodes.push_back(n);
    }

    // ── ECONOMICS ────────────────────────────────────────────────────────────
    {
        TechNode n;
        n.id = "central_banking"; n.name = "Central Banking System"; n.era = TechEra::INDUSTRIALIZATION; n.branch = TechBranch::ECONOMICS;
        n.description = "National fractional reserves and sovereign monetary stability (+40% trade revenue).";
        n.research_cost = 500.0f; n.trade_mult = 1.40f;
        nodes.push_back(n);
    }
    {
        TechNode n;
        n.id = "algorithmic_markets"; n.name = "Algorithmic Global Markets"; n.era = TechEra::MODERN; n.branch = TechBranch::ECONOMICS;
        n.description = "High-frequency trade settlements and global liquidity networks (+80% trade).";
        n.research_cost = 1100.0f; n.prerequisites = {"central_banking"}; n.trade_mult = 1.80f;
        nodes.push_back(n);
    }

    // ── NAVAL ────────────────────────────────────────────────────────────────
    {
        TechNode n;
        n.id = "steam_ironclads"; n.name = "Armored Ironclads"; n.era = TechEra::INDUSTRIALIZATION; n.branch = TechBranch::NAVAL;
        n.description = "Steam-powered metal warships secure coastal sovereignty (+35% military).";
        n.research_cost = 450.0f; n.prerequisites = {"steam_engine"}; n.military_mult = 1.35f;
        nodes.push_back(n);
    }

    // ── AEROSPACE ────────────────────────────────────────────────────────────
    {
        TechNode n;
        n.id = "orbital_rocketry"; n.name = "Orbital Rocketry"; n.era = TechEra::MODERN; n.branch = TechBranch::AEROSPACE;
        n.description = "Launches reconnaissance satellites and long-range ballistic vectors (+50% military).";
        n.research_cost = 1200.0f; n.prerequisites = {"electricity"}; n.military_mult = 1.50f;
        nodes.push_back(n);
    }
    {
        TechNode n;
        n.id = "orbital_defense"; n.name = "Orbital Defense Grid"; n.era = TechEra::ADVANCED; n.branch = TechBranch::AEROSPACE;
        n.description = "Space-based kinetic defense satellites and planetary sensors (+100% military).";
        n.research_cost = 2500.0f; n.prerequisites = {"ai_synthesis", "orbital_rocketry"}; n.unlocks_orbital = true; n.military_mult = 2.0f;
        nodes.push_back(n);
    }

    // Policies
    policies.push_back({"mil_conscription", "Mass Conscription", IdeologyTree::MILITARISM, 1, false, "+25% Army Size"});
    policies.push_back({"mil_blitzkrieg", "Grand Conquest", IdeologyTree::MILITARISM, 2, false, "+35% Military Power"});
    policies.push_back({"sci_academy", "Royal Academy", IdeologyTree::SCIENTISM, 1, false, "+30% Research Speed"});
    policies.push_back({"sci_technocracy", "Technocratic Rule", IdeologyTree::SCIENTISM, 2, false, "+50% Science Output"});
    policies.push_back({"mer_free_trade", "Free Trade Charter", IdeologyTree::MERCANTILISM, 1, false, "+40% Merchant Revenue"});
    policies.push_back({"div_zealotry", "Holy Inquisitions", IdeologyTree::DIVINE_EMPIRE, 1, false, "+35% Empire Stability"});
}

bool TechTreeEngine::can_research(int civ_tech_pts, const std::string& tech_id, const std::vector<std::string>& unlocked_list) const {
    const TechNode* node = find_node(tech_id);
    if (!node) return false;
    if (std::find(unlocked_list.begin(), unlocked_list.end(), tech_id) != unlocked_list.end()) {
        return false; // Already unlocked
    }
    for (const auto& req : node->prerequisites) {
        if (std::find(unlocked_list.begin(), unlocked_list.end(), req) == unlocked_list.end()) {
            return false; // Missing prerequisite
        }
    }
    return civ_tech_pts >= node->research_cost;
}

const TechNode* TechTreeEngine::find_node(const std::string& tech_id) const {
    for (const auto& n : nodes) {
        if (n.id == tech_id) return &n;
    }
    return nullptr;
}

void TechTreeEngine::apply_tech_effects(AeonCivilization& civ) {
    // Dynamic multipliers computed from unlocked tech nodes
    float food_mult = 1.0f;
    float ind_mult  = 1.0f;
    float mil_mult  = 1.0f;
    float sci_mult  = 1.0f;
    float trade_mult= 1.0f;
    float health_mult = 1.0f;
    float energy_mult = 1.0f;

    for (const auto& tech_id : civ.tech.unlocked_nodes) {
        const auto* n = find_node(tech_id);
        if (n) {
            food_mult   *= n->food_yield_mult;
            ind_mult    *= n->industry_mult;
            mil_mult    *= n->military_mult;
            sci_mult    *= n->science_mult;
            trade_mult  *= n->trade_mult;
            health_mult *= n->health_mult;
            energy_mult *= n->energy_mult;
        }
    }

    civ.economy.gdp_growth = 0.025f * ind_mult * trade_mult + (civ.tech.progress / 600.0f);
    civ.resources.energy  = std::max(50.0f, civ.resources.energy * energy_mult);
    civ.resources.food    = std::max(50.0f, civ.resources.food * food_mult);
    civ.population.food_security = std::clamp(civ.population.food_security * food_mult, 20.0f, 100.0f);
    civ.population.health = std::clamp(civ.population.health * health_mult, 20.0f, 100.0f);
    civ.military_power    = civ.army_size * 0.12f * mil_mult;
    civ.tech.research_pts += (sci_mult - 1.0f) * 10.0f;
}

void TechTreeEngine::process_ideology_cold_wars(AeonEngine& engine) {
    (void)engine;
    // Cold war dynamics: civs with high military focus create tension with high science focus
}

void TechTreeEngine::process_brain_drain_tick(AeonEngine& engine) {
    for (size_t i = 0; i < engine.civs.size(); ++i) {
        auto& origin = engine.civs[i];
        if (origin.is_alive <= 0.0f || origin.stability > 45.0f) continue;

        // Civ with low stability & happiness experiences brain drain to high culture neighbors
        for (size_t j = 0; j < engine.civs.size(); ++j) {
            if (i == j) continue;
            auto& dest = engine.civs[j];
            if (dest.is_alive <= 0.0f) continue;

            if (dest.cultural_prestige > origin.cultural_prestige + 25.0f && dest.stability > 70.0f) {
                float emigrants = origin.population.total * 0.005f; // 0.5% educated brain drain
                emigrants = std::min(emigrants, 50000.0f);
                origin.population.total = std::max(1000LL, origin.population.total - (long long)emigrants);
                dest.population.total += (long long)emigrants;
                dest.tech.research_pts += emigrants * 0.1f; // Scientists arrive
                break;
            }
        }
    }
}

} // namespace Aeon
