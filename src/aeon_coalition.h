#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace Aeon {

enum class EspionageMission {
    INFILTRATE_CAPITAL,
    STEAL_BLUEPRINT,
    SABOTAGE_INDUSTRY,
    INSTIGATE_REBELLION,
    SATELLITE_HIJACK
};

inline const char* espionage_mission_name(EspionageMission m) {
    switch (m) {
        case EspionageMission::INFILTRATE_CAPITAL: return "Infiltrate Capital";
        case EspionageMission::STEAL_BLUEPRINT:    return "Steal Tech Blueprint";
        case EspionageMission::SABOTAGE_INDUSTRY:  return "Sabotage Industry";
        case EspionageMission::INSTIGATE_REBELLION:return "Instigate Rebellion";
        case EspionageMission::SATELLITE_HIJACK:   return "Orbital Satellite Hijack";
    }
    return "Unknown Mission";
}

struct Coalition {
    int id = 0;
    std::string name;
    int founder_civ_id = -1;
    std::vector<int> member_civ_ids;
    int founded_year = 0;
    float mutual_defense_strength = 1.0f; // Multiplier on defensive assistance
};

struct EspionageResult {
    bool success = false;
    std::string report;
    float stability_impact = 0.0f;
    float tech_gained = 0.0f;
};

class CoalitionEngine {
public:
    CoalitionEngine() = default;

    std::vector<Coalition> coalitions;
    int next_coalition_id = 1;

    int create_coalition(const std::string& name, int founder_civ_id, int year);
    bool join_coalition(int coalition_id, int civ_id);
    bool leave_coalition(int coalition_id, int civ_id);
    bool is_in_same_coalition(int civ_a, int civ_b) const;

    EspionageResult execute_espionage(int attacker_civ_id, int target_civ_id, EspionageMission mission, int current_year);
    void check_automatic_coalition_formation(class AeonEngine& engine, int current_year);
};

} // namespace Aeon
