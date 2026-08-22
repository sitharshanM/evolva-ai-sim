#pragma once
#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct GeminiStrategyResponse {
    std::string strategic_plan;
    std::string imperial_decree;
    std::string target_action; // e.g. "DECLARE_WAR", "FORM_ALLIANCE", "INVEST_TECH"
    bool success = false;
};

class AeonGemini {
public:
    static std::string api_key;
    static std::string model_name; // default "gemini-2.5-flash"

    // Check if API key is set
    static bool is_configured();

    // Call Gemini 2.5 Flash API with custom prompt
    static std::string generate_content(const std::string& prompt);

    // High-level AI Ruler Strategic Brain powered by Gemini 2.5
    static GeminiStrategyResponse plan_ruler_strategy(
        const std::string& ruler_name,
        const std::string& civ_name,
        const std::string& personality,
        const std::string& world_context,
        float military_power,
        float treasury_gold,
        float gdp,
        bool at_war
    );
};

} // namespace Aeon
