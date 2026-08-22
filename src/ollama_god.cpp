#include "ollama_god.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <chrono>

// ── Windows HTTP (WinHTTP) ────────────────────────────────────────────────────
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winhttp.h>

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
OllamaGod::OllamaGod()  = default;
OllamaGod::~OllamaGod() { stop(); }

void OllamaGod::start(const std::string& model) {
    model_   = model;
    running_ = true;
    worker_  = std::thread(&OllamaGod::worker_loop, this);
}

void OllamaGod::stop() {
    running_ = false;
    if (worker_.joinable()) worker_.join();
}

// ── Main-thread API ───────────────────────────────────────────────────────────
void OllamaGod::request_decree(const Stats& stats, float sim_time, const LeaderPersonality& leader) {
    if (busy_) return;
    std::lock_guard<std::mutex> lock(req_mtx_);
    req_stats_    = stats;
    req_sim_time_ = sim_time;
    req_leader_   = leader;
    has_decree_req_.store(true);
}

void OllamaGod::request_emergency_decree(const Stats& stats, float sim_time, const LeaderPersonality& leader, const std::string& reason, const std::string& target_leader) {
    std::lock_guard<std::mutex> lock(req_mtx_);
    req_stats_            = stats;
    req_sim_time_         = sim_time;
    req_leader_           = leader;
    req_emergency_reason_ = reason;
    req_target_leader_    = target_leader;
    has_emergency_req_.store(true);
}

void OllamaGod::request_narrator(const std::string& event) {
    if (busy_) return;
    std::lock_guard<std::mutex> lock(req_mtx_);
    req_event_ = event;
    has_narrator_req_.store(true);
}

void OllamaGod::request_citizen_thought(uint64_t org_id, const std::string& citizen_name, const std::string& profession, const std::string& faction_name, const std::string& leader_name, const std::string& region_name, float health_pct, int kills) {
    if (busy_) return;
    std::lock_guard<std::mutex> lock(req_mtx_);
    req_citizen_org_id_ = org_id;
    std::ostringstream ss;
    ss << "You are " << citizen_name << ", a " << profession << " of the " << faction_name << " led by " << leader_name << ".\n";
    ss << "You live in " << region_name << ". Your health is at " << (int)(health_pct * 100) << "% and you have slain " << kills << " enemy invaders.\n";
    ss << "In ONE short sentence (max 100 chars), what are your inner thoughts right now?\n";
    ss << "Respond with ONLY your inner thought sentence.";
    req_citizen_prompt_ = ss.str();
    has_citizen_req_.store(true);
}

std::optional<PendingDecree> OllamaGod::poll_decree() {
    std::lock_guard<std::mutex> lock(res_mtx_);
    auto result = std::move(result_decree_);
    result_decree_.reset();
    return result;
}

std::optional<std::string> OllamaGod::poll_narrator() {
    std::lock_guard<std::mutex> lock(res_mtx_);
    auto result = std::move(result_narrator_);
    result_narrator_.reset();
    return result;
}

std::optional<std::pair<uint64_t, std::string>> OllamaGod::poll_citizen_thought() {
    std::lock_guard<std::mutex> lock(res_mtx_);
    auto result = std::move(result_citizen_thought_);
    result_citizen_thought_.reset();
    return result;
}

