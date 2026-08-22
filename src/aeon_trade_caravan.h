#pragma once
#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>

namespace Aeon {

enum class StrategicResourceType {
    GOLD_VEIN,
    IRON_MINE,
    COAL_DEPOSIT,
    OIL_WELL,
    RARE_EARTH_VEIN,
    URANIUM_NODE
};

inline const char* strategic_resource_name(StrategicResourceType t) {
    switch (t) {
        case StrategicResourceType::GOLD_VEIN:        return "Gold Deposit 💰";
        case StrategicResourceType::IRON_MINE:        return "Iron Vein ⚒️";
        case StrategicResourceType::COAL_DEPOSIT:     return "Coal Seam ⬛";
        case StrategicResourceType::OIL_WELL:         return "Oil Field 🛢️";
        case StrategicResourceType::RARE_EARTH_VEIN:  return "Rare Earth Lode 💎";
        case StrategicResourceType::URANIUM_NODE:     return "Uranium Node ☢️";
    }
    return "Resource";
}

struct StrategicNode {
    int id = 0;
    glm::vec2 pos{0.0f, 0.0f};
    StrategicResourceType type = StrategicResourceType::GOLD_VEIN;
    float initial_reserve = 10000.0f;
    float remaining_reserve = 10000.0f;
    float cumulative_extracted = 0.0f;
    float extraction_efficiency = 1.0f;
    int controlling_civ_id = -1;

    // Hubbert Peak Bell Curve: yield peaks at 50% cumulative extraction
    float compute_hubbert_yield() const {
        if (initial_reserve <= 0.001f || remaining_reserve <= 0.0f) return 0.0f;
        float fraction_extracted = cumulative_extracted / initial_reserve;
        // Bell shape: 4 * f * (1 - f) peaks at 1.0 when f = 0.5
        float hubbert_factor = 4.0f * fraction_extracted * (1.0f - fraction_extracted);
        hubbert_factor = std::max(0.05f, hubbert_factor);
        return extraction_efficiency * hubbert_factor * 100.0f;
    }
};

struct MapCaravan {
    int id = 0;
    int origin_civ_id = -1;
    int dest_civ_id = -1;
    glm::vec2 pos{0.0f, 0.0f};
    glm::vec2 dest_pos{0.0f, 0.0f};
    float speed = 25.0f;
    float cargo_value = 150.0f;
    std::string cargo_type = "Gold & Spices";
    int cargo_tier = 1; // 1 = Raw, 2 = Processed (Steel/Fuel), 3 = High-Tech
    bool arrived = false;
    bool raided = false;
    bool embargoed = false;
};

struct TradeTariff {
    int imposing_civ_id = -1;
    int target_civ_id = -1;
    float tariff_rate_pct = 15.0f; // 15% tariff
    bool total_embargo = false;
};

class TradeCaravanEngine {
public:
    TradeCaravanEngine() = default;

    std::vector<StrategicNode> nodes;
    std::vector<MapCaravan> caravans;
    std::vector<TradeTariff> tariffs;
    int next_caravan_id = 1;

    void init_map_resources(float world_w, float world_h, uint64_t seed);
    void spawn_caravan(int origin_civ, int dest_civ, glm::vec2 start, glm::vec2 dest, const std::string& cargo, float val, int tier = 1);
    void update_caravans(float dt_years, class AeonEngine& engine);
    
    bool raid_caravan(int caravan_id, int raider_civ_id, class AeonEngine& engine);
    void process_resource_nodes_tick(class AeonEngine& engine);
    void process_manufacturing_chains_tick(class AeonEngine& engine);
    void set_tariff(int imposing_civ, int target_civ, float rate_pct, bool embargo);
    bool is_embargoed(int civ_a, int civ_b) const;
    float get_tariff_rate(int imposing_civ, int target_civ) const;
};

} // namespace Aeon

