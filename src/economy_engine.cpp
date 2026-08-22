#include "economy_engine.h"
#include <algorithm>

EconomyEngine::EconomyEngine() {
    // Initialize 13 resource market prices
    for (int i = 0; i <= (int)ResourceType::LABOR; ++i) {
        ResourceMarketPrice p;
        p.type = static_cast<ResourceType>(i);
        p.price = 10.0f + (i * 2.5f);
        p.supply = 1500.0f;
        p.demand = 1200.0f;
        p.inflation_rate = 0.02f;
        market_prices[i] = p;
    }
}

void EconomyEngine::update(float dt) {
    // Dynamic supply and demand price adjustments
    for (auto& pair : market_prices) {
        auto& p = pair.second;
        if (p.demand > p.supply) {
            p.price += 0.1f * dt;
        } else if (p.supply > p.demand) {
            p.price = std::max(1.0f, p.price - 0.05f * dt);
        }
    }
}

void EconomyEngine::process_trade(int exporter_id, int importer_id, ResourceType res, float amount) {
    int key = static_cast<int>(res);
    if (market_prices.find(key) != market_prices.end()) {
        market_prices[key].demand += amount * 0.1f;
    }
}
