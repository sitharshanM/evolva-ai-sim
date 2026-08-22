#include "aeon_diplomatic_summit.h"
#include "aeon_engine.h"
#include "aeon_ollama.h"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace Aeon {

void AeonDiplomaticSummitEngine::init() {
    current_summit = DiplomaticSummit();
    summit_history.clear();
}

void AeonDiplomaticSummitEngine::convene_summit(AeonEngine& engine, const std::string& topic) {
    current_summit = DiplomaticSummit();
    current_summit.id = static_cast<int>(summit_history.size()) + 1;
    current_summit.topic = topic;
    current_summit.year = engine.year;
    current_summit.active = true;
    current_summit.current_speaker_idx = 0;

    engine.history.record(engine.year, engine.month, "DIPLOMACY",
        "Global Peace & Crisis Summit Convened",
        "World leaders assemble to debate resolution: " + topic, 5);

    std::cout << "[SUMMIT] Convened: " << topic << std::endl;
}

void AeonDiplomaticSummitEngine::process_summit_step(AeonEngine& engine) {
    if (!current_summit.active) return;
    if (current_summit.current_speaker_idx >= 5) {
        conclude_summit(engine);
        return;
    }

    int cid = current_summit.current_speaker_idx;
    if (cid < 0 || cid >= (int)engine.civs.size()) {
        current_summit.current_speaker_idx++;
        return;
    }

    auto& civ = engine.civs[cid];
    if (civ.is_alive <= 0.0f) {
        current_summit.current_speaker_idx++;
        return;
    }

    SummitSpeech speech;
    speech.civ_id = civ.id;
    speech.civ_name = civ.name;
    speech.speaker_title = "Ruler of " + civ.name;

    // Determine vote and speech
    if (civ.diplomacy_pref > 0.6f) {
        speech.vote = SummitVote::SUPPORT;
        speech.speech = "We endorse this global resolution to ensure peace and open trade across all sovereign realms.";
        current_summit.votes_support++;
    } else if (civ.aggression > 0.65f) {
        speech.vote = SummitVote::OPPOSE;
        speech.speech = "This treaty compromises our sovereign imperial ambitions. We firmly vote AGAINST!";
        current_summit.votes_oppose++;
    } else {
        speech.vote = SummitVote::ABSTAIN;
        speech.speech = "We maintain a neutral posture on this matter to protect our state autonomy.";
        current_summit.votes_abstain++;
    }

    current_summit.speeches.push_back(speech);
    current_summit.current_speaker_idx++;

    if (current_summit.current_speaker_idx >= 5) {
        conclude_summit(engine);
    }
}

void AeonDiplomaticSummitEngine::conclude_summit(AeonEngine& engine) {
    if (!current_summit.active) return;
    current_summit.active = false;
    current_summit.resolution_passed = (current_summit.votes_support > current_summit.votes_oppose);

    std::ostringstream ss;
    if (current_summit.resolution_passed) {
        ss << "RESOLUTION PASSED (" << current_summit.votes_support << " Support vs " << current_summit.votes_oppose << " Oppose). Global stability increases!";
        for (auto& c : engine.civs) {
            c.stability = std::min(100.0f, c.stability + 5.0f);
        }
    } else {
        ss << "RESOLUTION REJECTED (" << current_summit.votes_oppose << " Oppose vs " << current_summit.votes_support << " Support). Geopolitical tensions remain high.";
    }

    current_summit.final_resolution_summary = ss.str();
    summit_history.push_back(current_summit);

    engine.history.record(engine.year, engine.month, "DIPLOMACY",
        "Global Summit Concluded",
        current_summit.final_resolution_summary, 5);

    std::cout << "[SUMMIT RESULT] " << current_summit.final_resolution_summary << std::endl;
}

} // namespace Aeon
