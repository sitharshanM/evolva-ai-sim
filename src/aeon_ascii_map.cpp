#include "aeon_ascii_map.h"
#include <sstream>
#include <cmath>
#include <random>
#include <algorithm>

namespace Aeon {

// ─── Noise helper (simple seeded) ────────────────────────────────────────────
static float noise2(int x, int y, uint64_t seed) {
    uint64_t h = seed ^ (uint64_t(x) * 2654435761ULL) ^ (uint64_t(y) * 2246822519ULL);
    h ^= h >> 33; h *= 0xff51afd7ed558ccdULL; h ^= h >> 33;
    return float(h & 0xFFFF) / 65535.0f;
}

static float smooth_noise(int x, int y, uint64_t seed, int octaves = 4) {
    float val = 0.0f, amp = 1.0f, freq = 1.0f, max_val = 0.0f;
    for (int o = 0; o < octaves; ++o) {
        int sx = int(x / freq), sy = int(y / freq);
        val     += noise2(sx, sy, seed + o) * amp;
        max_val += amp;
        amp  *= 0.5f;
        freq *= 2.0f;
    }
    return val / max_val;
}

// ─── Real Earth Geography Model ───────────────────────────────────────────────
static bool eval_earth_point(float lon, float lat, float& out_elev, Biome& out_biome, float& out_moist, uint64_t seed) {
    // lon in [-180..+180], lat in [-90..+90]
    out_elev = 0.1f;
    out_biome = Biome::OCEAN;
    out_moist = 0.8f;
    bool is_land = false;

    // 1. Antarctica
    if (lat < -60.0f) {
        float edge_dist = (-60.0f - lat) / 30.0f;
        out_elev = 0.7f + edge_dist * 0.25f;
        out_biome = Biome::ICE;
        out_moist = 0.1f;
        return true;
    }

    // 2. Greenland
    if (lat >= 60.0f && lat <= 83.0f && lon >= -55.0f && lon <= -18.0f) {
        out_elev = 0.75f;
        out_biome = (lat > 68.0f || (lon > -45.0f && lon < -25.0f)) ? Biome::ICE : Biome::TUNDRA;
        out_moist = 0.2f;
        return true;
    }

    // 3. North America
    // Alaska
    if (lat >= 54.0f && lat <= 71.0f && lon >= -168.0f && lon <= -130.0f) {
        is_land = true;
        out_elev = 0.65f;
        out_biome = (lat > 66.0f) ? Biome::TUNDRA : Biome::FOREST;
        out_moist = 0.6f;
    }
    // Canada & Continental US
    else if (lat >= 25.0f && lat <= 72.0f && lon >= -126.0f && lon <= -58.0f) {
        // Exclude ocean cuts (Gulf of Mexico, Hudson Bay)
        bool in_hudson = (lat >= 52.0f && lat <= 64.0f && lon >= -94.0f && lon <= -78.0f);
        bool in_gulf   = (lat >= 20.0f && lat <= 29.0f && lon >= -95.0f && lon <= -82.0f);
        bool in_pacific = (lon < -124.0f && lat < 48.0f);
        bool in_atlantic = (lon > -70.0f && lat < 42.0f && lat > 28.0f);

        if (!in_hudson && !in_gulf && !in_pacific && !in_atlantic) {
            is_land = true;
            // Elevation & Biome profiling
            if (lon >= -122.0f && lon <= -104.0f && lat >= 32.0f && lat <= 60.0f) {
                out_elev = 0.85f; // Rocky Mountains & Cascades
                out_biome = (out_elev > 0.82f) ? Biome::MOUNTAIN : Biome::FOREST;
            } else if (lon >= -118.0f && lon <= -102.0f && lat >= 26.0f && lat <= 38.0f) {
                out_elev = 0.55f;
                out_biome = Biome::DESERT; // Mojave / Sonora / Chihuahuan
                out_moist = 0.15f;
            } else if (lat >= 58.0f) {
                out_elev = 0.45f;
                out_biome = Biome::TUNDRA;
                out_moist = 0.3f;
            } else if (lon >= -104.0f && lon <= -88.0f) {
                out_elev = 0.48f;
                out_biome = Biome::PLAINS; // Great Plains Breadbasket
                out_moist = 0.55f;
            } else {
                out_elev = 0.45f;
                out_biome = Biome::FERTILE_VALLEY; // Midwest / East Coast
                out_moist = 0.70f;
            }
        }
    }
    // Mexico & Central America
    else if (lat >= 7.0f && lat <= 32.0f && lon >= -116.0f && lon <= -77.0f) {
        float width_factor = (lat - 7.0f) / 25.0f; // tapers down to Panama
        float min_lon = -116.0f + (1.0f - width_factor) * 35.0f;
        float max_lon = -77.0f - (1.0f - width_factor) * 2.0f;
        if (lon >= min_lon && lon <= max_lon) {
            is_land = true;
            out_elev = 0.60f;
            out_biome = (lat < 20.0f) ? Biome::JUNGLE : Biome::SCRUBLAND;
            out_moist = (lat < 20.0f) ? 0.85f : 0.35f;
        }
    }
    // Caribbean Islands
    else if (lat >= 17.0f && lat <= 26.0f && lon >= -85.0f && lon <= -64.0f) {
        float n = smooth_noise(int((lon + 180) * 4), int((lat + 90) * 4), seed);
        if (n > 0.68f) {
            is_land = true;
            out_elev = 0.42f;
            out_biome = Biome::COAST;
            out_moist = 0.80f;
        }
    }

    // 4. South America
    else if (lat >= -56.0f && lat <= 13.0f && lon >= -82.0f && lon <= -34.0f) {
        // Tapering southern cone
        float min_lon = -82.0f;
        float max_lon = -34.0f;
        if (lat < -20.0f) {
            float taper = (-20.0f - lat) / 36.0f;
            min_lon = -75.0f + taper * 8.0f;
            max_lon = -45.0f - taper * 22.0f;
        }
        if (lon >= min_lon && lon <= max_lon) {
            is_land = true;
            // Andes Mountains along Western Ridge
            if (lon <= min_lon + 6.0f) {
                out_elev = 0.90f;
                out_biome = Biome::MOUNTAIN;
            } else if (lat >= -15.0f && lat <= 6.0f && lon <= -45.0f) {
                out_elev = 0.45f;
                out_biome = Biome::JUNGLE; // Amazon Rainforest
                out_moist = 0.95f;
            } else if (lat < -30.0f) {
                out_elev = 0.48f;
                out_biome = (lon < -68.0f) ? Biome::MOUNTAIN : Biome::GRASSLAND; // Pampas
                out_moist = 0.50f;
            } else {
                out_elev = 0.52f;
                out_biome = Biome::FERTILE_VALLEY; // Brazilian Highlands
                out_moist = 0.70f;
            }
        }
    }

    // 5. Europe
    else if (lat >= 35.0f && lat <= 71.0f && lon >= -11.0f && lon <= 55.0f) {
        // British Isles
        bool is_uk = (lon >= -10.0f && lon <= 2.0f && lat >= 50.0f && lat <= 59.0f);
        // Scandinavia
        bool is_scandi = (lon >= 5.0f && lon <= 30.0f && lat >= 55.0f && lat <= 71.0f);
        // Iberia (Spain/Portugal)
        bool is_iberia = (lon >= -10.0f && lon <= 3.0f && lat >= 36.0f && lat <= 44.0f);
        // Italy
        bool is_italy = (lon >= 8.0f && lon <= 18.0f && lat >= 37.0f && lat <= 46.0f);
        // Mainland Western/Central/Eastern Europe
        bool is_mainland = (lon >= 0.0f && lon <= 55.0f && lat >= 43.0f && lat <= 65.0f);

        // Mediterranean Cutout
        bool in_med = (lat >= 33.0f && lat <= 41.0f && lon >= 0.0f && lon <= 35.0f && !is_italy && !is_iberia);

        if ((is_uk || is_scandi || is_iberia || is_italy || is_mainland) && !in_med) {
            is_land = true;
            if (lat >= 45.0f && lat <= 47.5f && lon >= 6.0f && lon <= 15.0f) {
                out_elev = 0.88f; // Alps
                out_biome = Biome::PEAK;
            } else if (is_scandi && lat >= 64.0f) {
                out_elev = 0.60f;
                out_biome = Biome::TUNDRA;
            } else if (is_iberia || (is_italy && lat < 42.0f)) {
                out_elev = 0.50f;
                out_biome = Biome::SCRUBLAND;
                out_moist = 0.40f;
            } else {
                out_elev = 0.45f;
                out_biome = Biome::FERTILE_VALLEY; // European Plain
                out_moist = 0.75f;
            }
        }
    }

    // 6. Africa
    else if (lat >= -35.0f && lat <= 37.0f && lon >= -18.0f && lon <= 52.0f) {
        // Red Sea Cutout
        bool in_red_sea = (lat >= 12.0f && lat <= 28.0f && lon >= 32.0f && lon <= 44.0f && (lon - lat * 0.7f > 15.0f));
        // Gulf of Guinea Cutout
        bool in_guinea_bight = (lat >= 0.0f && lat <= 5.0f && lon >= 0.0f && lon <= 9.0f);

        // Southern tapering
        float min_lon = -18.0f;
        float max_lon = 52.0f;
        if (lat < 5.0f) {
            min_lon = 8.0f + (-lat) * 0.2f;
            max_lon = 42.0f - (-lat) * 0.3f;
        }

        if (!in_red_sea && !in_guinea_bight && lon >= min_lon && lon <= max_lon) {
            is_land = true;
            if (lat >= 16.0f && lat <= 32.0f) {
                out_elev = 0.48f;
                out_biome = Biome::DESERT; // Sahara Desert
                out_moist = 0.05f;
            } else if (lat >= 10.0f && lat < 16.0f) {
                out_elev = 0.45f;
                out_biome = Biome::SCRUBLAND; // Sahel
                out_moist = 0.25f;
            } else if (lat >= -8.0f && lat < 10.0f && lon <= 30.0f) {
                out_elev = 0.45f;
                out_biome = Biome::JUNGLE; // Congo Basin
                out_moist = 0.90f;
            } else if (lat >= 30.0f && lon <= 0.0f) {
                out_elev = 0.75f; // Atlas Mountains
                out_biome = Biome::MOUNTAIN;
            } else {
                out_elev = 0.52f;
                out_biome = Biome::GRASSLAND; // Savannah & Southern Africa
                out_moist = 0.55f;
            }
        }
        // Madagascar
        if (lat >= -26.0f && lat <= -12.0f && lon >= 43.0f && lon <= 51.0f) {
            is_land = true;
            out_elev = 0.55f;
            out_biome = Biome::JUNGLE;
            out_moist = 0.85f;
        }
    }

    // 7. Asia
    else if (lat >= 1.0f && lat <= 78.0f && lon >= 26.0f && lon <= 178.0f) {
        // Persian Gulf cutout
        bool in_persian_gulf = (lat >= 24.0f && lat <= 30.0f && lon >= 48.0f && lon <= 56.0f);
        // Bay of Bengal / Arabian sea separation for India
        bool is_india = (lat >= 8.0f && lat <= 35.0f && lon >= 68.0f && lon <= 90.0f);
        bool in_arabian_sea = (lat >= 5.0f && lat <= 23.0f && lon >= 58.0f && lon <= 68.0f);
        bool in_bay_bengal = (lat >= 5.0f && lat <= 20.0f && lon >= 82.0f && lon <= 93.0f && !is_india);

        // Southeast Asia
        bool is_se_asia = (lat >= 8.0f && lat <= 24.0f && lon >= 96.0f && lon <= 109.0f);
        // Korean peninsula
        bool is_korea = (lat >= 34.0f && lat <= 42.0f && lon >= 124.0f && lon <= 130.0f);
        // Japan
        bool is_japan = (lat >= 30.0f && lat <= 45.0f && lon >= 130.0f && lon <= 145.0f);

        // Mainland Asia
        bool is_main_asia = (lat >= 20.0f && lat <= 76.0f && lon >= 35.0f && lon <= 175.0f);

        if (!in_persian_gulf && !in_arabian_sea && !in_bay_bengal && (is_india || is_se_asia || is_korea || is_japan || is_main_asia)) {
            is_land = true;
            // Himalayas & Tibetan Plateau
            if (lat >= 27.0f && lat <= 38.0f && lon >= 75.0f && lon <= 102.0f) {
                out_elev = 0.95f; // Roof of the World
                out_biome = Biome::PEAK;
                out_moist = 0.20f;
            } else if (lat >= 12.0f && lat <= 32.0f && lon >= 36.0f && lon <= 58.0f) {
                out_elev = 0.45f;
                out_biome = Biome::DESERT; // Arabian Peninsula
                out_moist = 0.05f;
            } else if (lat >= 38.0f && lat <= 48.0f && lon >= 95.0f && lon <= 116.0f) {
                out_elev = 0.58f;
                out_biome = Biome::DESERT; // Gobi Desert
                out_moist = 0.12f;
            } else if (lat >= 60.0f) {
                out_elev = 0.48f;
                out_biome = Biome::TUNDRA; // Siberia North
                out_moist = 0.35f;
            } else if (lat >= 48.0f && lat < 60.0f) {
                out_elev = 0.46f;
                out_biome = Biome::FOREST; // Siberian Taiga
                out_moist = 0.65f;
            } else if (is_india) {
                out_elev = 0.45f;
                out_biome = (lat > 24.0f) ? Biome::FERTILE_VALLEY : Biome::JUNGLE; // Gangetic Plain / Deccan
                out_moist = 0.85f;
            } else if (is_japan || is_korea) {
                out_elev = 0.60f;
                out_biome = Biome::FOREST;
                out_moist = 0.80f;
            } else {
                out_elev = 0.48f;
                out_biome = Biome::FERTILE_VALLEY; // East China / Yangtze & Yellow River Basins
                out_moist = 0.78f;
            }
        }
    }

    // 8. Maritime Southeast Asia / Indonesia / Philippines
    else if (lat >= -11.0f && lat <= 19.0f && lon >= 95.0f && lon <= 130.0f) {
        float n = smooth_noise(int((lon + 180) * 3), int((lat + 90) * 3), seed + 50);
        if (n > 0.58f) {
            is_land = true;
            out_elev = 0.50f;
            out_biome = Biome::JUNGLE;
            out_moist = 0.95f;
        }
    }

    // 9. Australia & Oceania
    else if (lat >= -40.0f && lat <= -10.0f && lon >= 112.0f && lon <= 155.0f) {
        is_land = true;
        if (lon >= 118.0f && lon <= 139.0f && lat <= -18.0f && lat >= -32.0f) {
            out_elev = 0.48f;
            out_biome = Biome::DESERT; // Australian Outback
            out_moist = 0.10f;
        } else if (lon >= 140.0f) {
            out_elev = 0.58f;
            out_biome = Biome::FERTILE_VALLEY; // Eastern Coast / Great Dividing Range
            out_moist = 0.70f;
        } else {
            out_elev = 0.45f;
            out_biome = Biome::SCRUBLAND;
            out_moist = 0.35f;
        }
    }
    // New Zealand
    else if (lat >= -48.0f && lat <= -34.0f && lon >= 165.0f && lon <= 179.0f) {
        float n = smooth_noise(int((lon + 180) * 3), int((lat + 90) * 3), seed + 77);
        if (n > 0.55f) {
            is_land = true;
            out_elev = 0.70f;
            out_biome = Biome::MOUNTAIN;
            out_moist = 0.85f;
        }
    }
    // Papua New Guinea
    else if (lat >= -11.0f && lat <= -2.0f && lon >= 130.0f && lon <= 152.0f) {
        is_land = true;
        out_elev = 0.65f;
        out_biome = Biome::JUNGLE;
        out_moist = 0.95f;
    }

    // Apply fine fractal noise to coastlines
    if (is_land) {
        float coastal_noise = smooth_noise(int((lon + 180) * 6), int((lat + 90) * 6), seed + 123, 3);
        if (coastal_noise < 0.22f && out_elev < 0.50f) {
            // Natural coastal inlet / bay
            is_land = false;
            out_biome = Biome::COAST;
            out_elev = 0.28f;
        }
    }

    return is_land;
}

// ─── Generate ─────────────────────────────────────────────────────────────────
void AsciiMap::generate(uint64_t seed) {
    tiles_.resize(MAP_WIDTH * MAP_HEIGHT);
    overlay_.assign(MAP_WIDTH * MAP_HEIGHT, 0);
    overlay_civ_.assign(MAP_WIDTH * MAP_HEIGHT, -1);

    generate_heightmap(seed);
    assign_biomes();
    carve_rivers(seed + 1);
    place_resources(seed + 2);
    place_landmarks(seed + 3);
}

void AsciiMap::generate_heightmap(uint64_t seed) {
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        float lat = 90.0f - (float(y) / MAP_HEIGHT) * 180.0f; // +90 (North) to -90 (South)
        for (int x = 0; x < MAP_WIDTH; ++x) {
            float lon = (float(x) / MAP_WIDTH) * 360.0f - 180.0f; // -180 (West) to +180 (East)

            float elev = 0.1f;
            Biome b = Biome::OCEAN;
            float moist = 0.5f;

            bool is_land = eval_earth_point(lon, lat, elev, b, moist, seed);

            auto& t = tile(x, y);
            t.elevation = elev;
            t.moisture  = moist;
            t.biome     = is_land ? b : (elev > 0.25f ? Biome::COAST : Biome::OCEAN);
            t.fertility = is_land ? std::max(0.1f, moist * (1.0f - std::abs(lat) / 90.0f) * 1.2f) : 0.0f;
        }
    }
}

