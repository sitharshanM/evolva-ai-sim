#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "config.h"

enum class GovernmentForm {
    DEMOCRACY, MONARCHY, DICTATORSHIP, REPUBLIC, THEOCRACY, TECHNOCRACY, AI_OVERLORD
};

inline const char* government_form_str(GovernmentForm g) {
    switch (g) {
        case GovernmentForm::DEMOCRACY:   return "Democracy 🗳️";
        case GovernmentForm::MONARCHY:    return "Absolute Monarchy 👑";
        case GovernmentForm::DICTATORSHIP:return "Military Dictatorship ⚔️";
        case GovernmentForm::REPUBLIC:   return "Merchant Republic 🏛️";
        case GovernmentForm::THEOCRACY:   return "Divine Theocracy 🔮";
        case GovernmentForm::TECHNOCRACY: return "Technocracy 🔬";
        case GovernmentForm::AI_OVERLORD: return "AI Overlord Matrix 🤖";
    }
    return "Monarchy";
}

struct CityProfile {
    std::string name       = "New Horizon";
    glm::vec2   pos        {0.0f, 0.0f};
    int         population = 250000;
    std::string tier       = "City"; // Village -> Town -> City -> Industrial City -> Megacity
    float       happiness  = 85.0f;  // 0 to 100
    float       crime_rate = 5.0f;   // 0 to 100
    float       jobs_fill  = 92.0f;  // 0 to 100
};

struct PoliticalFaction {
    std::string name        = "Merchants Guild";
    float       satisfaction = 75.0f; // 0 to 100
    float       power        = 25.0f; // % influence
    std::string primary_demand = "Lower Corporate Taxes";
};

struct CyberWarfareSuite {
    float firewall_strength   = 80.0f;
    float power_grid_defense  = 90.0f;
    float cyber_attack_power  = 50.0f;
    int   cyber_incidents     = 0;
};

class KingdomDossier {
public:
    int         id             = 0;
    std::string name           = "Veyra Empire";
    std::string culture        = "Scientific Collectivism";
    std::string religion       = "Rationalist Cult";
    GovernmentForm government  = GovernmentForm::TECHNOCRACY;

    float stability_pct        = 82.0f;
    float tech_level           = 74.0f;
    float military_rating      = 61.0f;
    float public_approval      = 78.0f;

    std::vector<CityProfile>      cities;
    std::vector<PoliticalFaction> factions;
    CyberWarfareSuite            cyber;

    std::string media_headline = "Empire reports record economic growth.";
};
