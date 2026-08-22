#pragma once
#include <string>
#include <vector>

namespace Aeon {

struct OllamaRequest {
    std::string model;     // e.g. "llama3.1"
    std::string prompt;
    int         max_tokens = 200;
    float       temperature = 0.75f;
};

struct LLMToolCall {
    std::string tool_name;
    std::string parameter;
};

struct DiplomaticSummitResult {
    std::string dialogue_transcript;
    std::string final_agreement; // PEACE_TREATY, NON_AGGRESSION_PACT, TRADE_DEAL, WAR_DECLARED
    bool agreed = false;
};

class AeonOllama {
public:
    // Send a prompt to Ollama and return the plain text response.
    // Returns empty string if Ollama is unreachable or times out.
    static std::string generate(const OllamaRequest& req);

    // Same as generate() but with no timeout — used from background threads.
    static std::string generate_blocking(const OllamaRequest& req);

    // Chat directly with an Empire Ruler in-character
    static std::string chat_with_ruler(const std::string& ruler_name, const std::string& civ_name, const std::string& ruler_personality, const std::string& user_message, const std::string& model = "llama3.1");

    // Hold a multi-turn LLM diplomatic summit between two AI rulers
    static DiplomaticSummitResult hold_summit(const std::string& ruler1_name, const std::string& civ1_name,
                                              const std::string& ruler2_name, const std::string& civ2_name,
                                              const std::string& topic, const std::string& model = "llama3.1");

    // Generate natural language state news / propaganda summary
    static std::string generate_propaganda_brief(const std::string& civ_name, const std::string& event_summary, const std::string& model = "llama3.1");

    // Process a user custom prompt into a structured decree
    static std::string generate_custom_decree(const std::string& custom_prompt, const std::string& world_context, const std::string& model = "llama3.1");

    // Tool Calling Protocol Protocol
    static std::string format_tool_prompt(const std::string& ruler_name, const std::string& state_context);
    static LLMToolCall parse_tool_call(const std::string& llm_raw_response);

    // Check if Ollama is reachable (quick GET to /api/tags)
    static bool is_available();
};

} // namespace Aeon
