#include "aeon_openrouter.h"
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Aeon {

std::string AeonOpenRouter::api_key = "";
std::string AeonOpenRouter::drakor_api_key = "";
std::string AeonOpenRouter::eldoria_api_key = "";
std::string AeonOpenRouter::model_name = "mistralai/mistral-large-2411";

bool AeonOpenRouter::is_configured() {
    if (!api_key.empty()) return true;

    // 1. Check openrouter_key.txt in working directory
    std::ifstream key_file("openrouter_key.txt");
    if (key_file.is_open()) {
        std::string line;
        if (std::getline(key_file, line) && !line.empty()) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (!line.empty()) {
                api_key = line;
                return true;
            }
        }
    }

    // 2. Check .env file
    std::ifstream env_file(".env");
    if (env_file.is_open()) {
        std::string line;
        while (std::getline(env_file, line)) {
            if (line.rfind("OPENROUTER_API_KEY=", 0) == 0) {
                size_t eq_pos = line.find('=');
                std::string val = line.substr(eq_pos + 1);
                val.erase(0, val.find_first_not_of(" \t\r\n\"'"));
                val.erase(val.find_last_not_of(" \t\r\n\"'") + 1);
                if (!val.empty()) {
                    api_key = val;
                    return true;
                }
            }
        }
    }

    // 3. Check environment variable
    const char* env_key = std::getenv("OPENROUTER_API_KEY");
    if (env_key && strlen(env_key) > 0) {
        api_key = std::string(env_key);
        return true;
    }

    return false;
}

std::string AeonOpenRouter::get_drakor_key() {
    if (!drakor_api_key.empty()) return drakor_api_key;

    // 1. Check openrouter_drakor_key.txt
    std::ifstream key_file("openrouter_drakor_key.txt");
    if (key_file.is_open()) {
        std::string line;
        if (std::getline(key_file, line) && !line.empty()) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (!line.empty()) {
                drakor_api_key = line;
                return drakor_api_key;
            }
        }
    }

    // 2. Check .env file
    std::ifstream env_file(".env");
    if (env_file.is_open()) {
        std::string line;
        while (std::getline(env_file, line)) {
            if (line.rfind("OPENROUTER_DRAKOR_API_KEY=", 0) == 0) {
                size_t eq_pos = line.find('=');
                std::string val = line.substr(eq_pos + 1);
                val.erase(0, val.find_first_not_of(" \t\r\n\"'"));
                val.erase(val.find_last_not_of(" \t\r\n\"'") + 1);
                if (!val.empty()) {
                    drakor_api_key = val;
                    return drakor_api_key;
                }
            }
        }
    }

    if (is_configured()) return api_key;
    return "";
}

std::string AeonOpenRouter::get_eldoria_key() {
    if (!eldoria_api_key.empty()) return eldoria_api_key;

    // 1. Check openrouter_eldoria_key.txt
    std::ifstream key_file("openrouter_eldoria_key.txt");
    if (key_file.is_open()) {
        std::string line;
        if (std::getline(key_file, line) && !line.empty()) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (!line.empty()) {
                eldoria_api_key = line;
                return eldoria_api_key;
            }
        }
    }

    // 2. Check .env file
    std::ifstream env_file(".env");
    if (env_file.is_open()) {
        std::string line;
        while (std::getline(env_file, line)) {
            if (line.rfind("OPENROUTER_ELDORIA_API_KEY=", 0) == 0) {
                size_t eq_pos = line.find('=');
                std::string val = line.substr(eq_pos + 1);
                val.erase(0, val.find_first_not_of(" \t\r\n\"'"));
                val.erase(val.find_last_not_of(" \t\r\n\"'") + 1);
                if (!val.empty()) {
                    eldoria_api_key = val;
                    return eldoria_api_key;
                }
            }
        }
    }

    if (is_configured()) return api_key;
    return "";
}

std::string AeonOpenRouter::generate_content(const std::string& prompt) {
    return generate_content_custom(prompt, "", "");
}