void AsciiMap::assign_biomes() {
    // Biomes are accurately assigned directly from Earth model during generation
}

void AsciiMap::carve_rivers(uint64_t seed) {
    (void)seed;
    // Major real-world Earth river arteries
    struct RiverDef { const char* name; int sx, sy; int ex, ey; };
    RiverDef rivers[] = {
        { "Nile River",        212, 85, 211, 60 }, // Lake Victoria -> Cairo
        { "Amazon River",      105, 93, 130, 90 }, // Andes -> Atlantic
        { "Mississippi River", 90, 48,  91, 61 }, // Minnesota -> New Orleans
        { "Yangtze River",     278, 57, 301, 58 }, // Tibet -> Shanghai
        { "Ganges River",      258, 62, 268, 67 }, // Himalayas -> Bay of Bengal
        { "Danube River",      188, 42, 208, 45 }, // Black Forest -> Black Sea
        { "Volga River",       218, 33, 228, 44 }, // Valdai -> Caspian Sea
        { "Rhine River",       186, 44, 185, 38 }, // Alps -> North Sea
        { "Congo River",       200, 95, 192, 96 }, // Lualaba -> Atlantic
        { "Indus River",       252, 58, 248, 66 }, // Karakoram -> Arabian Sea
    };

    for (const auto& r : rivers) {
        int steps = std::max(std::abs(r.ex - r.sx), std::abs(r.ey - r.sy));
        for (int s = 0; s <= steps; ++s) {
            float t = (steps > 0) ? float(s) / steps : 0.0f;
            int cx = int(r.sx + (r.ex - r.sx) * t);
            int cy = int(r.sy + (r.ey - r.sy) * t);
            if (cx >= 0 && cx < MAP_WIDTH && cy >= 0 && cy < MAP_HEIGHT) {
                auto& tile_obj = tile(cx, cy);
                tile_obj.has_river = true;
                tile_obj.moisture = std::min(1.0f, tile_obj.moisture + 0.35f);
                if (tile_obj.biome == Biome::DESERT) tile_obj.biome = Biome::FERTILE_VALLEY; // Nile delta fertile valley
            }
        }
    }
}

