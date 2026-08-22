#ifndef AEON_MARITIME_H
#define AEON_MARITIME_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct MaritimeCargoShip {
    int id = 0;
    int owner_civ_id = 0;
    std::string origin_port;
    std::string dest_port;
    int x = 0;
    int y = 0;
    double cargo_value_gold = 5000.0;
    bool intercepted_by_pirates = false;
    bool sunk_by_storm = false;
};

struct NavalBlockade {
    int attacker_civ_id = 0;
    int target_civ_id = 0;
    std::string port_name;
    bool active = true;
};

struct PirateHaven {
    int id = 0;
    std::string name;
    int x = 0;
    int y = 0;
    int pirate_strength = 200;
    float accumulated_loot = 5000.0f;
    bool destroyed = false;
};

struct LetterOfMarque {
    int patron_civ_id = -1;
    int target_civ_id = -1;
    float bounty_cut_pct = 40.0f; // 40% of raided loot goes to state
    bool active = true;
};

class AeonMaritimeEngine {
public:
    std::vector<MaritimeCargoShip> cargo_ships;
    std::vector<NavalBlockade> active_blockades;
    std::vector<PirateHaven> pirate_havens;
    std::vector<LetterOfMarque> privateers;
    int pirate_fleet_count = 3;

    AeonMaritimeEngine();
    void init_maritime(const AeonEngine& engine);
    void update_maritime_tick(AeonEngine& engine);
    void enact_naval_blockade(int attacker_civ_id, int target_civ_id, AeonEngine& engine);
    void issue_letter_of_marque(int patron_civ, int target_civ);
    void launch_anti_piracy_expedition(int haven_id, int attacking_civ_id, AeonEngine& engine);
};

} // namespace Aeon

#endif // AEON_MARITIME_H