// ── Worker thread loop ────────────────────────────────────────────────────────
void OllamaGod::worker_loop() {
    while (running_) {
        bool handle_emergency = has_emergency_req_.exchange(false);
        bool handle_decree    = has_decree_req_.exchange(false);
        bool handle_narrator  = has_narrator_req_.exchange(false);
        bool handle_citizen   = has_citizen_req_.exchange(false);

        if (handle_emergency || handle_decree) {
            busy_ = true;

            Stats stats_copy;
            float sim_time_copy;
            LeaderPersonality leader_copy;
            std::string emergency_reason, target_leader;
            {
                std::lock_guard<std::mutex> lock(req_mtx_);
                stats_copy       = req_stats_;
                sim_time_copy    = req_sim_time_;
                leader_copy      = req_leader_;
                emergency_reason = req_emergency_reason_;
                target_leader    = req_target_leader_;
            }

            status_msg_ = handle_emergency
                ? "🚨 EMERGENCY: " + leader_copy.name + " reacting to " + emergency_reason
                : "Consulting " + leader_copy.name + " (" + leader_copy.model + ")...";

            std::string prompt   = build_decree_prompt(stats_copy, sim_time_copy, leader_copy);
            if (handle_emergency) {
                prompt += "\nEMERGENCY TRIGGER EVENT: " + emergency_reason;
                if (!target_leader.empty()) prompt += " (Rival Leader involved: " + target_leader + ")";
                prompt += "\nReact spontaneously with an aggressive/defensive counter-decree and fierce speech!";
            }

            std::string response = http_post(prompt, leader_copy.model);

            if (!response.empty()) {
                PendingDecree decree;
                if (parse_decree_response(response, decree)) {
                    decree.leader_faction_id = leader_copy.faction_id;
                    decree.leader_name       = leader_copy.name;
                    std::lock_guard<std::mutex> lock(res_mtx_);
                    result_decree_ = decree;
                    status_msg_ = leader_copy.name + " issued turn: " + decree.type;
                } else {
                    status_msg_ = "Bad LLM response from " + leader_copy.name;
                }
            } else {
                status_msg_ = "Ollama unreachable (" + leader_copy.model + ")";
            }
            busy_ = false;
        }
        else if (handle_citizen) {
            busy_ = true;
            status_msg_ = "Reading Citizen Mind...";

            uint64_t org_id;
            std::string prompt;
            {
                std::lock_guard<std::mutex> lock(req_mtx_);
                org_id = req_citizen_org_id_;
                prompt = req_citizen_prompt_;
            }

            std::string response = http_post(prompt);
            if (!response.empty()) {
                std::lock_guard<std::mutex> lock(res_mtx_);
                result_citizen_thought_ = std::make_pair(org_id, response);
                status_msg_ = "Citizen Thought Read";
            }
            busy_ = false;
        }
        else if (handle_narrator) {
            busy_ = true;
            status_msg_ = "Narrating...";

            std::string event_copy;
            { std::lock_guard<std::mutex> lock(req_mtx_); event_copy = req_event_; }

            std::string prompt   = build_narrator_prompt(event_copy);
            std::string response = http_post(prompt);

            if (!response.empty()) {
                std::lock_guard<std::mutex> lock(res_mtx_);
                result_narrator_ = response;
            }
            busy_ = false;
        }

        // Sleep briefly to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// ── HTTP POST via WinHTTP ─────────────────────────────────────────────────────
std::string OllamaGod::http_post(const std::string& user_prompt, const std::string& model_override) {
    // Build Ollama request JSON
    json req;
    req["model"]       = model_override.empty() ? model_ : model_override;
    req["prompt"]      = user_prompt;
    req["stream"]      = false;
    req["temperature"] = 0.70;
    req["options"]["num_predict"] = 256;

    std::string body = req.dump();

    // Wide-string host
    int wlen = MultiByteToWideChar(CP_UTF8, 0, Config::OLLAMA_HOST, -1, nullptr, 0);
    std::wstring whost(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, Config::OLLAMA_HOST, -1, whost.data(), wlen);

    HINTERNET hSession = WinHttpOpen(
        L"DigitalLife/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession) return {};

    HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(),
                                         (INTERNET_PORT)Config::OLLAMA_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return {}; }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"POST", L"/api/generate",
        nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return {};
    }

    // Set a reasonable timeout (30 seconds)
    DWORD timeout = 30000;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SEND_TIMEOUT,    &timeout, sizeof(timeout));

    std::wstring headers = L"Content-Type: application/json\r\n";
    BOOL ok = WinHttpSendRequest(
        hRequest,
        headers.c_str(), (DWORD)-1L,
        (LPVOID)body.c_str(), (DWORD)body.size(),
        (DWORD)body.size(), 0);

    if (!ok || !WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return {};
    }

    // Read the response in chunks
    std::string raw_response;
    DWORD bytes_available = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytes_available) && bytes_available > 0) {
        std::string buf(bytes_available, '\0');
        DWORD bytes_read = 0;
        WinHttpReadData(hRequest, buf.data(), bytes_available, &bytes_read);
        raw_response.append(buf.data(), bytes_read);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    // Parse Ollama JSON envelope  → extract "response" field
    try {
        auto j = json::parse(raw_response);
        if (j.contains("response"))
            return j["response"].get<std::string>();
    } catch (...) {}
    return raw_response; // fallback: return raw
}

