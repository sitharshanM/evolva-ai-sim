#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "config.h"

enum class DiplomaticStatus {
    PEACE,
    ALLIANCE,
    WAR,
    VASSALAGE,   // Faction A is overlord, Faction B is vassal
    TRADE_PACT,
    NON_AGGRESSION_PACT
};

inline const char* diplomatic_status_str(DiplomaticStatus status) {
    switch (status) {
        case DiplomaticStatus::PEACE:               return "Peace";
        case DiplomaticStatus::ALLIANCE:            return "Alliance 🛡️";
        case DiplomaticStatus::WAR:                 return "WAR ⚔️";
        case DiplomaticStatus::VASSALAGE:           return "Vassalage 👑";
        case DiplomaticStatus::TRADE_PACT:          return "Trade Pact 💰";
        case DiplomaticStatus::NON_AGGRESSION_PACT: return "NAP 📜";
    }
    return "Unknown";
}

struct DiplomaticRelation {
    DiplomaticStatus status = DiplomaticStatus::PEACE;
    float duration = 0.0f;          // how long relation has lasted
    int   clashes_count = 0;        // recent combat skirmishes
    std::string treaty_name;        // e.g. "Pact of the Northern River"
    std::vector<std::string> grievances;
};

class DiplomaticMatrix {
public:
    DiplomaticMatrix() = default;

    void add_grievance(int f1, int f2, const std::string& msg) {
        if (f1 == f2) return;
        uint64_t k = key(f1, f2);
        auto& g = relations_[k].grievances;
        if (g.size() >= 5) g.erase(g.begin());
        g.push_back(msg);
    }

    void set_status(int f1, int f2, DiplomaticStatus status, const std::string& treaty = "") {
        if (f1 == f2) return;
        uint64_t k = key(f1, f2);
        relations_[k].status = status;
        relations_[k].duration = 0.0f;
        if (!treaty.empty()) relations_[k].treaty_name = treaty;
    }

    DiplomaticStatus get_status(int f1, int f2) const {
        if (f1 == f2) return DiplomaticStatus::ALLIANCE; // same faction is always allied
        uint64_t k = key(f1, f2);
        auto it = relations_.find(k);
        if (it != relations_.end()) return it->second.status;
        return DiplomaticStatus::PEACE;
    }

    bool is_at_war(int f1, int f2) const {
        return get_status(f1, f2) == DiplomaticStatus::WAR;
    }

    bool is_allied(int f1, int f2) const {
        return f1 == f2 || get_status(f1, f2) == DiplomaticStatus::ALLIANCE;
    }

    const DiplomaticRelation* get_relation(int f1, int f2) const {
        uint64_t k = key(f1, f2);
        auto it = relations_.find(k);
        if (it != relations_.end()) return &it->second;
        return nullptr;
    }

    void tick(float dt) {
        for (auto& [k, rel] : relations_) {
            rel.duration += dt;
        }
    }

private:
    static uint64_t key(int f1, int f2) {
        int a = std::min(f1, f2);
        int b = std::max(f1, f2);
        return (static_cast<uint64_t>(a) << 32) | static_cast<uint64_t>(b);
    }

    std::unordered_map<uint64_t, DiplomaticRelation> relations_;
};
