#pragma once
#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct PresidentCrisis {
    std::string title;
    std::string description;
    std::string advisor_defense;
    std::string advisor_economy;
    std::string advisor_opposition;
    std::vector<std::string> options;
    int year_triggered = 0;
    bool active = false;
};

enum class DecreeType {
    ECONOMIC_STIMULUS,
    EMERGENCY_DRAFT,
    RESEARCH_SUBSIDY,
    DIPLOMATIC_ENVOY,
    FOOD_RELIEF,
    BORDER_LOCKOUT,
    CUSTOM_DECREE
};

struct PresidentialRecord {
    int year = 0;
    std::string title;
    std::string summary;
    float approval_delta = 0.0f;
};

class AeonPresidentGame {
public:
    AeonPresidentGame() = default;

    void init();
    void tick_year(AeonEngine& engine);

    // Player State
    bool active = true;
    int player_civ_id = 0;
    std::string president_name = "President Alex Sterling";
    std::string administration_name = "Administration of Progress";

    // Presidential Stats (0..100%)
    float approval_rating = 65.0f;
    float coup_risk = 5.0f;
    int years_in_office = 0;
    int term_counter = 0; // 0..4 years
    int elections_won = 1;
    bool is_overthrown = false;
    bool election_loss = false;
    std::string last_news_headline = "President Alex Sterling takes office with high public expectations.";
    float treasury_gold = 50000.0f;

    // Fiscal & Policy Sliders
    float income_tax = 15.0f;     // % (0..50)
    float corporate_tax = 20.0f;  // % (0..50)
    float import_tariff = 10.0f;  // % (0..30)

    // Department Budget Allocations
    float budget_defense = 25.0f;
    float budget_healthcare = 25.0f;
    float budget_education = 20.0f;
    float budget_infrastructure = 15.0f;
    float budget_welfare = 15.0f;

    // History & Crisis
    std::vector<PresidentialRecord> decree_history;
    PresidentCrisis current_crisis;
    bool crisis_pending = false;

    // Methods
    void trigger_ollama_crisis(AeonEngine& engine);
    void resolve_crisis_option(AeonEngine& engine, int option_idx);
    void enact_decree(AeonEngine& engine, DecreeType decree, const std::string& custom_prompt = "");
    void declare_war(AeonEngine& engine, int target_civ_id);
    void trigger_election(AeonEngine& engine);

    std::string get_status_summary() const;
};

} // namespace Aeon