void AsciiMap::place_resources(uint64_t seed) {
    (void)seed;
    // Accurate real-world strategic resource deposits across Earth
    struct ResLocation { int x, y; ResourceKind res; };
    ResLocation deposits[] = {
        // Oil & Gas
        { 230, 64, ResourceKind::OIL },           // Ghawar / Persian Gulf
        { 80,  60, ResourceKind::OIL },           // Texas Permian Basin
        { 183, 35, ResourceKind::OIL },           // North Sea
        { 258, 30, ResourceKind::OIL },           // Samotlor Siberia
        { 110, 78, ResourceKind::OIL },           // Venezuela Orinoco
        // Iron Ore
        { 300, 112, ResourceKind::IRON },         // Pilbara Australia
        { 128, 96,  ResourceKind::IRON },         // Carajas Brazil
        { 198, 23,  ResourceKind::IRON },         // Kiruna Sweden
        { 90,  44,  ResourceKind::IRON },         // Mesabi Iron Range US
        { 298, 49,  ResourceKind::IRON },         // Anshan China
        // Coal
        { 100, 52,  ResourceKind::COAL },         // Appalachia US
        { 187, 39,  ResourceKind::COAL },         // Ruhr Valley Germany
        { 292, 53,  ResourceKind::COAL },         // Shanxi China
        { 268, 35,  ResourceKind::COAL },         // Kuzbass Russia
        // Rare Minerals
        { 290, 48,  ResourceKind::RARE_MINERALS },// Bayan Obo China
        { 64,  55,  ResourceKind::RARE_MINERALS },// Mountain Pass California
        { 206, 99,  ResourceKind::RARE_MINERALS },// Katanga Congo
        // Gold
        { 208, 116, ResourceKind::GOLD_ORE },     // Witwatersrand South Africa
        { 63,  51,  ResourceKind::GOLD_ORE },     // Nevada Carlin Trend
        { 308, 120, ResourceKind::GOLD_ORE },     // Kalgoorlie Australia
        { 295, 28,  ResourceKind::GOLD_ORE },     // Kolyma Siberia
        // Copper
        { 108, 110, ResourceKind::COPPER },       // Escondida Chile
        { 68,  58,  ResourceKind::COPPER },       // Arizona Copper Basin
        { 204, 98,  ResourceKind::COPPER },       // Zambian Copperbelt
        // Wood & Stone
        { 75,  38,  ResourceKind::WOOD },         // Canadian Boreal Forest
        { 240, 26,  ResourceKind::WOOD },         // Siberian Taiga
        { 120, 92,  ResourceKind::WOOD },         // Amazon Rainforest
        { 188, 46,  ResourceKind::STONE },        // Alps
        { 265, 59,  ResourceKind::STONE },        // Himalayas
    };

    for (const auto& d : deposits) {
        if (d.x >= 0 && d.x < MAP_WIDTH && d.y >= 0 && d.y < MAP_HEIGHT) {
            tile(d.x, d.y).resources.push_back(d.res);
        }
    }
}

