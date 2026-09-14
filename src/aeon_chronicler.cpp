#include "aeon_chronicler.h"
#include "aeon_engine.h"
#include "aeon_ollama.h"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace Aeon {

void AeonChronicler::update_eras(AeonEngine& engine, int current_year) {
    // 1. Count active wars
    int at_war_count = 0;
    for (const auto& c : engine.civs) {
        if (c.is_alive > 0.0f && c.at_war) {
            at_war_count++;
        }
    }

    // 2. Count active civil wars and identify crisis civs
    int active_civil_wars = 0;
    std::string crisis_civ_name;
    for (const auto& r : engine.rebellion_engine.rebellions) {
        if (r.active) {
            active_civil_wars++;
            if (r.parent_civ_id >= 0 && r.parent_civ_id < static_cast<int>(engine.civs.size())) {
                crisis_civ_name = engine.civs[r.parent_civ_id].name;
            }
        }
    }
    if (crisis_civ_name.empty()) {
        for (const auto& c : engine.civs) {
            if (c.is_alive > 0.0f && !c.is_commons && c.stability < 20.0f) {
                crisis_civ_name = c.name;
                break;
            }
        }
    }

    // 3. Check active truces and border claim revanchism
    int active_truces = 0;
    int claim_pairs = 0;
    for (const auto& c : engine.civs) {
        if (c.is_alive <= 0.0f) continue;
        for (const auto& t : c.signed_treaties) {
            if (t.year_signed + t.truce_duration_years >= current_year) {
                active_truces++;
            }
        }
        for (const auto& kv : c.bilateral_relations) {
            if (kv.second.border_claim_score > 35.0f || kv.second.hatred > 40.0f) {
                claim_pairs++;
            }
        }
    }

    // 4. Check Commons status
    bool commons_surging = false;
    for (const auto& c : engine.civs) {
        if (c.is_commons && c.is_alive > 0.0f) {
            if (c.territory_tiles > 35.0f || c.economy.gdp > 2500.0f || c.military_power > 1200.0f) {
                commons_surging = true;
            }
        }
    }

    // 5. Average stability
    float sum_stability = 0.0f;
    int alive_civ_count = 0;
    for (const auto& c : engine.civs) {
        if (c.is_alive > 0.0f && !c.is_commons) {
            sum_stability += c.stability;
            alive_civ_count++;
        }
    }
    float avg_stability = (alive_civ_count > 0) ? (sum_stability / alive_civ_count) : 50.0f;

    // Classification
    EraType detected_type = EraType::AGE_OF_PROSPERITY;
    std::string detected_title = "The Age of Prosperity";
    std::string detected_summary = "Trade flourishes, borders remain stable, and sovereign realms enjoy peace and economic growth.";

    if (active_civil_wars >= 1 || (!crisis_civ_name.empty() && avg_stability < 35.0f)) {
        detected_type = EraType::REGIONAL_CRISIS;
        detected_title = crisis_civ_name.empty() ? "The Crisis of Nations" : ("The " + crisis_civ_name + " Crisis");
        detected_summary = "Widespread civil instability and general rebellions shatter the foundations of sovereign states.";
    } else if (at_war_count >= 3) {
        detected_type = EraType::GREAT_CONTINENTAL_WAR;
        detected_title = "The Great Continental War";
        detected_summary = "Continental powers clash across burning frontlines in an era of destructive total war.";
    } else if (commons_surging) {
        detected_type = EraType::RISE_OF_THE_COMMONS;
        detected_title = "The Rise of The Commons";
        detected_summary = "Decentralized communes and the common populace expand their reach into a formidable continental force.";
    } else if (at_war_count == 0 && (active_truces >= 2 || claim_pairs >= 2)) {
        detected_type = EraType::FRACTURED_PEACE;
        detected_title = "The Fractured Peace";
        detected_summary = "Armies stand down under signed truces, but border revanchism and simmering animosities prepare the next storm.";
    } else if (avg_stability >= 65.0f && at_war_count == 0) {
        detected_type = EraType::AGE_OF_PROSPERITY;
        detected_title = "The Age of Prosperity";
        detected_summary = "Unprecedented peace, flourishing commerce, and shared stability reign across the continent.";
    }

    // Initialize or check transition
    if (!era_initialized_) {
        current_era_.type = detected_type;
        current_era_.title = detected_title;
        current_era_.start_year = current_year;
        current_era_.end_year = -1;
        current_era_.historical_summary = detected_summary;
        era_initialized_ = true;
    } else if (detected_type != current_era_.type && (current_year - current_era_.start_year >= 2)) {
        current_era_.end_year = current_year;
        recorded_eras_.push_back(current_era_);

        std::cout << "\n"
                  << "================================================================================\n"
                  << "📜 HISTORICAL EPOCH: ENTERING " << detected_title << " (Year " << current_year << ")\n"
                  << "--------------------------------------------------------------------------------\n"
                  << "  " << detected_summary << "\n"
                  << "================================================================================\n" << std::endl;

        engine.history.record(current_year, engine.month, "HISTORICAL_ERA",
            "Epoch Begins: " + detected_title,
            detected_summary, -1, -1, {"era_transition"}, 0.95f);

        current_era_.type = detected_type;
        current_era_.title = detected_title;
        current_era_.start_year = current_year;
        current_era_.end_year = -1;
        current_era_.historical_summary = detected_summary;
    }
}

std::string AeonChronicler::generate_aeon_daily(const AeonEngine& engine, int current_year) {
    std::ostringstream ss;
    ss << "\n📰 [AEON DAILY — YEAR " << current_year << "]\n";

    if (!current_era_.title.empty()) {
        ss << "  EPOCH: '" << current_era_.title << "' (Since Year " << current_era_.start_year << ")\n";
    }

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
       << "Active Era: " << current_era_.title << "\n\n"
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
        std::string fallback = "Chapter (Years " + std::to_string(from_year) + " - " + std::to_string(to_year) + "):\n"
                              + "Under " + current_era_.title + ", a period of great expansion, diplomacy, and conflict unfolded across the continent.";
        chapters_.push_back(fallback);
    }
}

std::string AeonChronicler::get_chronicle() const {
    std::ostringstream ss;
    ss << "\n========================================\n";
    ss << "       GRAND CHRONICLE OF AEON          \n";
    ss << "========================================\n\n";

    if (!recorded_eras_.empty() || !current_era_.title.empty()) {
        ss << "=== HISTORICAL ERAS OF RECORD ===\n";
        for (const auto& e : recorded_eras_) {
            ss << " • " << e.title << " (Years " << e.start_year << " - " << e.end_year << "): "
               << e.historical_summary << "\n";
        }
        if (!current_era_.title.empty()) {
            ss << " • " << current_era_.title << " (Year " << current_era_.start_year << " - Present): "
               << current_era_.historical_summary << "\n";
        }
        ss << "\n----------------------------------------\n\n";
    }

    if (chapters_.empty()) {
        ss << "  The Imperial Chronicle has no chapters recorded yet. Run simulation for 25+ years!\n";
    } else {
        for (const auto& ch : chapters_) {
            ss << ch << "\n\n----------------------------------------\n\n";
        }
    }
    return ss.str();
}

} // namespace Aeon
