#ifndef AEON_CENTRAL_BANK_H
#define AEON_CENTRAL_BANK_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct NationalCurrency {
    int civ_id = 0;
    std::string civ_name;
    std::string currency_name = "Credits";
    std::string symbol = "CR";
    double money_supply_millions = 50000.0;
    float inflation_rate = 2.5f;          // Percentage e.g. 2.5%
    float interest_rate = 4.0f;           // Central Bank Benchmark Rate e.g. 4.0%
    float exchange_rate_vs_reserve = 1.0f;// Value relative to Sol Credit (reserve)
    float war_finance_printing = 0.0f;    // Money printed for current war effort
    double national_debt_millions = 12000.0;
    bool is_in_default = false;
    // Accuracy additions
    float  reserve_ratio         = 0.10f; // fraction of deposits held in reserve
    float  credit_multiplier     = 10.0f; // money multiplier from fractional reserve
    bool   is_reserve_currency   = false; // global reserve currency status
    float  speculative_pressure  = 0.0f;  // 0-1; foreign speculation vs this currency
    int    debt_restructuring_year = -1;  // year last restructuring happened
    float  deposit_flight_pct    = 0.0f;  // fraction withdrawn during bank panic
};

// ─── Banking Crisis ────────────────────────────────────────────────────────────
struct BankingCrisis {
    int   civ_id          = -1;
    int   panic_year      = 0;
    float deposit_flight_pct = 0.3f;  // 30% of deposits withdrawn
    bool  resolved        = false;
    int   years_active    = 0;
};

class AeonCentralBankEngine {
public:
    std::vector<NationalCurrency> currencies;
    std::vector<BankingCrisis>    banking_crises;
    int reserve_currency_civ_id = 0;  // which civ holds reserve currency privilege

    AeonCentralBankEngine();
    void init_default_currencies(const AeonEngine& engine);
    void update_central_banks_tick(AeonEngine& engine);

    const NationalCurrency* get_currency(int civ_id) const;
    void print_war_money(int civ_id, double amount_millions);
    void adjust_interest_rate(int civ_id, float delta);
    void execute_quantitative_easing(int civ_id, double amount_millions);
    void check_hyperinflation(AeonEngine& engine);
    void trigger_banking_panic(int civ_id, AeonEngine& engine);
    void attempt_debt_restructuring(int civ_id, AeonEngine& engine);
    void update_reserve_currency(AeonEngine& engine);
    void advance_business_cycle(AeonEngine& engine);
};

} // namespace Aeon

#endif // AEON_CENTRAL_BANK_H
