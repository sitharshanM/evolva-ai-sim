#include "aeon_economy_market.h"
#include "aeon_engine.h"
#include <cstdlib>
#include <algorithm>

namespace Aeon {

void AeonEconomyMarketEngine::init() {
    stocks.clear();
    central_bank_interest_rate = 4.5f;
    inflation_rate = 2.4f;
    market_index_points = 5000.0f;

    StockCompany c1{ 1, "AEON", "Aeon Dynamics & Defense", 120.0f, 120.0f, 0 };
    StockCompany c2{ 2, "NRDG", "Nordra Energy & Oil", 85.0f, 85.0f, 0 };
    StockCompany c3{ 3, "APEX", "Apex Bio-Genetics Corp", 210.0f, 210.0f, 0 };
    StockCompany c4{ 4, "SOLR", "Solaris Orbital Tech", 340.0f, 340.0f, 0 };

    stocks.push_back(c1);
    stocks.push_back(c2);
    stocks.push_back(c3);
    stocks.push_back(c4);
}

void AeonEconomyMarketEngine::tick_year(AeonEngine& engine) {
    // 1. Stock Price Fluctuations
    float index_change = 0.0f;
    for (auto& s : stocks) {
        s.prev_price = s.share_price;
        float change_pct = ((rand() % 200 - 95) / 1000.0f); // -9.5% to +10.5%
        s.share_price = std::max(5.0f, s.share_price * (1.0f + change_pct));
        index_change += (s.share_price - s.prev_price);
    }
    market_index_points = std::max(1000.0f, market_index_points + index_change * 10.0f);

    // 2. Inflation & Interest Rate Effects
    inflation_rate = std::max(0.5f, std::min(18.0f, 2.0f + (5.0f - central_bank_interest_rate) * 0.8f + (rand() % 20 - 10) * 0.1f));

    auto& p = engine.president_game;
    if (p.active && p.player_civ_id >= 0 && p.player_civ_id < (int)engine.civs.size()) {
        auto& civ = engine.civs[p.player_civ_id];
        // High inflation reduces approval & GDP real growth
        if (inflation_rate > 8.0f) {
            p.approval_rating -= 2.0f;
        }
        civ.economy.gdp *= (1.0f - (inflation_rate - 2.0f) * 0.005f);
    }
}

bool AeonEconomyMarketEngine::buy_stock(AeonEngine& engine, int company_id, int shares) {
    auto& p = engine.president_game;
    for (auto& s : stocks) {
        if (s.id == company_id) {
            float total_cost = s.share_price * shares;
            if (p.treasury_gold >= total_cost) {
                p.treasury_gold -= total_cost;
                s.player_shares_owned += shares;
                p.last_news_headline = "STOCK MARKET: Purchased " + std::to_string(shares) + " shares of " + s.ticker + " ($" + std::to_string((int)total_cost) + ").";
                return true;
            }
        }
    }
    return false;
}

bool AeonEconomyMarketEngine::sell_stock(AeonEngine& engine, int company_id, int shares) {
    auto& p = engine.president_game;
    for (auto& s : stocks) {
        if (s.id == company_id) {
            if (s.player_shares_owned >= shares) {
                float return_gold = s.share_price * shares;
                s.player_shares_owned -= shares;
                p.treasury_gold += return_gold;
                p.last_news_headline = "STOCK MARKET: Sold " + std::to_string(shares) + " shares of " + s.ticker + " for $" + std::to_string((int)return_gold) + " Gold.";
                return true;
            }
        }
    }
    return false;
}

bool AeonEconomyMarketEngine::adjust_interest_rate(AeonEngine& engine, float new_rate) {
    central_bank_interest_rate = std::max(0.0f, std::min(20.0f, new_rate));
    engine.president_game.last_news_headline = "MONETARY POLICY: Central Bank adjusts benchmark interest rate to " + std::to_string((int)central_bank_interest_rate) + ".0%.";
    return true;
}

} // namespace Aeon
