#include "organism.h"
#include "world.h"
#include <cmath>
#include <algorithm>
#include <glm/gtc/constants.hpp>

// ── HSV → RGB helper ─────────────────────────────────────────────────────────
static glm::vec3 hsv_to_rgb(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
    float m = v - c;
    glm::vec3 rgb;
    int hi = static_cast<int>(h * 6.0f) % 6;
    if      (hi == 0) rgb = {c, x, 0};
    else if (hi == 1) rgb = {x, c, 0};
    else if (hi == 2) rgb = {0, c, x};
    else if (hi == 3) rgb = {0, x, c};
    else if (hi == 4) rgb = {x, 0, c};
    else              rgb = {c, 0, x};
    return rgb + glm::vec3(m);
}

// ── Lineage → color using golden ratio hashing ───────────────────────────────
static glm::vec3 lineage_color(int lineage_id, float herbivore_ratio) {
    const float golden = 0.6180339887f;
    float hue = std::fmod(lineage_id * golden, 1.0f);
    float sat = 0.55f + 0.4f * herbivore_ratio;        // herbivores more saturated
    float val = 0.85f - 0.2f * (1.0f - herbivore_ratio); // carnivores slightly darker
    return hsv_to_rgb(hue, sat, val);
}

// ── init_from_dna ─────────────────────────────────────────────────────────────
void Organism::init_from_dna() {
    dna.express_traits();
    
    region = RegionInfo::get_region_for_pos(pos);
    speed        = dna.speed * region.speed_mod;
    radius       = dna.radius;
    vision_range = dna.vision_range;
    metabolism   = dna.metabolism;
    aggression   = dna.aggression * region.aggression_mod;
    herbivore    = dna.herbivore;
    mutation_rate = dna.mutation_rate;

    citizen = CitizenProfile::generate_random(id, faction_id, region);

    nn.load_from_genes(dna.nn_weights(), dna.nn_weight_count());
    nn.reset_memory();

    color = lineage_color(lineage_id, herbivore);
}

// ── build_inputs ─────────────────────────────────────────────────────────────
std::vector<float> Organism::build_inputs(const World& world) const {
    std::vector<float> inp(Config::NN_INPUT_SIZE, 0.0f);

    // ── Find nearest food ─────────────────────────────────────────────────────
    float best_food_dist = vision_range + 1.0f;
    glm::vec2 food_dir(0.0f, 1.0f);
    float food_energy_norm = 0.0f;

    auto food_ids = world.food_hash.query(pos, vision_range);
    for (int fid : food_ids) {
        if (fid < 0 || fid >= (int)world.food.size()) continue;
        const Food& f = world.food[fid];
        if (!f.alive) continue;
        float d = world.torus_dist(pos, f.pos);
        if (d < best_food_dist) {
            best_food_dist = d;
            food_dir = world.torus_dir(pos, f.pos);
            food_energy_norm = f.energy / Config::FOOD_ENERGY;
        }
    }

    // ── Find nearest enemy (larger organism of different lineage or pure carnivore) ──
    float best_enemy_dist = vision_range + 1.0f;
    glm::vec2 enemy_dir(0.0f, -1.0f);
    float enemy_size_norm = 0.0f;

    // ── Find nearest ally (same lineage) ─────────────────────────────────────
    float best_ally_dist = vision_range + 1.0f;
    glm::vec2 ally_dir(0.0f, 1.0f);

    int num_nearby_food = (int)food_ids.size();
    auto org_ids = world.nearby_organisms(pos, vision_range);
    int num_nearby_orgs = 0;

    for (int oid : org_ids) {
        if (oid < 0 || oid >= (int)world.organisms.size()) continue;
        const Organism& o = world.organisms[oid];
        if (!o.alive || o.id == id) continue;

        float d = world.torus_dist(pos, o.pos);
        if (d > vision_range) continue;
        ++num_nearby_orgs;

        glm::vec2 dir = world.torus_dir(pos, o.pos);

        // Enemy: different lineage/faction AND (at war OR bigger than us OR carnivorous)
        bool at_war = world.diplomacy.is_at_war(faction_id, o.faction_id);
        bool allied = world.diplomacy.is_allied(faction_id, o.faction_id);

        bool is_threat = !allied && (at_war || o.lineage_id != lineage_id) &&
                         (at_war || o.radius > radius * 0.85f || o.herbivore < 0.3f);
        if (is_threat && d < best_enemy_dist) {
            best_enemy_dist = d;
            enemy_dir = dir;
            enemy_size_norm = o.radius / Config::ORG_MAX_RADIUS;
        }

        // Ally: same lineage or allied faction
        if (allied && d < best_ally_dist) {
            best_ally_dist = d;
            ally_dir = dir;
        }
    }

    // ── Pack into input vector ────────────────────────────────────────────────
    float food_dist_norm  = (best_food_dist  <= vision_range) ? (1.0f - best_food_dist  / vision_range) : 0.0f;
    float enemy_dist_norm = (best_enemy_dist <= vision_range) ? (1.0f - best_enemy_dist / vision_range) : 0.0f;
    float ally_dist_norm  = (best_ally_dist  <= vision_range) ? (1.0f - best_ally_dist  / vision_range) : 0.0f;

    inp[0]  = food_dir.x;
    inp[1]  = food_dir.y;
    inp[2]  = food_dist_norm;
    inp[3]  = food_energy_norm;
    inp[4]  = enemy_dir.x;
    inp[5]  = enemy_dir.y;
    inp[6]  = enemy_dist_norm;
    inp[7]  = enemy_size_norm;
    inp[8]  = ally_dir.x;
    inp[9]  = ally_dir.y;
    inp[10] = ally_dist_norm;
    inp[11] = hunger;
    inp[12] = energy / Config::MAX_ENERGY;
    inp[13] = age / Config::MAX_AGE;
    inp[14] = std::min(1.0f, static_cast<float>(num_nearby_food) / 20.0f);
    inp[15] = std::min(1.0f, static_cast<float>(num_nearby_orgs) / 15.0f);

    return inp;
}

