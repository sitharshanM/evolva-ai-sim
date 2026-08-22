#pragma once
#include "aeon_types.h"
#include <vector>
#include <string>
#include <cstdint>

namespace Aeon {

// ─────────────────────────────────────────────────────────────────────────────
//  AsciiMap  —  Procedural world generator & ASCII renderer
// ─────────────────────────────────────────────────────────────────────────────
class AsciiMap {
public:
    AsciiMap() = default;

    // Generate a new world from a seed
    void generate(uint64_t seed);

    // Render full map to string (stdout)
    std::string render(int pan_x = 0, int pan_y = 0,
                       int zoom_w = MAP_WIDTH, int zoom_h = MAP_HEIGHT) const;

    // Render a sub-region (zoom + pan)
    std::string render_region(int origin_x, int origin_y, int w, int h) const;

    // Inspect a tile
    std::string inspect(int x, int y) const;

    // Overlay civilisation territory colours on top of biome chars
    void set_owner(int x, int y, int civ_id);
    void set_city(int x, int y, int city_id, char symbol = '@');

    const MapTile& tile(int x, int y) const { return tiles_[y * MAP_WIDTH + x]; }
    MapTile&       tile(int x, int y)       { return tiles_[y * MAP_WIDTH + x]; }

    std::vector<Landmark> landmarks;

private:
    std::vector<MapTile> tiles_; // MAP_WIDTH * MAP_HEIGHT flat grid
    std::vector<char>    overlay_; // city / faction overlay characters
    std::vector<int>     overlay_civ_; // which civ owns each tile for colour

    void generate_heightmap(uint64_t seed);
    void assign_biomes();
    void carve_rivers(uint64_t seed);
    void place_resources(uint64_t seed);
    void place_landmarks(uint64_t seed);
};

} // namespace Aeon
