#ifndef AEON_INDIVIDUAL_CITIZENS_H
#define AEON_INDIVIDUAL_CITIZENS_H

#include <string>
#include <vector>

namespace Aeon {

class AeonEngine;

enum class CitizenRole {
    GENERAL,
    GRAND_MERCHANT,
    CHIEF_SCIENTIST,
    IMPERIAL_SPY,
    GOVERNOR,
    HIGH_CLERIC,
    MASTER_ARTISAN
};

struct IndividualCitizen {
    int id = 0;
    int civ_id = 0;
    std::string name;
    CitizenRole role = CitizenRole::GENERAL;
    int age = 28;
    float health = 100.0f;
    double wealth_gold = 1000.0;
    float loyalty = 80.0f;       // 0..100
    float ambition = 50.0f;      // 0..100
    float influence = 40.0f;     // 0..100
    int map_x = 0;
    int map_y = 0;
    std::vector<std::string> journal_log;
    bool is_alive = true;

    std::string spouse_name;
    std::string rival_name;
    int artifacts_created = 0;
    bool has_active_coup_plot = false;
    int coup_plot_stage = 0; // 0=None, 1=Inception, 2=Arming, 3=Strike

    std::string get_role_string() const;
};

class AeonCitizenEngine {
public:
    std::vector<IndividualCitizen> citizens;
    int next_citizen_id = 100;

    AeonCitizenEngine();
    void init_citizens(const AeonEngine& engine);
    void update_citizens_tick(AeonEngine& engine);

    void promote_citizen(int citizen_id, CitizenRole new_role, AeonEngine& engine);
    void exile_citizen(int citizen_id, AeonEngine& engine);
    void assassinate_citizen(int citizen_id, AeonEngine& engine);
    void trigger_coup(int citizen_id, AeonEngine& engine);
    void progress_coup_plot(IndividualCitizen& citizen, AeonEngine& engine);
    void check_oligarch_monopolies(AeonEngine& engine);
    void create_artisan_artifact(int citizen_id, AeonEngine& engine);
    void spawn_replacement_citizen(int civ_id, AeonEngine& engine);
};

} // namespace Aeon

#endif // AEON_INDIVIDUAL_CITIZENS_H

