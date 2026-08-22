#include "aeon_economy.h"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace Aeon {

void GlobalMarketEngine::init() {
    // Base prices in gold per unit
    prices_[int(ResourceKind::FOOD)]          = 1.0f;
    prices_[int(ResourceKind::WATER)]         = 0.5f;
    prices_[int(ResourceKind::WOOD)]          = 2.0f;
    prices_[int(ResourceKind::STONE)]         = 2.0f;
    prices_[int(ResourceKind::IRON)]          = 5.0f;
    prices_[int(ResourceKind::COPPER)]        = 4.0f;
    prices_[int(ResourceKind::COAL)]          = 6.0f;
    prices_[int(ResourceKind::OIL)]           = 12.0f;
    prices_[int(ResourceKind::NATURAL_GAS)]   = 10.0f;
    prices_[int(ResourceKind::RARE_MINERALS)] = 25.0f;
    prices_[int(ResourceKind::ENERGY)]        = 8.0f;
    prices_[int(ResourceKind::KNOWLEDGE)]     = 15.0f;
    prices_[int(ResourceKind::TECH)]          = 30.0f;
    prices_[int(ResourceKind::GOLD_ORE)]      = 50.0f;
}

float GlobalMarketEngine::price_of(ResourceKind res) const {
    auto it = prices_.find(int(res));
    return (it != prices_.end()) ? it->second : 1.0f;
}

void GlobalMarketEngine::tick_year(std::vector<AeonCivilization>& civs, int year) {
    update_prices(civs);
    update_trade_routes(civs, year);
}

void GlobalMarketEngine::update_prices(const std::vector<AeonCivilization>& civs) {
    // Aggregate global supply vs demand
    float total_food_supply = 0.0f;
    float total_pop = 0.0f;

    for (const auto& c : civs) {
        if (c.is_alive <= 0.0f) continue;
        total_food_supply += c.resources.food;
        total_pop += float(c.population.total);
    }

    // Scarcity adjusts food prices
    float food_demand = total_pop * 0.0002f;
    if (food_demand > 0.0f) {
        float ratio = total_food_supply / food_demand;
        prices_[int(ResourceKind::FOOD)] = std::max(0.5f, std::min(5.0f, 1.0f / ratio));
    }
}

void GlobalMarketEngine::update_trade_routes(std::vector<AeonCivilization>& civs, int year) {
    // Dynamically form trade routes between friendly civs
    for (size_t i = 0; i < civs.size(); ++i) {
        for (size_t j = i + 1; j < civs.size(); ++j) {
            auto& c1 = civs[i];
            auto& c2 = civs[j];
            if (c1.is_alive <= 0.0f || c2.is_alive <= 0.0f) continue;
            if (c1.at_war || c2.at_war) continue;

            // Ally or Trade partner relation -> boost economy
            auto rel1 = c1.relations.find(c2.id);
            if (rel1 != c1.relations.end() &&
               (rel1->second == DiplomacyStatus::ALLY || rel1->second == DiplomacyStatus::TRADE_PARTNER)) {
                c1.economy.gdp += 50.0f;
                c2.economy.gdp += 50.0f;

                // Check if route exists
                bool exists = false;
                for (const auto& r : active_routes) {
                    if ((r.civ_a == c1.id && r.civ_b == c2.id) ||
                        (r.civ_a == c2.id && r.civ_b == c1.id)) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    TradeRoute tr;
                    tr.id = next_route_id++;
                    tr.civ_a = c1.id;
                    tr.civ_b = c2.id;
                    tr.origin_city = c1.name + " Capital";
                    tr.dest_city   = c2.name + " Capital";
                    tr.primary_good = "Wood & Iron";
                    tr.annual_volume = 200.0f;
                    tr.active = true;
                    active_routes.push_back(tr);

                    std::cout << "[YEAR " << year << "] TRADE ROUTE FORMED between "
                              << c1.name << " and " << c2.name << "!" << std::endl;
                }
            }
        }
    }
}

std::string GlobalMarketEngine::market_report() const {
    std::ostringstream ss;
    ss << "--- GLOBAL MARKET PRICES ---\n";
    static const ResourceKind all_res[] = {
        ResourceKind::FOOD, ResourceKind::WATER, ResourceKind::WOOD, ResourceKind::STONE,
        ResourceKind::IRON, ResourceKind::COPPER, ResourceKind::COAL, ResourceKind::OIL,
        ResourceKind::NATURAL_GAS, ResourceKind::RARE_MINERALS, ResourceKind::ENERGY,
        ResourceKind::KNOWLEDGE, ResourceKind::TECH, ResourceKind::GOLD_ORE
    };
    for (auto r : all_res) {
        ss << "  " << resource_name(r) << " : "
           << price_of(r) << " gold/unit\n";
    }
    ss << "--- ACTIVE TRADE ROUTES (" << active_routes.size() << ") ---\n";
    for (const auto& tr : active_routes) {
        if (tr.active)
            ss << "  " << tr.origin_city << " <-> " << tr.dest_city
               << " (Volume: " << int(tr.annual_volume) << " gold/yr)\n";
    }
    return ss.str();
}

bool GlobalMarketEngine::establish_manual_trade_route(std::vector<AeonCivilization>& civs, int civ1_id, int civ2_id, int year) {
    if (civ1_id < 0 || civ1_id >= (int)civs.size() || civ2_id < 0 || civ2_id >= (int)civs.size()) return false;
    auto& c1 = civs[civ1_id];
    auto& c2 = civs[civ2_id];

    c1.relations[c2.id] = DiplomacyStatus::TRADE_PARTNER;
    c2.relations[c1.id] = DiplomacyStatus::TRADE_PARTNER;

    c1.economy.gdp += 300.0f;
    c2.economy.gdp += 300.0f;

    TradeRoute tr;
    tr.id = next_route_id++;
    tr.civ_a = c1.id;
    tr.civ_b = c2.id;
    tr.origin_city = c1.name + " Port";
    tr.dest_city   = c2.name + " Port";
    tr.primary_good = "Strategic Goods & Tech";
    tr.annual_volume = 500.0f;
    tr.active = true;
    active_routes.push_back(tr);

    std::cout << "[YEAR " << year << "] PRESIDENTIAL TRADE DEAL SIGNED between " << c1.name << " and " << c2.name << "!\n";
    return true;
}

bool GlobalMarketEngine::cancel_trade_route_sanctions(std::vector<AeonCivilization>& civs, int civ1_id, int civ2_id, int year) {
    if (civ1_id < 0 || civ1_id >= (int)civs.size() || civ2_id < 0 || civ2_id >= (int)civs.size()) return false;
    auto& c1 = civs[civ1_id];
    auto& c2 = civs[civ2_id];

    c1.relations[c2.id] = DiplomacyStatus::EMBARGOED;
    c2.relations[c1.id] = DiplomacyStatus::EMBARGOED;

    c2.economy.gdp = std::max(100.0f, c2.economy.gdp * 0.80f); // 20% GDP hit

    active_routes.erase(std::remove_if(active_routes.begin(), active_routes.end(),
        [&](const TradeRoute& r) {
            return (r.civ_a == c1.id && r.civ_b == c2.id) || (r.civ_a == c2.id && r.civ_b == c1.id);
        }), active_routes.end());

    std::cout << "[YEAR " << year << "] ECONOMIC SANCTIONS & EMBARGO ENFORCED by " << c1.name << " on " << c2.name << "!\n";
    return true;
}

} // namespace Aeon
