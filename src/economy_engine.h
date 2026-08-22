#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "world_layer.h"

struct ResourceMarketPrice {
    ResourceType type;
    float price           = 10.0f; // Gold per unit
    float supply          = 1000.0f;
    float demand          = 1000.0f;
    float inflation_rate  = 0.02f; // % per tick
};

class EconomyEngine {
public:
    EconomyEngine();

    void update(float dt);

    std::unordered_map<int, ResourceMarketPrice> market_prices; // key: ResourceType

    float global_inflation = 0.02f;
    float global_gdp_growth = 0.035f;

    void process_trade(int exporter_id, int importer_id, ResourceType res, float amount);
};
