#include "ruler_ai.h"
#include <cmath>

void RulerAI::update_memory_decay(float dt) {
    // Memory decay over time (Hatred -> 15 after 100 simulated seconds unless reinforced)
    for (auto& mem : memory_matrix) {
        if (mem.hatred_impact > 15.0f) {
            mem.hatred_impact -= 0.05f * dt;
        }
        if (mem.fear_impact > 10.0f) {
            mem.fear_impact -= 0.04f * dt;
        }
    }
}

void RulerAI::add_memory(const std::string& desc, int source_f, float trust, float hatred, float fear_v) {
    if (memory_matrix.size() >= 20) memory_matrix.erase(memory_matrix.begin());
    MemoryEntry m;
    m.event_desc     = desc;
    m.source_faction = source_f;
    m.trust_impact   = trust;
    m.hatred_impact  = hatred;
    m.fear_impact    = fear_v;
    memory_matrix.push_back(m);
}
