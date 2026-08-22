#include "aeon_supply_demand.h"
#include "aeon_engine.h"
#include <algorithm>
#include <cmath>

namespace Aeon {

AeonSupplyDemandEngine::AeonSupplyDemandEngine() {
    init_default_commodities();
}

void AeonSupplyDemandEngine::init_default_commodities() {
    commodities = {
        {"Crude Oil ($/bbl)", 75.0f, 75.0f, 5000.0f, 5200.0f, 0.0f},
        {"Iron Ore ($/ton)", 110.0f, 110.0f, 8000.0f, 7800.0f, 0.0f},
        {"Wheat Grain ($/bushel)", 6.50f, 6.50f, 12000.0f, 12500.0f, 0.0f},
        {"Rare Earth Metals ($/kg)", 240.0f, 240.0f, 1500.0f, 1900.0f, 0.0f},
        {"Semiconductor Microchips ($/unit)", 450.0f, 450.0f, 3000.0f, 3400.0f, 0.0f}
    };
}

void AeonSupplyDemandEngine::update_prices_tick(AeonEngine& engine) {
    (void)engine;
    for (auto& item : commodities) {
        // Organic random market noise
        float noise = ((float)(rand() % 100) / 100.0f - 0.5f) * 50.0f;
        item.global_demand = std::max(100.0f, item.global_demand + noise);

        // Real Economic Elasticity Formula: Price = Base * (1 + (Demand - Supply) / Supply)
        float supply_denom = std::max(1.0f, item.global_supply);
        float elasticity_ratio = (item.global_demand - item.global_supply) / supply_denom;
        float target_price = item.base_price * (1.0f + std::clamp(elasticity_ratio, -0.7f, 2.5f));

        float old_price = item.current_price;
        // Smooth exponential moving average update
        item.current_price = item.current_price * 0.85f + target_price * 0.15f;

        if (old_price > 0.001f) {
            item.price_change_pct = ((item.current_price - old_price) / old_price) * 100.0f;
        }
    }
}

float AeonSupplyDemandEngine::get_price(const std::string& name) const {
    for (const auto& item : commodities) {
        if (item.name.find(name) != std::string::npos) return item.current_price;
    }
    return 100.0f;
}

} // namespace Aeon
