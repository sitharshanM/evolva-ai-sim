#include "world_layer.h"
#include <algorithm>
#include <cstdlib>

void WorldLayer::init() {
    infrastructure.clear();
    disasters.clear();

    global_temperature = 22.0f;
    global_pollution   = 0.05f;

    // Seed basic world infrastructure ports and power plants
    for (int i = 0; i < 6; ++i) {
        InfrastructureNode port;
        port.pos = glm::vec2(200.0f + i * 500.0f, 300.0f + (i % 2) * 600.0f);
        port.type = "Deepwater Port";
        port.faction_id = i;
        port.level = 2.0f;
        infrastructure.push_back(port);

        InfrastructureNode plant;
        plant.pos = glm::vec2(300.0f + i * 500.0f, 400.0f + (i % 2) * 600.0f);
        plant.type = "Power Plant Grid";
        plant.faction_id = i;
        plant.level = 1.5f;
        infrastructure.push_back(plant);
    }
}

void WorldLayer::update(float dt) {
    for (auto& d : disasters) {
        d.duration -= dt;
    }
    disasters.erase(
        std::remove_if(disasters.begin(), disasters.end(),
            [](const WorldDisaster& d) { return d.duration <= 0.0f; }),
        disasters.end());
}
