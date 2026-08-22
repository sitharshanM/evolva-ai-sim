#pragma once
#include <string>

namespace Aeon {

struct NemotronStrategyResponse {
    std::string strategic_plan;
    std::string imperial_decree;
    std::string target_action;
    bool success = false;
};

class AeonNemotron {
public:
    static std::string api_key;
    static std::string model_name; // default "nvidia/llama-3.1-nemotron-70b-instruct"

    // Check if API key is configured (from memory, file, or environment)
    static bool is_configured();

    // Call NVIDIA NIM Nemotron API with custom prompt
    static std::string generate_content(const std::string& prompt);

    // Strategic brain for Kingdom Ruler AI powered by NVIDIA Nemotron
    static NemotronStrategyResponse plan_ruler_strategy(
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
