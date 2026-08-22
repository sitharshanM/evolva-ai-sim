#include "aeon_chronicler.h"
#include "aeon_engine.h"
#include "aeon_ollama.h"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace Aeon {

std::string AeonChronicler::generate_aeon_daily(const AeonEngine& engine, int current_year) {
    std::ostringstream ss;
    ss << "\n📰 [AEON DAILY — YEAR " << current_year << "]\n";

    // 1. MILITARY HEGEMON
    int top_mil_id = -1;
    float max_mil = -1.0f;
    for (const auto& civ : engine.civs) {
        if (civ.is_alive > 0.0f && civ.military_power > max_mil) {
            max_mil = civ.military_power;
            top_mil_id = civ.id;
        }
    }
    if (top_mil_id >= 0) {
        ss << "  MILITARY: '" << engine.civs[top_mil_id].name
           << " maintains continental military superiority with " << int(max_mil) << " power.'\n";
    }

    // 2. ECONOMY LEADER
    int top_econ_id = -1;
    float max_gdp = -1.0f;
    for (const auto& civ : engine.civs) {
        if (civ.is_alive > 0.0f && civ.economy.gdp > max_gdp) {
            max_gdp = civ.economy.gdp;
            top_econ_id = civ.id;
        }
    }
    if (top_econ_id >= 0) {
        ss << "  ECONOMY: '" << engine.civs[top_econ_id].name
           << " powers global trade, holding the world's largest economy ($" << int(max_gdp) << " GDP).'\n";
    }

    // 3. POLITICS & RULER
    if (!engine.civs.empty()) {
        int sample_civ_idx = (current_year % (int)engine.civs.size());
        const auto& c = engine.civs[sample_civ_idx];
        if (c.is_alive > 0.0f && c.ruler_id >= 0) {
            for (const auto& ch : engine.characters) {
                if (ch.id == c.ruler_id && ch.is_alive) {
                    ss << "  POLITICS: '" << c.name << " governed under "
                       << ch.name << " (" << gov_form_name(c.government) << ").'\n";
                    break;
                }
            }
        }
    }

    // 4. CRISIS OR WAR
    bool found_crisis = false;
    for (const auto& civ : engine.civs) {
        if (civ.is_alive > 0.0f && civ.at_war && civ.war_with_civ >= 0 && civ.war_with_civ < (int)engine.civs.size()) {
            ss << "  CRISIS: 'Frontlines ablaze as " << civ.name << " battles "
               << engine.civs[civ.war_with_civ].name << "!'\n";
            found_crisis = true;
            break;
        } else if (civ.is_alive > 0.0f && civ.stability < 30.0f) {
            ss << "  CRISIS: '" << civ.name << " struggles with severe civil unrest and factional instability.'\n";
            found_crisis = true;
            break;
        }
    }
    if (!found_crisis) {
        ss << "  CRISIS: 'Continental borders remain stable under current peace accords.'\n";
    }

    // 5. DIPLOMACY & TRADE
    if (!engine.history.all().empty()) {
        const auto& ev = engine.history.all().back();
        ss << "  DIPLOMACY: '" << ev.headline << ".'\n";
    }

    return ss.str();
}

void AeonChronicler::update_chronicle(const AeonHistory& history, int current_year) {
    if (current_year - last_summarized_year_ < 25) return;

    int from_year = last_summarized_year_ + 1;
    int to_year   = current_year;
    last_summarized_year_ = current_year;

    std::string timeline = history.timeline_summary(from_year, to_year);
    if (timeline.empty()) return;

    std::ostringstream ss;
    ss << "You are the Grand Imperial Chronicler of the world AEON.\n"
       << "Write a dramatic, engaging historical narrative chapter covering Years "
       << from_year << " to " << to_year << " based on these raw timeline events:\n\n"
       << timeline << "\n\n"
       << "Write a 3-paragraph historical chronicle chapter titled 'Chapter: Years "
       << from_year << " - " << to_year << "'. Be vivid and epic.";

    OllamaRequest req;
    req.model       = "llama3.1";
    req.prompt      = ss.str();
    req.max_tokens  = 350;
    req.temperature = 0.80f;

    std::string chapter = AeonOllama::generate(req);

    if (!chapter.empty()) {
        chapters_.push_back(chapter);
        std::cout << "\n[CHRONICLER] New chapter written: Years "
                  << from_year << " - " << to_year << "!" << std::endl;
    } else {
        // Fallback narrative if Ollama is offline
        std::string fallback = "Chapter (Years " + std::to_string(from_year) + " - " + std::to_string(to_year) + "):\n"
                              + "A period of great expansion, diplomacy, and conflict unfolded across the continent.";
        chapters_.push_back(fallback);
    }
}

std::string AeonChronicler::get_chronicle() const {
    if (chapters_.empty()) {
        return "  The Imperial Chronicle has no chapters recorded yet. Run simulation for 25+ years!\n";
    }
    std::ostringstream ss;
    ss << "\n========================================\n";
    ss << "       GRAND CHRONICLE OF AEON          \n";
    ss << "========================================\n\n";
    for (const auto& ch : chapters_) {
        ss << ch << "\n\n----------------------------------------\n\n";
    }
    return ss.str();
}

} // namespace Aeon
