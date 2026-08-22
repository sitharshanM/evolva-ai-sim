#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <optional>
#include <vector>
#include <glm/glm.hpp>
#include "stats.h"
#include "config.h"

// Decree parameters passed from OllamaGod → World (mirrors world.h DecreeParams)
struct DecreeParams {
    float     value1   = 0.0f;
    float     value2   = 0.0f;
    float     value3   = 0.0f;
    glm::vec2 pos      {-1.0f, -1.0f};
    float     radius   = 300.0f;
    float     duration = 15.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
//  OllamaGod  —  Background-thread LLM "World God" via Ollama REST API
//
//  Thread safety:
//    • request_*() can be called from the main (render) thread
//    • The worker thread calls Ollama HTTP and stores results
//    • poll_*() safely transfers results to the main thread
// ─────────────────────────────────────────────────────────────────────────────

#include "dynasty_leader.h"

struct PendingDecree {
    std::string    type;         // e.g. "food_surge"
    std::string    description;  // LLM's English sentence
    DecreeParams   params;

    // Leader info
    int         leader_faction_id = 0;
    std::string leader_name;
    std::string speech;

    // Optional political decree
    bool        has_political = false;
    std::string pol_action_type; // DECLARE_WAR, FORM_ALLIANCE, PEACE_TREATY, CIVIL_WAR
    int         faction_a = -1;
    int         faction_b = -1;
    std::string treaty_name;
    std::string declaration;
};

class OllamaGod {
public:
    OllamaGod();
    ~OllamaGod();

    void start(const std::string& model = Config::OLLAMA_MODEL);
    void stop();

    // Called from main thread after timer fires
    void request_decree(const Stats& stats, float sim_time, const LeaderPersonality& leader = LeaderPersonality());
    void request_emergency_decree(const Stats& stats, float sim_time, const LeaderPersonality& leader, const std::string& reason, const std::string& target_leader = "");
    void request_narrator(const std::string& event_description);
    void request_citizen_thought(uint64_t org_id, const std::string& citizen_name, const std::string& profession, const std::string& faction_name, const std::string& leader_name, const std::string& region_name, float health_pct, int kills);

    // Returns a ready decree (if one arrived since last call)
    std::optional<PendingDecree> poll_decree();
    // Returns a narrator line (if one arrived)
    std::optional<std::string> poll_narrator();
    // Returns a citizen thought (pair: org_id, thought_string)
    std::optional<std::pair<uint64_t, std::string>> poll_citizen_thought();

    bool is_busy()             const { return busy_.load(); }
    const std::string& status() const { return status_msg_; }

private:
    std::string model_;

    // ── Worker thread ─────────────────────────────────────────────────────────
    std::thread  worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> busy_   {false};

    // ── Request queue (main thread writes, worker reads) ──────────────────────
    std::mutex         req_mtx_;
    std::atomic<bool>  has_decree_req_   {false};
    std::atomic<bool>  has_emergency_req_{false};
    std::atomic<bool>  has_narrator_req_ {false};
    std::atomic<bool>  has_citizen_req_  {false};
    Stats              req_stats_;
    float              req_sim_time_ = 0.0f;
    LeaderPersonality  req_leader_;
    std::string        req_emergency_reason_;
    std::string        req_target_leader_;
    std::string        req_event_;

    uint64_t           req_citizen_org_id_ = 0;
    std::string        req_citizen_prompt_;

    // ── Result queue (worker writes, main thread reads) ───────────────────────
    std::mutex                   res_mtx_;
    std::optional<PendingDecree> result_decree_;
    std::optional<std::string>   result_narrator_;
    std::optional<std::pair<uint64_t, std::string>> result_citizen_thought_;

    std::string status_msg_ = "Idle";

    // ── Internal ──────────────────────────────────────────────────────────────
    void worker_loop();

    std::string http_post(const std::string& json_body, const std::string& model_override = "");
    std::string build_decree_prompt (const Stats& s, float sim_time, const LeaderPersonality& leader);
    std::string build_narrator_prompt(const std::string& event);

    bool parse_decree_response(const std::string& llm_text, PendingDecree& out);
};
