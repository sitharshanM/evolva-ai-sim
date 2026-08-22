#include "aeon_gemini.h"
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

#include <fstream>

namespace Aeon {

std::string AeonGemini::api_key = "";
std::string AeonGemini::model_name = "gemini-2.5-flash";

bool AeonGemini::is_configured() {
    if (!api_key.empty()) return true;

    // 1. Check gemini_key.txt in working directory
    std::ifstream key_file("gemini_key.txt");
    if (key_file.is_open()) {
        std::string line;
        if (std::getline(key_file, line) && !line.empty()) {
            // Trim whitespace/newlines
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
            if (line.rfind("GEMINI_API_KEY=", 0) == 0) {
                std::string val = line.substr(15);
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
    const char* env_key = std::getenv("GEMINI_API_KEY");
    if (env_key && strlen(env_key) > 0) {
        api_key = std::string(env_key);
        return true;
    }
    return false;
}


std::string AeonGemini::generate_content(const std::string& prompt) {
    if (!is_configured()) {
        std::cout << "[GEMINI API] No API Key provided. Returning local strategic fallback." << std::endl;
        return "";
    }

    HINTERNET hSession = WinHttpOpen(L"Evolva/2.0 Gemini Client", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return "";

    HINTERNET hConnect = WinHttpConnect(hSession, L"generativelanguage.googleapis.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return ""; }

    std::wstring path = L"/v1beta/models/gemini-flash-latest:generateContent?key=" + std::wstring(api_key.begin(), api_key.end());

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return ""; }

    // Build JSON payload
    json payload = {
        {"contents", {{
            {"parts", {{
                {"text", prompt}
            }}}
        }}}
    };
    std::string req_body = payload.dump();
    std::wstring headers = L"Content-Type: application/json\r\n";

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
        if (resp_json.contains("candidates") && !resp_json["candidates"].empty()) {
            return resp_json["candidates"][0]["content"]["parts"][0]["text"].get<std::string>();
        }
    } catch (...) {}

    return "";
}

GeminiStrategyResponse AeonGemini::plan_ruler_strategy(
    const std::string& ruler_name,
    const std::string& civ_name,
    const std::string& personality,
    const std::string& world_context,
    float military_power,
    float treasury_gold,
    float gdp,
    bool at_war
) {
    GeminiStrategyResponse res;

    std::ostringstream prompt;
    prompt << "You are " << ruler_name << ", High Sovereign of " << civ_name << ".\n"
           << "Personality: " << personality << "\n"
           << "Geopolitical Context:\n"
           << "- GDP: $" << gdp << " Gold\n"
           << "- Treasury Reserves: $" << treasury_gold << " Gold\n"
           << "- Military Power: " << military_power << " Divisions\n"
           << "- War State: " << (at_war ? "AT WAR" : "AT PEACE") << "\n"
           << "- World Summary: " << world_context << "\n\n"
           << "Formulate your grand 10-year strategic plan and issue 1 immediate binding Imperial Decree.";

    std::string raw_reply = generate_content(prompt.str());

    if (!raw_reply.empty()) {
        res.strategic_plan = raw_reply;
        res.imperial_decree = "By Decree of " + ruler_name + ": Execute Gemini Strategic Directive.";
        res.success = true;
    } else {
        // High-level fallback simulation logic when no key is set
        res.strategic_plan = "[Gemini 2.5 Strategic Simulation]: Sovereign " + ruler_name +
            " orders industrial modernization, fortification of borders, and strategic reserve accumulation.";
        res.imperial_decree = "Decree #" + std::to_string(rand() % 900 + 100) + ": Increase R&D budget by 15% and deploy border patrols.";
        res.target_action = at_war ? "MOBILIZE_DIVISIONS" : "INVEST_FUSION_ENERGY";
        res.success = true;
    }

    return res;
}

} // namespace Aeon
