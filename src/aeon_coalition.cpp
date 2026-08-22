#include "aeon_coalition.h"
#include "aeon_engine.h"
#include <algorithm>
#include <sstream>
#include <random>

namespace Aeon {

int CoalitionEngine::create_coalition(const std::string& name, int founder_civ_id, int year) {
    Coalition c;
    c.id = next_coalition_id++;
    c.name = name;
    c.founder_civ_id = founder_civ_id;
    c.member_civ_ids.push_back(founder_civ_id);
    c.founded_year = year;
    coalitions.push_back(c);
    return c.id;
}

bool CoalitionEngine::join_coalition(int coalition_id, int civ_id) {
    for (auto& c : coalitions) {
        if (c.id == coalition_id) {
            if (std::find(c.member_civ_ids.begin(), c.member_civ_ids.end(), civ_id) == c.member_civ_ids.end()) {
                c.member_civ_ids.push_back(civ_id);
                return true;
            }
        }
    }
    return false;
}

bool CoalitionEngine::leave_coalition(int coalition_id, int civ_id) {
    for (auto& c : coalitions) {
        if (c.id == coalition_id) {
            auto it = std::remove(c.member_civ_ids.begin(), c.member_civ_ids.end(), civ_id);
            if (it != c.member_civ_ids.end()) {
                c.member_civ_ids.erase(it, c.member_civ_ids.end());
                return true;
            }
        }
    }
    return false;
}

bool CoalitionEngine::is_in_same_coalition(int civ_a, int civ_b) const {
    if (civ_a == civ_b) return true;
    for (const auto& c : coalitions) {
        bool has_a = std::find(c.member_civ_ids.begin(), c.member_civ_ids.end(), civ_a) != c.member_civ_ids.end();
        bool has_b = std::find(c.member_civ_ids.begin(), c.member_civ_ids.end(), civ_b) != c.member_civ_ids.end();
        if (has_a && has_b) return true;
    }
    return false;
}

EspionageResult CoalitionEngine::execute_espionage(int attacker_civ_id, int target_civ_id, EspionageMission mission, int current_year) {
    EspionageResult res;
    std::mt19937 rng(static_cast<unsigned int>(current_year * 1337 + attacker_civ_id * 101 + target_civ_id));
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    float chance = 0.55f;
    float roll = dist(rng);
    res.success = (roll < chance);

    std::ostringstream ss;
    if (res.success) {
        switch (mission) {
            case EspionageMission::INFILTRATE_CAPITAL:
                ss << "Agents successfully infiltrated target government network, gathering military maps.";
                res.stability_impact = -5.0f;
                break;
            case EspionageMission::STEAL_BLUEPRINT:
                ss << "Stole secret industrial schematics! Tech boost acquired.";
                res.tech_gained = 150.0f;
                break;
            case EspionageMission::SABOTAGE_INDUSTRY:
                ss << "Explosives detonated in manufacturing facilities! Infrastructure damaged.";
                res.stability_impact = -12.0f;
                break;
            case EspionageMission::INSTIGATE_REBELLION:
                ss << "Funded dissident factions, triggering severe civil unrest!";
                res.stability_impact = -25.0f;
                break;
            case EspionageMission::SATELLITE_HIJACK:
                ss << "Hacked target orbital defense grid! Telemetry compromised.";
                res.stability_impact = -18.0f;
                res.tech_gained = 80.0f;
                break;
        }
    } else {
        ss << "Mission FAILED! Operatives captured, diplomatic relations severely tarnished.";
        res.stability_impact = 0.0f;
    }
    res.report = ss.str();
    return res;
}

void CoalitionEngine::check_automatic_coalition_formation(AeonEngine& engine, int current_year) {
    // If an empire has >45% total strength or high aggression, others form defensive coalition
    int strongest_civ = -1;
    float max_power = 0.0f;
    float total_power = 0.0f;

    for (const auto& civ : engine.civs) {
        if (civ.is_alive <= 0.0f || civ.is_commons) continue;
        total_power += civ.military_power;
        if (civ.military_power > max_power) {
            max_power = civ.military_power;
            strongest_civ = civ.id;
        }
    }

    if (total_power > 0.0f && (max_power / total_power) > 0.40f && strongest_civ != -1) {
        // Check if anti-hegemony coalition already exists
        std::string coal_name = "Grand Anti-Hegemony League";
        bool exists = false;
        for (const auto& c : coalitions) {
            if (c.name == coal_name) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            int founder = -1;
            for (const auto& civ : engine.civs) {
                if (civ.is_alive > 0.0f && !civ.is_commons && civ.id != strongest_civ) {
                    founder = civ.id;
                    break;
                }
            }
            if (founder != -1) {
                int c_id = create_coalition(coal_name, founder, current_year);
                for (const auto& civ : engine.civs) {
                    if (civ.is_alive > 0.0f && !civ.is_commons && civ.id != strongest_civ && civ.id != founder) {
                        join_coalition(c_id, civ.id);
                    }
                }
                engine.history.record(current_year, 1, "DIPLOMACY",
                    "COALITION FORMED", "The " + coal_name + " was created to balance power!");
            }
        }
    }
}

} // namespace Aeon
