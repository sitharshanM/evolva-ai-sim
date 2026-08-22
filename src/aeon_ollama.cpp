#include "aeon_ollama.h"
#include "aeon_gemini.h"
#include <string>
#include <sstream>
#include <iostream>
#include <chrono>


#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winhttp.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

static const char*  AEON_OLLAMA_HOST = "127.0.0.1";
static const int    AEON_OLLAMA_PORT = 11434;

namespace Aeon {

// ─── is_available ─────────────────────────────────────────────────────────────
bool AeonOllama::is_available() {
    static bool cached_result = false;
    static auto last_check = std::chrono::steady_clock::now() - std::chrono::seconds(10);

    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_check).count() < 3) {
        return cached_result;
    }
    last_check = now;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, AEON_OLLAMA_HOST, -1, nullptr, 0);
    std::wstring whost(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, AEON_OLLAMA_HOST, -1, whost.data(), wlen);

    HINTERNET hSession = WinHttpOpen(L"AeonSim/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { cached_result = false; return false; }

    HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(),
                                        (INTERNET_PORT)AEON_OLLAMA_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); cached_result = false; return false; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", L"/api/tags",
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

    DWORD t = 50; // 50ms ultra fast check to prevent GUI frame drops
    WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &t, sizeof(t));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SEND_TIMEOUT,    &t, sizeof(t));

    bool ok = WinHttpSendRequest(hRequest, nullptr, 0, nullptr, 0, 0, 0)
           && WinHttpReceiveResponse(hRequest, nullptr);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    cached_result = ok;
    return cached_result;
}

// ─── generate ─────────────────────────────────────────────────────────────────
std::string AeonOllama::generate(const OllamaRequest& req) {
    if (!is_available()) return {};

    json body;
    body["model"]       = req.model;
    body["prompt"]      = req.prompt;
    body["stream"]      = false;
    body["temperature"] = req.temperature;
    body["options"]["num_predict"] = req.max_tokens;
    std::string body_str = body.dump();

    int wlen = MultiByteToWideChar(CP_UTF8, 0, AEON_OLLAMA_HOST, -1, nullptr, 0);
    std::wstring whost(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, AEON_OLLAMA_HOST, -1, whost.data(), wlen);

    HINTERNET hSession = WinHttpOpen(
        L"AeonSim/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return {};

    HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(),
                                        (INTERNET_PORT)AEON_OLLAMA_PORT, 0);
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

    DWORD timeout_ms = 250;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout_ms, sizeof(timeout_ms));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SEND_TIMEOUT,    &timeout_ms, sizeof(timeout_ms));

    std::wstring headers = L"Content-Type: application/json\r\n";
    BOOL ok = WinHttpSendRequest(
        hRequest,
        headers.c_str(), (DWORD)-1L,
        (LPVOID)body_str.c_str(), (DWORD)body_str.size(),
        (DWORD)body_str.size(), 0);

    if (!ok || !WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return {};
    }

    std::string raw;
    DWORD avail = 0;
    while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0) {
        std::string buf(avail, '\0');
        DWORD read = 0;
        WinHttpReadData(hRequest, buf.data(), avail, &read);
        raw.append(buf.data(), read);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    try {
        auto j = json::parse(raw);
        if (j.contains("response"))
            return j["response"].get<std::string>();
    } catch (...) {}
    return raw;
}

// ─── generate_blocking (no timeout — for background thread use) ───────────────
std::string AeonOllama::generate_blocking(const OllamaRequest& req) {
    json body;
    body["model"]       = req.model;
    body["prompt"]      = req.prompt;
    body["stream"]      = false;
    body["temperature"] = req.temperature;
    body["options"]["num_predict"] = req.max_tokens;
    std::string body_str = body.dump();

    int wlen = MultiByteToWideChar(CP_UTF8, 0, AEON_OLLAMA_HOST, -1, nullptr, 0);
    std::wstring whost(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, AEON_OLLAMA_HOST, -1, whost.data(), wlen);

    HINTERNET hSession = WinHttpOpen(
        L"AeonSim/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return {};

    HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(),
                                        (INTERNET_PORT)AEON_OLLAMA_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return {}; }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"POST", L"/api/generate",
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return {};
    }

    std::wstring headers = L"Content-Type: application/json\r\n";
    BOOL ok = WinHttpSendRequest(
        hRequest, headers.c_str(), (DWORD)-1L,
        (LPVOID)body_str.c_str(), (DWORD)body_str.size(),
        (DWORD)body_str.size(), 0);

    if (!ok || !WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        if (AeonGemini::is_configured()) return AeonGemini::generate_content(req.prompt);
        return {};
    }

    std::string raw;
    DWORD avail = 0;
    while (WinHttpQueryDataAvailable(hRequest, &avail) && avail > 0) {
        std::string buf(avail, '\0');
        DWORD read = 0;
        WinHttpReadData(hRequest, buf.data(), avail, &read);
        raw.append(buf.data(), read);
    }
    WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    std::string result = "";
    try {
        auto j = json::parse(raw);
        if (j.contains("response"))
            result = j["response"].get<std::string>();
    } catch (...) {}

    if (result.empty() && !raw.empty()) {
        result = raw;
    }

    if (result.empty() && AeonGemini::is_configured()) {
        return AeonGemini::generate_content(req.prompt);
    }
    return result;
}


