#include "aeon_trade_caravan.h"
#include "aeon_engine.h"
#include <random>
#include <cmath>
#include <algorithm>

namespace Aeon {

void TradeCaravanEngine::init_map_resources(float world_w, float world_h, uint64_t seed) {
    (void)world_w; (void)world_h; (void)seed;
    nodes.clear();
    int id_counter = 1;

    struct RealDeposit { float x, y; StrategicResourceType type; float reserves; };
    RealDeposit deposits[] = {
        // Gold Veins
        { 208.0f, 116.0f, StrategicResourceType::GOLD_VEIN, 18000.0f }, // Witwatersrand South Africa
        { 63.0f,  51.0f,  StrategicResourceType::GOLD_VEIN, 14000.0f }, // Nevada Carlin Trend
        { 308.0f, 120.0f, StrategicResourceType::GOLD_VEIN, 15000.0f }, // Kalgoorlie Australia
        { 295.0f, 28.0f,  StrategicResourceType::GOLD_VEIN, 12000.0f }, // Kolyma Siberia
        // Iron Mines
        { 300.0f, 112.0f, StrategicResourceType::IRON_MINE, 25000.0f }, // Pilbara Australia
        { 128.0f, 96.0f,  StrategicResourceType::IRON_MINE, 22000.0f }, // Carajas Brazil
        { 198.0f, 23.0f,  StrategicResourceType::IRON_MINE, 18000.0f }, // Kiruna Sweden
        { 90.0f,  44.0f,  StrategicResourceType::IRON_MINE, 19000.0f }, // Mesabi Iron Range US
        { 298.0f, 49.0f,  StrategicResourceType::IRON_MINE, 20000.0f }, // Anshan China
        // Coal Deposits
        { 100.0f, 52.0f,  StrategicResourceType::COAL_DEPOSIT, 24000.0f }, // Appalachia US
        { 187.0f, 39.0f,  StrategicResourceType::COAL_DEPOSIT, 21000.0f }, // Ruhr Valley Germany
        { 292.0f, 53.0f,  StrategicResourceType::COAL_DEPOSIT, 26000.0f }, // Shanxi China
        { 268.0f, 35.0f,  StrategicResourceType::COAL_DEPOSIT, 22000.0f }, // Kuzbass Russia
        // Oil Wells
        { 230.0f, 64.0f,  StrategicResourceType::OIL_WELL, 35000.0f }, // Ghawar / Persian Gulf
        { 80.0f,  60.0f,  StrategicResourceType::OIL_WELL, 28000.0f }, // Texas Permian Basin
        { 183.0f, 35.0f,  StrategicResourceType::OIL_WELL, 20000.0f }, // North Sea
        { 258.0f, 30.0f,  StrategicResourceType::OIL_WELL, 30000.0f }, // Samotlor Siberia
        { 110.0f, 78.0f,  StrategicResourceType::OIL_WELL, 22000.0f }, // Orinoco Belt Venezuela
        // Rare Earths
        { 290.0f, 48.0f,  StrategicResourceType::RARE_EARTH_VEIN, 16000.0f }, // Bayan Obo China
        { 64.0f,  55.0f,  StrategicResourceType::RARE_EARTH_VEIN, 11000.0f }, // Mountain Pass California
        { 206.0f, 99.0f,  StrategicResourceType::RARE_EARTH_VEIN, 14000.0f }, // Katanga Congo
        // Uranium Nodes
        { 75.0f,  34.0f,  StrategicResourceType::URANIUM_NODE, 8000.0f }, // Athabasca Basin Canada
        { 248.0f, 46.0f,  StrategicResourceType::URANIUM_NODE, 9500.0f }, // Chu-Sarysu Kazakhstan
        { 315.0f, 105.0f, StrategicResourceType::URANIUM_NODE, 7500.0f }, // Ranger / McArthur Australia
    };

    for (const auto& d : deposits) {
        nodes.push_back({id_counter++, {d.x, d.y}, d.type, d.reserves, d.reserves, 0.0f, 1.0f, -1});
    }
}


void TradeCaravanEngine::spawn_caravan(int origin_civ, int dest_civ, glm::vec2 start, glm::vec2 dest, const std::string& cargo, float val, int tier) {
    if (is_embargoed(origin_civ, dest_civ)) return; // Blocked by trade embargo

    MapCaravan c;
    c.id = next_caravan_id++;
    c.origin_civ_id = origin_civ;
    c.dest_civ_id = dest_civ;
    c.pos = start;
    c.dest_pos = dest;
    c.cargo_type = cargo;
    c.cargo_value = val;
    c.cargo_tier = tier;
    c.speed = 30.0f;
    caravans.push_back(c);
}

void TradeCaravanEngine::update_caravans(float dt_years, AeonEngine& engine) {
    for (auto& c : caravans) {
        if (c.arrived || c.raided) continue;

        // Check if embargo was placed mid-transit
        if (is_embargoed(c.origin_civ_id, c.dest_civ_id)) {
            c.embargoed = true;
            continue;
        }

        glm::vec2 dir = c.dest_pos - c.pos;
        float dist = glm::length(dir);
        if (dist < 10.0f) {
            c.arrived = true;
            // Grant income to receiving civ, minus applicable tariffs
            float tariff_pct = get_tariff_rate(c.dest_civ_id, c.origin_civ_id);
            float tariff_tax = c.cargo_value * (tariff_pct / 100.0f);
            float net_value = c.cargo_value - tariff_tax;

            for (auto& civ : engine.civs) {
                if (civ.id == c.dest_civ_id) {
                    civ.resources.food += net_value * 0.3f;
                    civ.economy.annual_income += net_value;
                    civ.economy.annual_income += tariff_tax; // Government collects tariff
                    break;
                }
            }
            for (auto& civ : engine.civs) {
                if (civ.id == c.origin_civ_id) {
                    civ.economy.annual_income += net_value * 0.5f; // Merchant profits
                    break;
                }
            }
        } else {
            dir /= dist;
            c.pos += dir * c.speed * dt_years * 10.0f;
        }
    }

    // Clean up arrived / raided / embargoed caravans
    caravans.erase(std::remove_if(caravans.begin(), caravans.end(), [](const MapCaravan& c) {
        return c.arrived || c.raided || c.embargoed;
    }), caravans.end());
}

bool TradeCaravanEngine::raid_caravan(int caravan_id, int raider_civ_id, AeonEngine& engine) {
    for (auto& c : caravans) {
        if (c.id == caravan_id && !c.raided && !c.arrived) {
            c.raided = true;
            for (auto& civ : engine.civs) {
                if (civ.id == raider_civ_id) {
                    civ.resources.food += c.cargo_value;
                    civ.economy.annual_income += c.cargo_value * 1.5f;
                }
            }
            engine.history.record(engine.year, engine.month, "DIPLOMACY",
                "CARAVAN RAIDED", "Civ " + std::to_string(raider_civ_id) + " ambushed a merchant caravan!");
            return true;
        }
    }
    return false;
}

void TradeCaravanEngine::process_resource_nodes_tick(AeonEngine& engine) {
    for (auto& node : nodes) {
        if (node.remaining_reserve <= 0.0f) continue;
        if (node.controlling_civ_id < 0 || node.controlling_civ_id >= (int)engine.civs.size()) {
            // Assign to closest civ if unowned
            float min_d = 99999.0f;
            int best_civ = -1;
            for (const auto& civ : engine.civs) {
                if (civ.is_alive <= 0.0f) continue;
                float d = glm::distance(node.pos, glm::vec2((float)civ.capital_x, (float)civ.capital_y));
                if (d < min_d) { min_d = d; best_civ = civ.id; }
            }
            node.controlling_civ_id = best_civ;
        }
        if (node.controlling_civ_id < 0) continue;

        auto& civ = engine.civs[node.controlling_civ_id];
        float annual_yield = node.compute_hubbert_yield();
        annual_yield = std::min(annual_yield, node.remaining_reserve);
        node.remaining_reserve -= annual_yield;
        node.cumulative_extracted += annual_yield;

        switch (node.type) {
            case StrategicResourceType::GOLD_VEIN:
                civ.economy.annual_income += annual_yield * 1.5f;
                break;
            case StrategicResourceType::IRON_MINE:
                civ.resources.iron += annual_yield;
                break;
            case StrategicResourceType::COAL_DEPOSIT:
                civ.resources.coal += annual_yield;
                civ.resources.energy += annual_yield * 0.8f;
                civ.carbon_output += annual_yield * 0.02f;
                break;
            case StrategicResourceType::OIL_WELL:
                civ.resources.oil += annual_yield;
                civ.carbon_output += annual_yield * 0.05f;
                break;
            case StrategicResourceType::RARE_EARTH_VEIN:
                civ.resources.rare += annual_yield;
                break;
            case StrategicResourceType::URANIUM_NODE:
                civ.resources.energy += annual_yield * 2.0f;
                break;
        }
    }
}

void TradeCaravanEngine::process_manufacturing_chains_tick(AeonEngine& engine) {
    for (auto& civ : engine.civs) {
        if (civ.is_alive <= 0.0f) continue;
        auto& r = civ.resources;

        // Tier 2: Steel Processing (Iron + Coal -> Steel & Heavy Materials)
        float iron_used = std::min(r.iron, 30.0f);
        if (iron_used > 5.0f) {
            r.iron -= iron_used;
            civ.economy.gdp += iron_used * 20.0f;
            civ.military_power += iron_used * 0.15f;
        }

        // Tier 2: Oil Refining (Crude Oil -> Fuel & Petrochemicals)
        float oil_used = std::min(r.oil, 25.0f);
        if (oil_used > 5.0f) {
            r.oil -= oil_used;
            r.energy += oil_used * 1.2f;
            civ.economy.annual_income += oil_used * 15.0f;
        }

        // Tier 3: High-Tech Semiconductor & AI (Rare Earths + Silicon + Energy)
        float rare_used = std::min(r.rare, 15.0f);
        if (rare_used > 2.0f && r.energy > 10.0f) {
            r.rare -= rare_used;
            r.energy -= rare_used * 0.5f;
            civ.tech.research_pts += rare_used * 25.0f; // High tech boost
            civ.economy.gdp += rare_used * 50.0f;
        }
    }
}


void TradeCaravanEngine::set_tariff(int imposing_civ, int target_civ, float rate_pct, bool embargo) {
    for (auto& t : tariffs) {
        if (t.imposing_civ_id == imposing_civ && t.target_civ_id == target_civ) {
            t.tariff_rate_pct = rate_pct;
            t.total_embargo = embargo;
            return;
        }
    }
    tariffs.push_back({imposing_civ, target_civ, rate_pct, embargo});
}

bool TradeCaravanEngine::is_embargoed(int civ_a, int civ_b) const {
    for (const auto& t : tariffs) {
        if (((t.imposing_civ_id == civ_a && t.target_civ_id == civ_b) ||
             (t.imposing_civ_id == civ_b && t.target_civ_id == civ_a)) && t.total_embargo) {
            return true;
        }
    }
    return false;
}

float TradeCaravanEngine::get_tariff_rate(int imposing_civ, int target_civ) const {
    for (const auto& t : tariffs) {
        if (t.imposing_civ_id == imposing_civ && t.target_civ_id == target_civ) {
            return t.tariff_rate_pct;
        }
    }
    return 5.0f; // Default 5% standard tariff
}

} // namespace Aeon

