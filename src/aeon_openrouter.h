#pragma once
#include <string>

namespace Aeon {

struct OpenRouterStrategyResponse {
    std::string strategic_plan;
    std::string imperial_decree;
    std::string target_action;
    bool success = false;
};

class AeonOpenRouter {
public:
    static std::string api_key;
    static std::string drakor_api_key;
    static std::string eldoria_api_key;
    static std::string model_name; // default "mistralai/mistral-large-2411"

    // Check if API key is configured
    static bool is_configured();
    static std::string get_drakor_key();
    static std::string get_eldoria_key();

    // Call OpenRouter API with custom prompt and key/model overrides
    static std::string generate_content(const std::string& prompt);
    static std::string generate_content_custom(const std::string& prompt, const std::string& key = "", const std::string& target_model = "");

    // Strategic brain for Ruler AI powered by OpenRouter
    static OpenRouterStrategyResponse plan_ruler_strategy(
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