// ── apply_outputs ─────────────────────────────────────────────────────────────
void Organism::apply_outputs(const std::vector<float>& out, float dt,
                              World& world, std::mt19937& rng)
{
    // out[0] = forward speed factor   [-1, 1]
    // out[1] = turn rate factor       [-1, 1]
    // out[2] = attack intention       tanh; >0.5 → attack
    // out[3] = reproduce intention    tanh; >0.5 → reproduce

    float fwd   = out[0];                      // forward/backward
    float turn  = out[1] * Config::TURN_SPEED_MAX; // radians/sec

    // Road heat tracking & paved road speed boost (+40%)
    int gx = std::clamp(static_cast<int>(pos.x / Config::WORLD_WIDTH * 100.0f), 0, 99);
    int gy = std::clamp(static_cast<int>(pos.y / Config::WORLD_HEIGHT * 100.0f), 0, 99);
    int cell = gy * 100 + gx;
    float road_spd_mult = 1.0f;
    if (cell >= 0 && cell < (int)world.road_heat.size()) {
        if (world.road_heat[cell] > 40) road_spd_mult = 1.4f; // Paved Road speed boost
        if (world.road_heat[cell] < 250) world.road_heat[cell] += 1;
    }

    float move_speed = speed * fwd * road_spd_mult;
    pos.x += std::sin(angle) * move_speed * dt;
    pos.y += std::cos(angle) * move_speed * dt;
    pos = world.wrap(pos);

    // ── Mine Resource Nodes (Wood, Iron, Gold) ────────────────────────────────
    for (auto& rn : world.resource_nodes) {
        if (rn.amount <= 0.0f) continue;
        if (world.torus_dist(pos, rn.pos) < radius + 15.0f) {
            float yield = 8.0f * dt;
            rn.amount -= yield;
            if (faction_id >= 0 && faction_id < (int)world.factions.size()) {
                if (rn.type == 0) world.factions[faction_id].resource_wood += yield;
                else if (rn.type == 1) world.factions[faction_id].resource_iron += yield;
                else if (rn.type == 2) world.factions[faction_id].resource_gold += yield;
            }
        }
    }

    // ── Eat food (passive: gobble any food we're touching) ────────────────────
    auto food_ids = world.food_hash.query(pos, radius + Config::FOOD_RADIUS + 2.0f);
    for (int fid : food_ids) {
        if (fid < 0 || fid >= (int)world.food.size()) continue;
        Food& f = world.food[fid];
        if (!f.alive) continue;
        float d = world.torus_dist(pos, f.pos);
        if (d < radius + f.radius) {
            float gain = f.energy * (0.5f + 0.5f * herbivore); // herbivores more efficient
            if (faction_id >= 0 && faction_id < (int)world.factions.size()) {
                float tax_rate = world.factions[faction_id].tax_rate;
                float tax = gain * tax_rate;
                gain -= tax;
                world.factions[faction_id].treasury_food += tax;
            }
            energy = std::min(Config::MAX_ENERGY, energy + gain);
            lifetime_food_eaten += gain;
            f.alive = false;
        }
    }

    // ── Attack ────────────────────────────────────────────────────────────────
    bool wants_attack = (out[2] > 0.3f) && (aggression > 0.3f) && (attack_cooldown <= 0.0f);
    if (wants_attack) {
        auto org_ids = world.nearby_organisms(pos, Config::ATTACK_RANGE + radius);
        float best_d = 1e9f;
        int   target = -1;
        for (int oid : org_ids) {
            if (oid < 0 || oid >= (int)world.organisms.size()) continue;
            Organism& o = world.organisms[oid];
            if (!o.alive || o.id == id) continue;
            if (world.diplomacy.is_allied(faction_id, o.faction_id)) continue; // Don't attack allies!

            float d = world.torus_dist(pos, o.pos);
            if (d < radius + o.radius + Config::ATTACK_RANGE && d < best_d) {
                best_d = d;
                target = oid;
            }
        }
        if (target >= 0) {
            Organism& victim = world.organisms[target];
            float dmg = Config::ATTACK_DAMAGE * (1.0f - herbivore); // carnivores hit harder
            if (world.diplomacy.is_at_war(faction_id, victim.faction_id)) {
                dmg *= Config::WAR_FERVOR_BUFF; // War fervor bonus
            }

            // ── Flanking Backstab Critical Bonus (+50%) ──────────────────────
            glm::vec2 attack_dir = world.torus_dir(pos, victim.pos);
            float victim_heading = victim.angle;
            float attack_angle   = std::atan2(attack_dir.x, attack_dir.y);
            float angle_diff     = std::abs(victim_heading - attack_angle);
            if (angle_diff > 2.0f) {
                dmg *= 1.50f; // Critical Backstab!
            }

            // ── Phalanx Shield Wall Reduction (-30%) ──────────────────────────
            auto victim_allies = world.nearby_organisms(victim.pos, 45.0f);
            int shield_count = 0;
            for (int a_id : victim_allies) {
                if (a_id >= 0 && a_id < (int)world.organisms.size()) {
                    if (world.organisms[a_id].alive && world.organisms[a_id].faction_id == victim.faction_id) {
                        ++shield_count;
                    }
                }
            }
            if (shield_count >= 3) {
                dmg *= 0.70f; // Shield wall absorbs 30% damage
            }

            victim.energy -= dmg;
            victim.last_attacker_id = id;
            if (victim.energy <= 0.0f) {
                citizen.kills++;
            }
            energy = std::min(Config::MAX_ENERGY, energy + dmg * 0.4f); // gain partial energy
            attack_cooldown = Config::ATTACK_COOLDOWN;
            energy -= Config::ATTACK_ENERGY_COST;
        }
    }

    // ── Kin Selection & Altruistic Food Sharing ──────────────────────────────
    if (energy > Config::MAX_ENERGY * 0.80f) {
        auto allies = world.nearby_organisms(pos, 40.0f);
        for (int a_id : allies) {
            if (a_id < 0 || a_id >= (int)world.organisms.size()) continue;
            Organism& ally = world.organisms[a_id];
            if (ally.alive && ally.id != id && ally.lineage_id == lineage_id && ally.energy < Config::MAX_ENERGY * 0.30f) {
                float gift = 15.0f * dt;
                energy -= gift;
                ally.energy += gift;
                break;
            }
        }
    }

    // ── Reproduce ─────────────────────────────────────────────────────────────
    bool wants_repro = (out[3] > 0.4f) && can_reproduce();
    if (wants_repro) {
        // Try sexual reproduction with nearest willing ally
        auto org_ids = world.nearby_organisms(pos, vision_range * 0.5f);
        bool did_sexual = false;
        for (int oid : org_ids) {
            if (oid < 0 || oid >= (int)world.organisms.size()) continue;
            Organism& partner = world.organisms[oid];
            if (!partner.alive || partner.id == id) continue;
            if (partner.lineage_id != lineage_id) continue;
            if (!partner.can_reproduce()) continue;
            float d = world.torus_dist(pos, partner.pos);
            if (d < radius + partner.radius + 10.0f) {
                Organism child = make_offspring_with(partner, rng, world.next_id++);
                energy -= Config::REPRODUCE_ENERGY_COST * 0.5f;
                partner.energy -= Config::REPRODUCE_ENERGY_COST * 0.5f;
                reproduce_cooldown = Config::REPRODUCE_COOLDOWN;
                partner.reproduce_cooldown = Config::REPRODUCE_COOLDOWN;
                ++children_count;
                world.pending_births.push_back(std::move(child));
                did_sexual = true;
                break;
            }
        }
        // Asexual budding if no partner found
        if (!did_sexual && energy >= Config::REPRODUCE_ENERGY_THRESHOLD + 5.0f) {
            Organism child = make_offspring(rng, world.next_id++);
            energy -= Config::REPRODUCE_ENERGY_COST;
            reproduce_cooldown = Config::REPRODUCE_COOLDOWN;
            ++children_count;
            world.pending_births.push_back(std::move(child));
        }
    }
}

