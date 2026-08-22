#ifndef AEON_NUCLEAR_H
#define AEON_NUCLEAR_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

struct TriadNuclearState {
    int civ_id = 0;
    std::string civ_name;
    int icbm_silos = 0;
    int stealth_bombers = 0;
    int ssbn_submarines = 0;
    int warheads_total = 0;
    int interceptor_batteries = 2; // Anti-ballistic missile batteries
    bool triad_complete = false;
};

class AeonNuclearEngine {
public:
    std::vector<TriadNuclearState> nuclear_arsenals;
    bool global_nuclear_winter = false;
    int nuclear_winter_years_left = 0;

    AeonNuclearEngine();
    void init_arsenals(const AeonEngine& engine);
    void update_nuclear_tick(AeonEngine& engine);
    void launch_nuclear_strike(int attacker_civ_id, int target_civ_id, AeonEngine& engine);
    void execute_mad_retaliation(int defender_civ_id, int original_attacker_id, AeonEngine& engine);
    const TriadNuclearState* get_arsenal(int civ_id) const;
};

} // namespace Aeon

#endif // AEON_NUCLEAR_H

