#include "aeon_central_bank.h"
#include "aeon_engine.h"
#include <algorithm>
#include <cmath>

namespace Aeon {

AeonCentralBankEngine::AeonCentralBankEngine() {
}

void AeonCentralBankEngine::init_default_currencies(const AeonEngine& engine) {
    currencies.clear();

    const char* default_names[] = {"Sol Sovereign Credit", "Nordra Imperial Mark", "Aethel Crown", "Vanguard Dinar", "Commons Token"};
    const char* default_syms[]  = {"SOL", "NIM", "ATC", "VGD", "CTK"};

    for (size_t i = 0; i < engine.civs.size(); ++i) {
        const auto& civ = engine.civs[i];
        NationalCurrency cur;
        cur.civ_id = civ.id;
        cur.civ_name = civ.name;
        cur.currency_name = (i < 5) ? default_names[i] : (civ.name + " Peso");
        cur.symbol = (i < 5) ? default_syms[i] : "PSO";
        cur.money_supply_millions = 50000.0 + (i * 12000.0);
        cur.inflation_rate = 2.1f + (i * 0.4f);
        cur.interest_rate = 4.5f - (i * 0.3f);
        cur.exchange_rate_vs_reserve = (i == 0) ? 1.0f : (0.85f - (i * 0.12f));
        cur.war_finance_printing = 0.0f;
        currencies.push_back(cur);
    }
}

void AeonCentralBankEngine::update_central_banks_tick(AeonEngine& engine) {
    for (auto& cur : currencies) {
        // Find corresponding civ
        bool in_war = false;
        for (const auto& civ : engine.civs) {
            if (civ.id == cur.civ_id) {
                in_war = civ.at_war;
                break;
            }
        }

        // War financing increases money supply
        if (in_war) {
            cur.war_finance_printing = 2500.0f; // Print 2.5B per year
            cur.money_supply_millions += cur.war_finance_printing;
            cur.inflation_rate += 1.8f; // War inflation spike
        } else {
            cur.war_finance_printing = 0.0f;
            // Central bank interest rate cooling
            if (cur.inflation_rate > 3.0f) {
                cur.interest_rate = std::min(12.0f, cur.interest_rate + 0.5f);
                cur.inflation_rate = std::max(1.5f, cur.inflation_rate - 0.7f);
            } else if (cur.inflation_rate < 1.5f) {
                cur.interest_rate = std::max(0.5f, cur.interest_rate - 0.25f);
                cur.inflation_rate += 0.3f;
            }
        }

        // National Debt Accumulation & Default Risk
        cur.national_debt_millions += cur.money_supply_millions * (cur.interest_rate / 100.0) * 0.05;
        if (cur.national_debt_millions > cur.money_supply_millions * 2.5) {
            cur.is_in_default = true;
            cur.exchange_rate_vs_reserve *= 0.8f;
        } else {
            cur.is_in_default = false;
        }

        // Exchange rate floats based on inflation differential vs Sol (reserve civ 0)
        float base_inflation = currencies.empty() ? 2.0f : currencies[0].inflation_rate;
        float diff = cur.inflation_rate - base_inflation;
        if (cur.civ_id != 0) {
            cur.exchange_rate_vs_reserve = std::max(0.05f, cur.exchange_rate_vs_reserve * (1.0f - (diff * 0.02f)));
        }
    }
}

const NationalCurrency* AeonCentralBankEngine::get_currency(int civ_id) const {
    for (const auto& cur : currencies) {
        if (cur.civ_id == civ_id) return &cur;
    }
    return nullptr;
}

void AeonCentralBankEngine::print_war_money(int civ_id, double amount_millions) {
    for (auto& cur : currencies) {
        if (cur.civ_id == civ_id) {
            cur.money_supply_millions += amount_millions;
            cur.war_finance_printing += (float)amount_millions;
            cur.inflation_rate += (float)(amount_millions / 1000.0) * 0.4f;
            break;
        }
    }
}

void AeonCentralBankEngine::adjust_interest_rate(int civ_id, float delta) {
    for (auto& cur : currencies) {
        if (cur.civ_id == civ_id) {
            cur.interest_rate = std::max(0.1f, std::min(25.0f, cur.interest_rate + delta));
            break;
        }
    }
}

void AeonCentralBankEngine::execute_quantitative_easing(int civ_id, double amount_millions) {
    for (auto& cur : currencies) {
        if (cur.civ_id == civ_id) {
            cur.money_supply_millions += amount_millions;
            cur.inflation_rate += (float)(amount_millions / 2000.0);
            cur.interest_rate = std::max(0.2f, cur.interest_rate - 0.5f);
            break;
        }
    }
}

// ─── check_hyperinflation ─────────────────────────────────────────────────────
void AeonCentralBankEngine::check_hyperinflation(AeonEngine& engine) {
    for (auto& cur : currencies) {
        // Hyperinflation trigger: war printing > 30% of money supply
        if (cur.war_finance_printing > cur.money_supply_millions * 0.0003f) {
            cur.inflation_rate *= 1.4f; // Exponential spiral
            cur.exchange_rate_vs_reserve *= 0.85f;
        }
        // Clamp at hyperinflation ceiling
        if (cur.inflation_rate > 500.0f) cur.inflation_rate = 500.0f;

        // Feedback: high inflation tanks GDP growth
        for (auto& civ : engine.civs) {
            if (civ.id == cur.civ_id && civ.is_alive > 0.0f) {
                float inflation_penalty = (cur.inflation_rate - 5.0f) * 0.002f;
                civ.economy.gdp_growth -= std::max(0.0f, inflation_penalty);
                // Interest rate → investment feedback
                float rate_penalty = (cur.interest_rate - 4.0f) * 0.005f;
                civ.economy.gdp_growth -= rate_penalty;
                civ.economy.gdp_growth = std::max(-0.15f, civ.economy.gdp_growth);
                break;
            }
        }
    }
}

// ─── trigger_banking_panic ────────────────────────────────────────────────────
void AeonCentralBankEngine::trigger_banking_panic(int civ_id, AeonEngine& engine) {
    for (auto& cur : currencies) {
        if (cur.civ_id != civ_id) continue;
        // Deposit flight: 30% of money supply vanishes
        cur.deposit_flight_pct = 0.30f;
        cur.money_supply_millions *= (1.0f - cur.deposit_flight_pct);
        cur.is_in_default = true;
        BankingCrisis crisis;
        crisis.civ_id = civ_id;
        crisis.panic_year = engine.year;
        crisis.deposit_flight_pct = cur.deposit_flight_pct;
        banking_crises.push_back(crisis);

        // GDP shock
        for (auto& civ : engine.civs) {
            if (civ.id == civ_id) {
                civ.economy.gdp *= 0.85f;
                civ.population.happiness -= 15.0f;
                civ.stability -= 10.0f;
                engine.history.record(engine.year, engine.month, "ECONOMY",
                    civ.name + " banking panic",
                    "Deposit flight collapses credit; GDP contracts 15%.", civ_id);
                break;
            }
        }
        // Contagion: trade partners get inflation bump
        for (auto& civ : engine.civs) {
            if (civ.id == civ_id || civ.is_alive <= 0.0f) continue;
            if (civ.relations.count(civ_id) &&
                civ.relations.at(civ_id) == DiplomacyStatus::TRADE_PARTNER) {
                for (auto& partner_cur : currencies) {
                    if (partner_cur.civ_id == civ.id) {
                        partner_cur.inflation_rate += 1.5f;
                        break;
                    }
                }
            }
        }
        break;
    }
}

// ─── attempt_debt_restructuring ───────────────────────────────────────────────
void AeonCentralBankEngine::attempt_debt_restructuring(int civ_id, AeonEngine& engine) {
    for (auto& cur : currencies) {
        if (cur.civ_id != civ_id) continue;
        if (cur.national_debt_millions < cur.money_supply_millions * 2.5) return;
        // Find largest creditor civ (for now use reserve currency holder)
        int creditor_id = reserve_currency_civ_id;
        // Haircut 30% of debt in exchange for stability concession
        cur.national_debt_millions *= 0.70;
        cur.is_in_default = false;
        cur.debt_restructuring_year = engine.year;
        // Creditor gains diplomatic leverage
        for (auto& civ : engine.civs) {
            if (civ.id == civ_id) {
                civ.stability -= 5.0f; // austerity pain
                if (creditor_id >= 0 && creditor_id < (int)engine.civs.size())
                    civ.trust_memory[creditor_id] -= 20.0f; // resents the terms
                engine.history.record(engine.year, engine.month, "ECONOMY",
                    civ.name + " debt restructuring",
                    "30% haircut negotiated. Austerity measures imposed.", civ_id);
                break;
            }
        }
        break;
    }
}

// ─── update_reserve_currency ──────────────────────────────────────────────────
void AeonCentralBankEngine::update_reserve_currency(AeonEngine& engine) {
    // The civ with the highest GDP holds reserve currency privilege
    float best_gdp = -1.0f;
    int   best_civ = -1;
    for (const auto& civ : engine.civs) {
        if (civ.is_alive > 0.0f && civ.economy.gdp > best_gdp) {
            best_gdp = civ.economy.gdp;
            best_civ = civ.id;
        }
    }
    if (best_civ < 0) return;

    // If reserve currency holder changes — losing civ gets inflation spike
    if (best_civ != reserve_currency_civ_id) {
        for (auto& cur : currencies) {
            if (cur.civ_id == reserve_currency_civ_id) {
                cur.inflation_rate += 5.0f;
                cur.is_reserve_currency = false;
                engine.history.record(engine.year, engine.month, "ECONOMY",
                    engine.civs[reserve_currency_civ_id].name + " loses reserve currency status",
                    "Inflation spikes as global demand for their currency falls.", reserve_currency_civ_id);
            }
            if (cur.civ_id == best_civ) {
                cur.is_reserve_currency = true;
            }
        }
        reserve_currency_civ_id = best_civ;
        for (auto& civ : engine.civs) {
            if (civ.id == best_civ) {
                civ.is_reserve_currency_holder = true;
            } else {
                civ.is_reserve_currency_holder = false;
            }
        }
    }
}

// ─── advance_business_cycle ───────────────────────────────────────────────────
void AeonCentralBankEngine::advance_business_cycle(AeonEngine& engine) {
    for (const auto& cur : currencies) {
        for (auto& civ : engine.civs) {
            if (civ.id != cur.civ_id || civ.is_alive <= 0.0f) continue;
            float& credit = civ.economy.credit_expansion;
            BusinessCycle& phase = civ.economy.business_cycle;
            // Advance cycle based on credit expansion level
            if (phase == BusinessCycle::BOOM) {
                credit += 0.05f;
                if (credit > 0.8f) { phase = BusinessCycle::STAGNATION; credit = 0.8f; }
            } else if (phase == BusinessCycle::STAGNATION) {
                if (cur.inflation_rate > 8.0f || civ.economy.asset_bubble_risk > 0.7f)
                    phase = BusinessCycle::RECESSION;
            } else if (phase == BusinessCycle::RECESSION) {
                credit -= 0.08f;
                civ.economy.gdp_growth -= 0.01f;
                civ.population.happiness -= 2.0f;
                if (credit < 0.1f) phase = BusinessCycle::DEPRESSION;
                if (credit < 0.0f) credit = 0.0f;
            } else if (phase == BusinessCycle::DEPRESSION) {
                credit -= 0.03f;
                civ.economy.gdp_growth -= 0.02f;
                civ.population.happiness -= 4.0f;
                civ.stability -= 2.0f;
                if (credit < -0.3f) credit = -0.3f;
                // Recovery eventually starts
                if (cur.interest_rate < 1.0f) phase = BusinessCycle::RECOVERY;
            } else if (phase == BusinessCycle::RECOVERY) {
                credit += 0.03f;
                civ.economy.gdp_growth += 0.005f;
                if (credit > 0.3f) phase = BusinessCycle::BOOM;
            }
            break;
        }
    }
}

} // namespace Aeon