void AsciiMap::place_landmarks(uint64_t seed) {
    (void)seed;
    landmarks.clear();

    // Strategic Earth landmarks, canals, spaceports & wonders
    struct EarthLandmark { int x, y; LandmarkType type; const char* name; const char* desc; };
    EarthLandmark earth_landmarks[] = {
        { 100, 81, LandmarkType::NATURAL_HARBOR, "Panama Canal", "Trans-oceanic maritime canal connecting Atlantic and Pacific." },
        { 212, 60, LandmarkType::NATURAL_HARBOR, "Suez Canal", "Vital shipping artery linking Mediterranean to Red Sea & Indian Ocean." },
        { 282, 87, LandmarkType::NATURAL_HARBOR, "Strait of Malacca", "Global energy and trade maritime chokepoint." },
        { 175, 54, LandmarkType::NATURAL_HARBOR, "Strait of Gibraltar", "Pillars of Hercules commanding entry to the Mediterranean." },
        { 106, 50, LandmarkType::ANCIENT_RUINS,  "Statue of Liberty & NYC", "Iconic gateway and financial hub of North America." },
        { 182, 42, LandmarkType::ANCIENT_RUINS,  "Eiffel Tower & Paris", "Cultural beacon and historic capital of Western Europe." },
        { 211, 60, LandmarkType::ANCIENT_RUINS,  "Great Pyramids of Giza", "Ancient Wonder of the World and tomb of Pharaohs." },
        { 296, 50, LandmarkType::ANCIENT_RUINS,  "Great Wall of China", "Massive stone fortification across the northern frontier." },
        { 258, 63, LandmarkType::ANCIENT_RUINS,  "Taj Mahal", "Crown jewel of Mughal architecture on the Yamuna river." },
        { 99,  62, LandmarkType::MOUNTAIN_PASS,   "Cape Canaveral Spaceport", "Primary orbital launch facility of the Americas." },
        { 244, 45, LandmarkType::MOUNTAIN_PASS,   "Baikonur Cosmodrome", "World's first and largest operational space launch complex." },
        { 58,  53, LandmarkType::MOUNTAIN_PASS,   "Silicon Valley", "Global epicentre of high-tech and artificial intelligence." },
    };

    for (const auto& lm_def : earth_landmarks) {
        if (lm_def.x >= 0 && lm_def.x < MAP_WIDTH && lm_def.y >= 0 && lm_def.y < MAP_HEIGHT) {
            Landmark lm;
            lm.x = lm_def.x;
            lm.y = lm_def.y;
            lm.type = lm_def.type;
            lm.name = lm_def.name;
            lm.description = lm_def.desc;
            tile(lm.x, lm.y).strategic = true;
            landmarks.push_back(lm);
        }
    }
}

