#include "aeon_nemotron.h"
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Aeon {

std::string AeonNemotron::api_key = "";
std::string AeonNemotron::model_name = "nvidia/llama-3.1-nemotron-70b-instruct";

bool AeonNemotron::is_configured() {
    if (!api_key.empty()) return true;

    // 1. Check nemotron_key.txt or nvidia_key.txt in working directory
    for (const char* filename : { "nemotron_key.txt", "nvidia_key.txt" }) {
        std::ifstream key_file(filename);
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
    }

    // 2. Check .env file
    std::ifstream env_file(".env");
    if (env_file.is_open()) {
        std::string line;
        while (std::getline(env_file, line)) {
            if (line.rfind("NVIDIA_API_KEY=", 0) == 0 || line.rfind("NEMOTRON_API_KEY=", 0) == 0) {
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

    // 3. Check environment variables
    for (const char* env_var : { "NVIDIA_API_KEY", "NEMOTRON_API_KEY" }) {
        const char* env_key = std::getenv(env_var);
        if (env_key && strlen(env_key) > 0) {
            api_key = std::string(env_key);
            return true;
        }
    }

    return false;
}

std::string AeonNemotron::generate_content(const std::string& prompt) {
    if (!is_configured()) {
        std::cout << "[NVIDIA NEMOTRON API] Key not set. Using local strategic simulation fallback." << std::endl;
        return "";
    }

    HINTERNET hSession = WinHttpOpen(L"Evolva/2.0 Nemotron Client", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(hSession, L"integrate.api.nvidia.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

    std::wstring path = L"/v1/chat/completions";

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    // Build NVIDIA NIM JSON payload
    json payload = {
        {"model", model_name},
        {"messages", {{
            {"role", "user"},
            {"content", prompt}
        }}},
        {"temperature", 0.6},
        {"max_tokens", 300}
    };

    std::string req_body = payload.dump();
    std::wstring headers = L"Content-Type: application/json\r\nAuthorization: Bearer " + std::wstring(api_key.begin(), api_key.end()) + L"\r\n";

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

NemotronStrategyResponse AeonNemotron::plan_ruler_strategy(
    const std::string& ruler_name,
    const std::string& civ_name,
    const std::string& personality,
    const std::string& world_context,
    float military_power,
    float treasury_gold,
    float gdp,
    bool at_war
) {
    NemotronStrategyResponse res;

    std::ostringstream prompt;
    prompt << "You are " << ruler_name << ", Supreme Ruler of the Kingdom of " << civ_name << " powered by NVIDIA Nemotron.\n"
           << "Personality: " << personality << "\n"
           << "Realm State:\n"
           << "- GDP: $" << gdp << " Gold\n"
           << "- Treasury Reserves: $" << treasury_gold << " Gold\n"
           << "- Military Power: " << military_power << " Army Divisions\n"
           << "- Conflict Status: " << (at_war ? "AT WAR" : "AT PEACE") << "\n"
           << "- World Geopolitical Context: " << world_context << "\n\n"
           << "Formulate your grand imperial strategy for the kingdom and issue 1 binding decree for your realm.";

    std::string raw_reply = generate_content(prompt.str());

    if (!raw_reply.empty()) {
        res.strategic_plan = raw_reply;
        res.imperial_decree = "By Imperial Decree of Sovereign " + ruler_name + ": Execute Nemotron High Command Directive.";
        res.success = true;
    } else {
        res.strategic_plan = "[NVIDIA Nemotron Strategic Brain]: Sovereign " + ruler_name +
            " orders border fortification, military modernization, and strategic resource accumulation.";
        res.imperial_decree = "Decree #" + std::to_string(rand() % 900 + 100) + ": Expand state infrastructure and technological research.";
        res.target_action = at_war ? "MOBILIZE_DIVISIONS" : "INVEST_TECH";
        res.success = true;
    }

    return res;
}

} // namespace Aeon
