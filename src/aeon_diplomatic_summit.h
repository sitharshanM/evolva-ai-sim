#pragma once
#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

enum class SummitVote {
    SUPPORT = 0,
    OPPOSE  = 1,
    ABSTAIN = 2
};

struct SummitSpeech {
    int civ_id = 0;
    std::string civ_name;
    std::string speaker_title;
    std::string speech;
    SummitVote vote = SummitVote::SUPPORT;
};

struct DiplomaticSummit {
    int id = 0;
    std::string topic;
    int year = 2026;
    bool active = false;
    int current_speaker_idx = 0;
    std::vector<SummitSpeech> speeches;
    int votes_support = 0;
    int votes_oppose = 0;
    int votes_abstain = 0;
    bool resolution_passed = false;
    std::string final_resolution_summary;
};

class AeonDiplomaticSummitEngine {
public:
    AeonDiplomaticSummitEngine() = default;

    void init();
    void convene_summit(AeonEngine& engine, const std::string& topic);
    void process_summit_step(AeonEngine& engine);
    void conclude_summit(AeonEngine& engine);

    DiplomaticSummit current_summit;
    std::vector<DiplomaticSummit> summit_history;
};

} // namespace Aeon
