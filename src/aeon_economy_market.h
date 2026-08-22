#ifndef AEON_ECONOMY_MARKET_H
#define AEON_ECONOMY_MARKET_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct StockCompany {
    int id = 0;
    std::string ticker;
    std::string name;
    float share_price = 100.0f;
    float prev_price = 100.0f;
    int player_shares_owned = 0;
};

class AeonEconomyMarketEngine {
public:
    float central_bank_interest_rate = 4.5f; // %
    float inflation_rate = 2.4f; // %
    float market_index_points = 5000.0f;
    std::vector<StockCompany> stocks;

    void init();
    void tick_year(AeonEngine& engine);
    bool buy_stock(AeonEngine& engine, int company_id, int shares);
    bool sell_stock(AeonEngine& engine, int company_id, int shares);
    bool adjust_interest_rate(AeonEngine& engine, float new_rate);
};

} // namespace Aeon

#endif // AEON_ECONOMY_MARKET_H
