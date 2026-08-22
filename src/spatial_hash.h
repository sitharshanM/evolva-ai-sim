#pragma once
#include <vector>
#include <unordered_map>
#include <cmath>
#include <glm/glm.hpp>
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
//  SpatialHash  —  O(1) average-case nearest-neighbor queries on a 2D grid
// ─────────────────────────────────────────────────────────────────────────────
struct SpatialHash {
    float cell_size;
    std::unordered_map<uint64_t, std::vector<int>> cells;

    explicit SpatialHash(float cs = Config::CELL_SIZE) : cell_size(cs) {}

    void clear() { cells.clear(); }

    void insert(int entity_id, glm::vec2 pos) {
        cells[key(pos)].push_back(entity_id);
    }

    // Returns all entity IDs whose cell overlaps a circle of 'radius' around 'pos'
    std::vector<int> query(glm::vec2 pos, float radius) const {
        std::vector<int> result;
        result.reserve(32);
        int r  = static_cast<int>(std::ceil(radius / cell_size));
        int cx = cell_x(pos.x);
        int cy = cell_y(pos.y);
        for (int dx = -r; dx <= r; ++dx) {
            for (int dy = -r; dy <= r; ++dy) {
                auto it = cells.find(key_ij(cx + dx, cy + dy));
                if (it != cells.end()) {
                    for (int id : it->second)
                        result.push_back(id);
                }
            }
        }
        return result;
    }

private:
    int cell_x(float x) const { return static_cast<int>(std::floor(x / cell_size)); }
    int cell_y(float y) const { return static_cast<int>(std::floor(y / cell_size)); }

    uint64_t key(glm::vec2 pos) const {
        return key_ij(cell_x(pos.x), cell_y(pos.y));
    }

    static uint64_t key_ij(int x, int y) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(x)))
             | (static_cast<uint64_t>(static_cast<uint32_t>(y)) << 32);
    }
};
