#pragma once
#include <string>

namespace Aeon {

class AeonEngine; // forward decl

class SaveLoadEngine {
public:
    // Save full simulation state to JSON file under d:/evolva/saves/
    static bool save(const AeonEngine& engine, const std::string& filename);

    // Load full simulation state from JSON file
    static bool load(AeonEngine& engine, const std::string& filename);
};

} // namespace Aeon
