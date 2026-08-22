#pragma once
#include "aeon_config.h"
#include "aeon_types.h"
#include "aeon_world_types.h"
#include "aeon_civilization.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  GlobalMarketEngine  —  Manages supply/demand prices and inter-city trade
// ─────────────────────────────────────────────────────────────────────────────
class GlobalMarketEngine {
public:
    GlobalMarketEngine() = default;

    void init();
    void tick_year(std::vector<AeonCivilization>& civs, int year);

    // Get current price of a resource (gold per unit)
    float price_of(ResourceKind res) const;
    std::string market_report() const;

    bool establish_manual_trade_route(std::vector<AeonCivilization>& civs, int civ1_id, int civ2_id, int year);
    bool cancel_trade_route_sanctions(std::vector<AeonCivilization>& civs, int civ1_id, int civ2_id, int year);

    std::vector<TradeRoute> active_routes;

private:
    std::unordered_map<int, float> prices_; // ResourceKind enum int -> float price
    int next_route_id = 1;

    void update_prices(const std::vector<AeonCivilization>& civs);
    void update_trade_routes(std::vector<AeonCivilization>& civs, int year);
};

} // namespace Aeon