std::string AeonOllama::chat_with_ruler(const std::string& ruler_name, const std::string& civ_name, const std::string& ruler_personality, const std::string& user_message, const std::string& model) {
    std::ostringstream ss;
    ss << "System: You are " << ruler_name << ", ruler of " << civ_name << ". Personality: " << ruler_personality << ". Respond to the diplomat in character (1-3 sentences).\n";
    ss << "Diplomat: " << user_message << "\n" << ruler_name << ":";
    
    OllamaRequest req;
    req.model = model;
    req.prompt = ss.str();
    req.max_tokens = 150;
    req.temperature = 0.8f;
    return generate_blocking(req);
}

DiplomaticSummitResult AeonOllama::hold_summit(const std::string& ruler1_name, const std::string& civ1_name,
                                               const std::string& ruler2_name, const std::string& civ2_name,
                                               const std::string& topic, const std::string& model) {
    DiplomaticSummitResult res;
    std::ostringstream ss;
    ss << "System: Simulate a high-level diplomatic summit dialogue between "
       << ruler1_name << " (" << civ1_name << ") and "
       << ruler2_name << " (" << civ2_name << ") regarding: " << topic << ".\n"
       << "Write a 3-turn exchange between the rulers and end with [SUMMIT AGREED] or [SUMMIT REJECTED].\n";

    OllamaRequest req;
    req.model = model;
    req.prompt = ss.str();
    req.max_tokens = 250;
    req.temperature = 0.7f;

    std::string text = generate_blocking(req);
    if (text.empty()) {
        res.dialogue_transcript = ruler1_name + ": We propose peace.\n" + ruler2_name + ": Sol agrees to terms.";
        res.final_agreement = "PEACE_TREATY";
        res.agreed = true;
    } else {
        res.dialogue_transcript = text;
        res.agreed = (text.find("[SUMMIT AGREED]") != std::string::npos || text.find("agreed") != std::string::npos);
        res.final_agreement = res.agreed ? "AGREEMENT_SIGNED" : "NO_DEAL";
    }
    return res;
}

std::string AeonOllama::generate_propaganda_brief(const std::string& civ_name, const std::string& event_summary, const std::string& model) {
    std::ostringstream ss;
    ss << "System: Write a dramatic 2-sentence state press release for " << civ_name << " regarding event: " << event_summary << ".\n";

    OllamaRequest req;
    req.model = model;
    req.prompt = ss.str();
    req.max_tokens = 120;
    req.temperature = 0.85f;
    std::string out = generate_blocking(req);
    if (out.empty()) {
        return "OFFICIAL GAZETTE: Imperial forces remain vigilant across all administrative sectors.";
    }
    return out;
}

std::string AeonOllama::generate_custom_decree(const std::string& custom_prompt, const std::string& world_context, const std::string& model) {
    std::ostringstream ss;
    ss << "You are the LLM World God ruling over an evolving simulation. Context: " << world_context << "\n"
       << "The user commands: \"" << custom_prompt << "\"\n"
       << "Summarize your divine decree in 2 sentences and specify the impact on the realm:\n";

    OllamaRequest req;
    req.model = model;
    req.prompt = ss.str();
    req.max_tokens = 150;
    req.temperature = 0.8f;
    return generate_blocking(req);
}

std::string AeonOllama::format_tool_prompt(const std::string& ruler_name, const std::string& state_context) {
    std::ostringstream ss;
    ss << "System: You are " << ruler_name << ". You must select ONE action tool call for your empire.\n"
       << "AVAILABLE TOOLS:\n"
       << "1. set_tax_rate(rate_percent)\n"
       << "2. declare_war(target_civ_name)\n"
       << "3. propose_trade_deal(target_civ_name)\n"
       << "4. construct_world_wonder(wonder_name)\n"
       << "GAME STATE: " << state_context << "\n\n"
       << "Respond ONLY with a JSON object: {\"tool_name\": \"...\", \"parameter\": \"...\"}\n";
    return ss.str();
}

LLMToolCall AeonOllama::parse_tool_call(const std::string& llm_raw_response) {
    LLMToolCall call{"NO_ACTION", ""};
    try {
        size_t start = llm_raw_response.find('{');
        size_t end = llm_raw_response.rfind('}');
        if (start != std::string::npos && end != std::string::npos && end > start) {
            std::string json_str = llm_raw_response.substr(start, end - start + 1);
            auto j = json::parse(json_str);
            if (j.contains("tool_name")) call.tool_name = j["tool_name"].get<std::string>();
            if (j.contains("parameter")) call.parameter = j["parameter"].get<std::string>();
        }
    } catch (...) {}
    return call;
}

} // namespace Aeon