// ── Prompt builders ───────────────────────────────────────────────────────────
std::string OllamaGod::build_decree_prompt(const Stats& s, float sim_time, const LeaderPersonality& leader) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << leader.system_prompt << "\n\n";
    ss << "Current world state after " << sim_time << " seconds:\n";
    ss << "- Total Population: " << s.population << " organisms across " << s.lineage_count << " nations (Factions 0 to 5).\n";
    ss << "- Herbivores: " << (int)(s.herbivore_ratio * 100) << "%, Carnivores: " << (int)(s.carnivore_ratio * 100) << "%\n";
    ss << "- Avg speed: " << s.avg_speed << ", Avg aggression: " << s.avg_aggression << "\n";
    ss << "- Leader Age: " << (int)leader.ruler_age << " years old | Heir Apparent: " << leader.heir_name << "\n";
    if (!leader.relational_memory.empty()) {
        ss << "- Recent Grievances & Rivalry Memory:\n";
        for (const auto& g : leader.relational_memory) {
            ss << "   * " << g << "\n";
        }
    }
    ss << "\n";
    ss << "Formulate your strategic leader turn. Provide a dramatic speech/taunt, an environmental decree, AND a political action.\n";
    ss << "Respond ONLY with valid JSON (no markdown, no surrounding text):\n";
    ss << R"({
  "speech": "Our empire will reign supreme!",
  "decree": "food_surge",
  "description": "Blessings upon our fertile lands.",
  "value1": 20.0, "radius": 300.0, "duration": 15.0,
  "political_action": {
    "type": "DECLARE_WAR",
    "faction_a": )" << leader.faction_id << R"(,
    "faction_b": 1,
    "treaty_name": "War of Conquest",
    "declaration": ")" << leader.name << R"( declares war on rival lands!"
  }
})";
    ss << "\n\nAvailable environmental decrees: food_surge, food_famine, plague, population_cull, genetic_drift_boost, predator_wave, resource_cluster, temperature_shift, storm.\n";
    ss << "Available political, economic & civilization action types: DECLARE_WAR, FORM_ALLIANCE, PEACE_TREATY, CIVIL_WAR, TAX_HARVEST, BUILD_FORTRESS, EXPAND_BORDERS, SUBSIDIZE_GROWTH, HIRE_MERCENARIES, ADOPT_GOVERNMENT, UPGRADE_SETTLEMENT, ESTABLISH_ROAD, GRANT_ASYLUM, NONE.\n";
    return ss.str();
}

std::string OllamaGod::build_narrator_prompt(const std::string& event) {
    return "You are a dramatic narrator for a digital civilization simulation. "
           "In ONE epic sentence (max 120 chars), narrate this political/world event: " + event +
           "\nRespond with ONLY the sentence, no quotes.";
}

// ── Response parser ───────────────────────────────────────────────────────────
bool OllamaGod::parse_decree_response(const std::string& text, PendingDecree& out) {
    auto start = text.find('{');
    auto end   = text.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end <= start)
        return false;

    std::string json_str = text.substr(start, end - start + 1);
    try {
        auto j = json::parse(json_str);
        out.type        = j.value("decree",      "food_surge");
        out.description = j.value("description", "The gods intervene.");
        out.speech      = j.value("speech",      "");
        out.params.value1   = j.value("value1",   0.0f);
        out.params.value2   = j.value("value2",   0.0f);
        out.params.radius   = j.value("radius",   300.0f);
        out.params.duration = j.value("duration", 15.0f);
        out.params.pos      = {-1.0f, -1.0f};

        if (j.contains("political_action") && j["political_action"].is_object()) {
            auto pol = j["political_action"];
            std::string ptype = pol.value("type", "NONE");
            if (ptype != "NONE" && !ptype.empty()) {
                out.has_political   = true;
                out.pol_action_type = ptype;
                out.faction_a       = pol.value("faction_a", 0);
                out.faction_b       = pol.value("faction_b", 1);
                out.treaty_name     = pol.value("treaty_name", "Treaty");
                out.declaration     = pol.value("declaration", "War declared!");
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}