// ─── Render ───────────────────────────────────────────────────────────────────
std::string AsciiMap::render(int pan_x, int pan_y, int zoom_w, int zoom_h) const {
    return render_region(pan_x, pan_y, zoom_w, zoom_h);
}

std::string AsciiMap::render_region(int origin_x, int origin_y, int w, int h) const {
    std::ostringstream ss;
    ss << "+";
    for (int x = 0; x < w; ++x) ss << "-";
    ss << "+\n";

    for (int row = 0; row < h; ++row) {
        int y = origin_y + row;
        ss << "|";
        for (int col = 0; col < w; ++col) {
            int x = origin_x + col;
            if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
                ss << " ";
                continue;
            }
            char ov = overlay_[y * MAP_WIDTH + x];
            if (ov != 0) {
                ss << ov;
            } else {
                const auto& t = tile(x, y);
                if (t.has_river && t.biome != Biome::OCEAN)
                    ss << "o";
                else if (t.strategic)
                    ss << "S";
                else
                    ss << biome_char(t.biome);
            }
        }
        ss << "|\n";
    }

    ss << "+";
    for (int x = 0; x < w; ++x) ss << "-";
    ss << "+\n";

    return ss.str();
}

std::string AsciiMap::inspect(int x, int y) const {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT)
        return "  [Out of bounds]\n";

    const auto& t = tile(x, y);
    float lat = 90.0f - (float(y) / MAP_HEIGHT) * 180.0f;
    float lon = (float(x) / MAP_WIDTH) * 360.0f - 180.0f;

    std::ostringstream ss;
    ss << "  Earth Coordinates: (" << int(std::abs(lat)) << (lat >= 0 ? "N" : "S") << ", "
       << int(std::abs(lon)) << (lon >= 0 ? "E" : "W") << ") | Grid [" << x << ", " << y << "]\n";
    ss << "  Biome     : " << biome_name(t.biome) << "\n";
    ss << "  Elevation : " << int(t.elevation * 100) << "%\n";
    ss << "  Moisture  : " << int(t.moisture  * 100) << "%\n";
    ss << "  Fertility : " << int(t.fertility * 100) << "%\n";
    ss << "  River     : " << (t.has_river ? "Yes" : "No") << "\n";
    ss << "  Strategic : " << (t.strategic  ? "Yes" : "No") << "\n";
    if (!t.resources.empty()) {
        ss << "  Resources : ";
        for (auto r : t.resources) ss << resource_name(r) << " ";
        ss << "\n";
    }
    if (t.owner_civ >= 0) ss << "  Sovereign : Civ #" << t.owner_civ << "\n";
    return ss.str();
}

void AsciiMap::set_owner(int x, int y, int civ_id) {
    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
        overlay_civ_[y * MAP_WIDTH + x] = civ_id;
        tile(x, y).owner_civ = civ_id;
    }
}

void AsciiMap::set_city(int x, int y, int /*city_id*/, char symbol) {
    if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT)
        overlay_[y * MAP_WIDTH + x] = symbol;
}

} // namespace Aeon