// ── update ────────────────────────────────────────────────────────────────────
void Organism::update(float dt, World& world, std::mt19937& rng) {
    if (!alive) return;

    // Age and metabolic cost
    age    += dt;
    energy -= metabolism * dt * world.global_metabolism_mul;
    hunger  = 1.0f - std::clamp(energy / Config::MAX_ENERGY, 0.0f, 1.0f);

    // Calculate Cognitive Drive
    if (energy < Config::MAX_ENERGY * 0.35f) {
        current_drive = CognitiveDrive::SURVIVAL;
    } else if (last_attacker_id != 0 || !world.war_banners.empty()) {
        current_drive = CognitiveDrive::DEFENSE;
    } else if (can_reproduce()) {
        current_drive = CognitiveDrive::REPRODUCTION;
    } else {
        current_drive = CognitiveDrive::LABOR;
    }

    // Decay cooldowns
    attack_cooldown    = std::max(0.0f, attack_cooldown    - dt);
    reproduce_cooldown = std::max(0.0f, reproduce_cooldown - dt);

    // Death
    if (energy <= 0.0f || age >= Config::MAX_AGE) {
        alive = false;
        if (energy > 2.0f)
            world.spawn_food(pos, energy * Config::CORPSE_ENERGY);
        return;
    }

    // Neural network decision-making
    auto inputs  = build_inputs(world);
    auto outputs = nn.forward(inputs);
    apply_outputs(outputs, dt, world, rng);
}

