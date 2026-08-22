#ifndef AEON_SUPPLY_DEMAND_H
#define AEON_SUPPLY_DEMAND_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct CommodityMarketItem {
    std::string name;
    float base_price = 100.0f;
    float current_price = 100.0f;
    float global_supply = 1000.0f;
    float global_demand = 1000.0f;
    float price_change_pct = 0.0f;
};

class AeonSupplyDemandEngine {
public:
    std::vector<CommodityMarketItem> commodities;

    AeonSupplyDemandEngine();
    void init_default_commodities();
    void update_prices_tick(AeonEngine& engine);
    float get_price(const std::string& name) const;
};

} // namespace Aeon

#endif // AEON_SUPPLY_DEMAND_H