std::string AeonOpenRouter::generate_content_custom(const std::string& prompt, const std::string& key, const std::string& target_model) {
    std::string use_key = key;
    if (use_key.empty()) {
        is_configured();
        use_key = api_key;
    }
    std::string use_model = target_model.empty() ? model_name : target_model;

    if (use_key.empty()) {
        std::cout << "[OPENROUTER API] Key not set. Using local strategic simulation fallback." << std::endl;
        return "";
    }

    HINTERNET hSession = WinHttpOpen(L"Evolva/2.0 OpenRouter Client", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(hSession, L"openrouter.ai", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

    std::wstring path = L"/api/v1/chat/completions";

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    // Build OpenRouter JSON payload
    json payload = {
        {"model", use_model},
        {"messages", {{
            {"role", "user"},
            {"content", prompt}
        }}},
        {"temperature", 0.7},
        {"max_tokens", 300}
    };

    std::string req_body = payload.dump();
    std::wstring headers = L"Content-Type: application/json\r\nAuthorization: Bearer " + std::wstring(use_key.begin(), use_key.end()) + L"\r\nHTTP-Referer: https://github.com/evolva\r\nX-Title: Evolva Digital Life Simulator\r\n";

    BOOL bResults = WinHttpSendRequest(hRequest, headers.c_str(), -1L, (LPVOID)req_body.c_str(), (DWORD)req_body.length(), (DWORD)req_body.length(), 0);
    if (!bResults) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    bResults = WinHttpReceiveResponse(hRequest, NULL);
    std::string response_str = "";
    if (bResults) {
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        do {
            dwSize = 0;
            if (WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                if (dwSize == 0) break;
                char* pszOutBuffer = new char[dwSize + 1];
                ZeroMemory(pszOutBuffer, dwSize + 1);
                if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
                    response_str.append(pszOutBuffer, dwDownloaded);
                }
                delete[] pszOutBuffer;
            }
        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    try {
        auto resp_json = json::parse(response_str);
        if (resp_json.contains("choices") && !resp_json["choices"].empty()) {
            auto& choice = resp_json["choices"][0];
            if (choice.contains("message") && choice["message"].contains("content")) {
                return choice["message"]["content"].get<std::string>();
            }
        }
    } catch (...) {}

    return "";
}

OpenRouterStrategyResponse AeonOpenRouter::plan_ruler_strategy(
    const std::string& ruler_name,
    const std::string& civ_name,
    const std::string& personality,
    const std::string& world_context,
    float military_power,
    float treasury_gold,
    float gdp,
    bool at_war
) {
    OpenRouterStrategyResponse res;

    std::ostringstream prompt;
    prompt << "You are " << ruler_name << ", High Sovereign of " << civ_name << " powered by OpenRouter.\n"
           << "Personality: " << personality << "\n"
           << "Geopolitical Context:\n"
           << "- GDP: $" << gdp << " Gold\n"
           << "- Treasury Reserves: $" << treasury_gold << " Gold\n"
           << "- Military Power: " << military_power << " Regiments\n"
           << "- State of War: " << (at_war ? "AT WAR" : "AT PEACE") << "\n"
           << "- Global Context: " << world_context << "\n\n"
           << "Formulate your grand imperial strategy for " << civ_name << " and issue 1 binding decree.";

    std::string raw_reply = generate_content(prompt.str());

    if (!raw_reply.empty()) {
        res.strategic_plan = raw_reply;
        res.imperial_decree = "By Imperial Order of " + ruler_name + ": Execute OpenRouter Strategic Directive.";
        res.success = true;
    } else {
        res.strategic_plan = "[OpenRouter Strategic Brain]: High Sovereign " + ruler_name +
            " orders European trade expansion, industrial modernization, and regional alliance building.";
        res.imperial_decree = "Decree #" + std::to_string(rand() % 900 + 100) + ": Increase diplomatic budget and border fortresses.";
        res.target_action = at_war ? "MOBILIZE_DIVISIONS" : "INVEST_TECH";
        res.success = true;
    }

    return res;
}

} // namespace Aeon