// ── make_offspring ────────────────────────────────────────────────────────────
Organism Organism::make_offspring(std::mt19937& rng, uint64_t next_id) const {
    Organism child;
    child.id        = next_id;
    child.parent_id = id;
    child.lineage_id = lineage_id;
    child.faction_id = faction_id;
    child.generation = generation + 1;
    child.dna        = dna.mutate(rng);

    // Apply global mutation boost
    // (handled by World when creating offspring)
    std::uniform_real_distribution<float> angle_dist(0.0f, glm::two_pi<float>());
    float offset_dist = radius + 5.0f;
    float off_angle   = angle_dist(rng);
    child.pos = pos + glm::vec2(std::cos(off_angle), std::sin(off_angle)) * offset_dist;
    child.angle  = angle_dist(rng);
    child.energy = Config::INITIAL_ENERGY * 0.7f;
    child.init_from_dna();
    return child;
}

Organism Organism::make_offspring_with(const Organism& partner,
                                        std::mt19937& rng, uint64_t next_id) const {
    Organism child;
    child.id        = next_id;
    child.parent_id = id;
    child.lineage_id = lineage_id;
    child.faction_id = faction_id;
    child.generation = std::max(generation, partner.generation) + 1;
    child.dna        = DNA::crossover(dna, partner.dna, rng).mutate(rng);

    std::uniform_real_distribution<float> angle_dist(0.0f, glm::two_pi<float>());
    float off_angle = angle_dist(rng);
    float offset    = (radius + partner.radius) * 0.5f + 5.0f;
    child.pos = (pos + partner.pos) * 0.5f
              + glm::vec2(std::cos(off_angle), std::sin(off_angle)) * offset;
    child.angle  = angle_dist(rng);
    child.energy = Config::INITIAL_ENERGY * 0.7f;
    child.init_from_dna();
    return child;
}

Organism Organism::create_custom_organism(uint64_t id, glm::vec2 pos, float speed, float vision, float aggression, float herbivore, glm::vec3 color) {
    Organism org;
    org.id = id;
    org.pos = pos;
    org.speed = speed;
    org.vision_range = vision;
    org.aggression = aggression;
    org.herbivore = herbivore;
    org.color = color;
    org.energy = 100.0f;
    org.dna.speed = speed;
    org.dna.vision_range = vision;
    org.dna.aggression = aggression;
    org.dna.herbivore = herbivore;
    org.nn.load_from_genes(org.dna.nn_weights(), org.dna.nn_weight_count());
    return org;
}
