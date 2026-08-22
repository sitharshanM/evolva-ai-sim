#include "world.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <iostream>
#include <glm/gtc/constants.hpp>

// ── Constructor ───────────────────────────────────────────────────────────────
World::World() : rng(std::random_device{}()) {}

// ── init ──────────────────────────────────────────────────────────────────────
void World::init() {
    organisms.clear();
    food.clear();
    decree_log.clear();
    decree_visuals.clear();
    pending_births.clear();
    active_lineages_.clear();
    factions.clear();

    next_id        = 1;
    next_lineage_id = 1;
    sim_time       = 0.0f;
    food_accum     = 0.0f;

    food_spawn_mult        = 1.0f;
    global_metabolism_mul  = 1.0f;
    global_mutation_boost  = 1.0f;
    plague_active          = false;

    // Initialize initial Factions
    glm::vec3 colors[] = {
        {0.9f, 0.2f, 0.2f}, // Red
        {0.2f, 0.8f, 0.3f}, // Green
        {0.2f, 0.5f, 1.0f}, // Blue
        {0.9f, 0.8f, 0.2f}, // Yellow
        {0.8f, 0.3f, 0.9f}, // Purple
        {0.2f, 0.9f, 0.9f}  // Cyan
    };
    for (int i = 0; i < 6; ++i) {
        factions.push_back(Faction::create_random(i, "", colors[i]));
    }

    road_heat.assign(100 * 100, 0);
    resource_nodes.clear();
    farm_plots.clear();

    // Spawn initial resource nodes (Wood, Iron, Gold) across biomes
    for (int i = 0; i < 40; ++i) {
        ResourceNode rn;
        rn.pos = random_pos();
        rn.type = i % 3; // 0=Wood, 1=Iron, 2=Gold
        rn.amount = 300.0f;
        resource_nodes.push_back(rn);
    }

    // Initialize sub-modules
    world_layer.init();
    kingdoms.clear();
    rulers.clear();
    for (int i = 0; i < 6; ++i) {
        KingdomDossier kd;
        kd.id = i;
        kd.name = factions[i].name;
        kingdoms.push_back(kd);

        RulerAI rai;
        rai.faction_id = i;
        rai.ruler_name = factions[i].leader.name;
        rulers.push_back(rai);
    }

    // Spawn initial food
    spawn_random_food(Config::INITIAL_FOOD);

    // Spawn initial organisms
    organisms.reserve(Config::MAX_ORGANISMS + 64);
    for (int i = 0; i < Config::INITIAL_ORGANISMS; ++i)
        spawn_organism();
}

// ── tick ──────────────────────────────────────────────────────────────────────
void World::tick(float dt) {
    sim_time += dt;

    // Tick sub-modules
    world_layer.update(dt);
    economy_engine.update(dt);
    history_engine.tick(dt);
    for (auto& r : rulers) r.update_memory_decay(dt);

    // Tick diplomacy clock & god powers
    diplomacy.tick(dt);
    god_powers.update(dt);

    // ── Rebuild spatial hashes ────────────────────────────────────────────────
    rebuild_hashes();

    // ── Stone Wall collisions for organisms ───────────────────────────────────
    for (auto& org : organisms) {
        if (!org.alive) continue;
        for (const auto& wall : god_powers.stone_walls) {
            float d = torus_dist(org.pos, wall.pos);
            if (d < wall.radius + org.radius) {
                glm::vec2 push_dir = torus_dir(wall.pos, org.pos);
                org.pos = wrap(wall.pos + push_dir * (wall.radius + org.radius + 1.0f));
            }
        }
    }

    // ── Update all organisms ──────────────────────────────────────────────────
    for (auto& org : organisms) {
        if (org.alive)
            org.update(dt, *this, rng);
    }

    // ── Flush pending births ──────────────────────────────────────────────────
    flush_births();

    // ── Update Caravans & Factions ─────────────────────────────────────────────
    update_caravans(dt);
    update_factions();

    // ── Process decree effects ────────────────────────────────────────────────
    process_plague(dt);

    // ── Update Projectiles ────────────────────────────────────────────────────
    for (auto& p : projectiles) {
        p.pos = wrap(p.pos + p.vel * dt);
        p.lifetime -= dt;
        auto targets = org_hash.query(p.pos, 15.0f);
        for (int tid : targets) {
            if (tid < 0 || tid >= (int)organisms.size()) continue;
            Organism& victim = organisms[tid];
            if (victim.alive && !diplomacy.is_allied(p.faction_id, victim.faction_id)) {
                victim.energy -= p.damage;
                p.lifetime = 0.0f;
                break;
            }
        }
    }
    projectiles.erase(
        std::remove_if(projectiles.begin(), projectiles.end(),
            [](const Projectile& p) { return p.lifetime <= 0.0f; }),
        projectiles.end());

    // ── Outpost Counter-Fire Arrow Towers ─────────────────────────────────────
    static float outpost_fire_cooldown = 0.0f;
    outpost_fire_cooldown -= dt;
    if (outpost_fire_cooldown <= 0.0f) {
        outpost_fire_cooldown = 0.5f;
        for (const auto& f : factions) {
            for (const auto& op : f.outposts) {
                auto enemies = org_hash.query(op.pos, 120.0f);
                for (int eid : enemies) {
                    if (eid < 0 || eid >= (int)organisms.size()) continue;
                    Organism& enemy = organisms[eid];
                    if (enemy.alive && diplomacy.is_at_war(f.id, enemy.faction_id)) {
                        Projectile proj;
                        proj.pos = op.pos;
                        proj.vel = torus_dir(op.pos, enemy.pos) * 220.0f;
                        proj.faction_id = f.id;
                        proj.damage = 18.0f;
                        proj.lifetime = 1.2f;
                        projectiles.push_back(proj);
                        break;
                    }
                }
            }
        }
    }

    // ── Update Siege Engines (Catapults) ──────────────────────────────────────
    for (auto& se : siege_engines) {
        se.cooldown -= dt;
        if (se.cooldown <= 0.0f) {
            se.cooldown = 4.0f; // fire heavy boulder every 4s
            // Find nearest enemy capital or outpost
            for (const auto& f : factions) {
                if (f.id != se.faction_id && diplomacy.is_at_war(se.faction_id, f.id) && f.member_count > 0) {
                    float dist = torus_dist(se.pos, f.capital_pos);
                    if (dist < 400.0f) {
                        Projectile boulder;
                        boulder.pos = se.pos;
                        boulder.vel = torus_dir(se.pos, f.capital_pos) * 180.0f;
                        boulder.faction_id = se.faction_id;
                        boulder.damage = 45.0f; // Heavy Catapult Boulder!
                        boulder.lifetime = 2.5f;
                        projectiles.push_back(boulder);
                        break;
                    }
                }
            }
        }
    }

    // ── Update Natural Disasters (Volcanic Eruptions & Earthquakes) ───────────
    for (auto& dis : disasters) {
        dis.lifetime -= dt;
        if (dis.type == 0) { // Volcano: spawn rich Iron/Gold veins
            if (std::rand() % 100 < 5) {
                ResourceNode rn;
                rn.pos = wrap(dis.pos + glm::vec2((std::rand() % 100 - 50), (std::rand() % 100 - 50)));
                rn.type = (std::rand() % 2 == 0) ? 1 : 2; // Iron or Gold
                rn.amount = 400.0f;
                resource_nodes.push_back(rn);
            }
        }
    }
    disasters.erase(
        std::remove_if(disasters.begin(), disasters.end(),
            [](const DisasterEvent& d) { return d.lifetime <= 0.0f; }),
        disasters.end());

    // ── Update decree visuals ─────────────────────────────────────────────────
    for (auto& dv : decree_visuals) dv.lifetime -= dt;
    decree_visuals.erase(
        std::remove_if(decree_visuals.begin(), decree_visuals.end(),
            [](const DecreeVisual& d) { return d.lifetime <= 0.0f; }),
        decree_visuals.end());

    // ── Update War Banners ────────────────────────────────────────────────────
    for (auto& wb : war_banners) {
        wb.lifetime -= dt;
    }
    war_banners.erase(
        std::remove_if(war_banners.begin(), war_banners.end(),
            [](const WarBanner& wb) { return wb.lifetime <= 0.0f; }),
        war_banners.end());

    // ── Apply War Banner steer to attacking faction organisms ────────────────
    if (!war_banners.empty()) {
        for (auto& o : organisms) {
            if (!o.alive || o.faction_id < 0) continue;
            for (const auto& wb : war_banners) {
                if (wb.attacking_faction == o.faction_id) {
                    glm::vec2 dir = torus_dir(o.pos, wb.pos);
                    o.angle = o.angle * 0.95f + std::atan2(dir.x, dir.y) * 0.05f;
                    break;
                }
            }
        }
    }

    // ── Remove dead organisms ─────────────────────────────────────────────────
    organisms.erase(
        std::remove_if(organisms.begin(), organisms.end(),
            [](const Organism& o) { return !o.alive; }),
        organisms.end());

    // ── Remove dead food ──────────────────────────────────────────────────────
    food.erase(
        std::remove_if(food.begin(), food.end(),
            [](const Food& f) { return !f.alive; }),
        food.end());

    // ── Spawn food ────────────────────────────────────────────────────────────
    if ((int)food.size() < Config::MAX_FOOD) {
        float rate = Config::FOOD_SPAWN_RATE * food_spawn_mult;
        food_accum += rate * dt;
        int to_spawn = static_cast<int>(food_accum);
        food_accum  -= to_spawn;
        // Cluster spawning: some food spawns near existing food
        std::uniform_real_distribution<float> cluster_r(0.0f, 1.0f);
        for (int i = 0; i < to_spawn; ++i) {
            if (!food.empty() && cluster_r(rng) < Config::FOOD_CLUSTER_PROB) {
                std::uniform_int_distribution<int> pick(0, (int)food.size() - 1);
                glm::vec2 near = food[pick(rng)].pos;
                std::uniform_real_distribution<float> offset(-80.0f, 80.0f);
                spawn_food(wrap(near + glm::vec2(offset(rng), offset(rng))));
            } else {
                spawn_food(random_pos());
            }
        }
    }

    // ── Population floor: respawn organisms if too few ────────────────────────
    if ((int)organisms.size() < Config::MIN_ORGANISMS) {
        int deficit = Config::MIN_ORGANISMS - (int)organisms.size();
        for (int i = 0; i < deficit; ++i) spawn_organism();
    }

    // ── Update statistics (every ~0.5 sim-seconds) ────────────────────────────
    update_stats();
}

// ── rebuild_hashes ───────────────────────────────────────────────────────────
void World::rebuild_hashes() {
    org_hash.clear();
    food_hash.clear();
    for (int i = 0; i < (int)organisms.size(); ++i)
        if (organisms[i].alive)
            org_hash.insert(i, organisms[i].pos);
    for (int i = 0; i < (int)food.size(); ++i)
        if (food[i].alive)
            food_hash.insert(i, food[i].pos);
}

// ── flush_births ──────────────────────────────────────────────────────────────
void World::flush_births() {
    for (auto& child : pending_births) {
        if ((int)organisms.size() >= Config::MAX_ORGANISMS) break;
        child.pos = wrap(child.pos);
        active_lineages_.insert(child.lineage_id);
        ++stats.total_births;
        organisms.push_back(std::move(child));
    }
    pending_births.clear();
}

// ── process_plague ────────────────────────────────────────────────────────────
void World::process_plague(float dt) {
    if (!plague_active) return;
    plague_timer -= dt;
    if (plague_timer <= 0.0f) { plague_active = false; return; }

    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    for (auto& org : organisms) {
        if (!org.alive) continue;
        if (chance(rng) < 0.01f * dt) // small per-tick chance
            org.energy -= plague_strength * dt;
    }
}

// ── wrap / torus helpers ──────────────────────────────────────────────────────
glm::vec2 World::wrap(glm::vec2 p) const {
    p.x = std::fmod(p.x + width  * 10.0f, width);
    p.y = std::fmod(p.y + height * 10.0f, height);
    if (p.x < 0.0f) p.x += width;
    if (p.y < 0.0f) p.y += height;
    return p;
}

float World::torus_dist(glm::vec2 a, glm::vec2 b) const {
    float dx = std::abs(a.x - b.x);
    float dy = std::abs(a.y - b.y);
    if (dx > width  * 0.5f) dx = width  - dx;
    if (dy > height * 0.5f) dy = height - dy;
    return std::sqrt(dx * dx + dy * dy);
}

glm::vec2 World::torus_dir(glm::vec2 from, glm::vec2 to) const {
    float dx = to.x - from.x;
    float dy = to.y - from.y;
    if (std::abs(dx) > width  * 0.5f) dx -= std::copysign(width,  dx);
    if (std::abs(dy) > height * 0.5f) dy -= std::copysign(height, dy);
    float len = std::sqrt(dx * dx + dy * dy);
    return (len > 1e-6f) ? glm::vec2(dx / len, dy / len) : glm::vec2(0.0f, 1.0f);
}

glm::vec2 World::random_pos() {
    std::uniform_real_distribution<float> rx(0.0f, width);
    std::uniform_real_distribution<float> ry(0.0f, height);
    return {rx(rng), ry(rng)};
}

// ── spawn helpers ─────────────────────────────────────────────────────────────
void World::spawn_food(glm::vec2 pos, float energy) {
    if ((int)food.size() >= Config::MAX_FOOD) return;
    Food f;
    f.pos    = wrap(pos);
    f.energy = energy;
    f.radius = Config::FOOD_RADIUS * (0.8f + 0.4f * energy / Config::FOOD_ENERGY);
    f.alive  = true;
    food.push_back(f);
}

void World::spawn_random_food(int count) {
    for (int i = 0; i < count; ++i)
        spawn_food(random_pos());
}

Organism& World::spawn_organism(glm::vec2 pos, int lineage) {
    Organism org;
    org.id     = next_id++;
    org.energy = Config::INITIAL_ENERGY;
    org.pos    = (pos.x < 0) ? random_pos() : wrap(pos);
    std::uniform_real_distribution<float> ang(0.0f, glm::two_pi<float>());
    org.angle  = ang(rng);

    if (lineage < 0) {
        org.lineage_id  = next_lineage_id++;
    } else {
        org.lineage_id = lineage;
    }
    active_lineages_.insert(org.lineage_id);

    int num_fac = static_cast<int>(factions.size());
    if (num_fac > 0) {
        org.faction_id = org.lineage_id % num_fac;
        org.color = factions[org.faction_id].color;
    }

    org.dna = DNA::random(rng);
    org.init_from_dna();
    if (num_fac > 0) org.color = factions[org.faction_id].color; // set faction color
    organisms.push_back(std::move(org));
    return organisms.back();
}

// ── Neighbor queries ──────────────────────────────────────────────────────────
int World::nearest_food(glm::vec2 pos, float range) const {
    auto ids = food_hash.query(pos, range);
    float best = range + 1.0f;
    int   best_idx = -1;
    for (int fid : ids) {
        if (fid < 0 || fid >= (int)food.size() || !food[fid].alive) continue;
        float d = torus_dist(pos, food[fid].pos);
        if (d < best) { best = d; best_idx = fid; }
    }
    return best_idx;
}

std::vector<int> World::nearby_organisms(glm::vec2 pos, float range) const {
    return org_hash.query(pos, range);
}

// ── Statistics update ─────────────────────────────────────────────────────────
void World::update_stats() {
    stats.sim_time   = sim_time;
    stats.population = static_cast<int>(organisms.size());
    stats.food_count = static_cast<int>(food.size());

    if (organisms.empty()) { stats.record(); return; }

    float sum_energy = 0, sum_speed = 0, sum_vis = 0, sum_agg = 0, sum_herb = 0, sum_met = 0;
    int herb_count = 0;
    int gen_max    = 0;

    for (const auto& o : organisms) {
        if (!o.alive) continue;
        sum_energy += o.energy;
        sum_speed  += o.speed;
        sum_vis    += o.vision_range;
        sum_agg    += o.aggression;
        sum_herb   += o.herbivore;
        sum_met    += o.metabolism;
        if (o.herbivore > 0.55f) ++herb_count;
        gen_max = std::max(gen_max, o.generation);
    }

    float n = static_cast<float>(stats.population);
    stats.avg_energy     = sum_energy / n;
    stats.avg_speed      = sum_speed  / n;
    stats.avg_vision     = sum_vis    / n;
    stats.avg_aggression = sum_agg    / n;
    stats.avg_herbivore  = sum_herb   / n;
    stats.avg_metabolism = sum_met    / n;
    stats.generation_max = gen_max;
    stats.herbivore_ratio = static_cast<float>(herb_count) / n;
    stats.carnivore_ratio = 1.0f - stats.herbivore_ratio;
    stats.record();

    static float last_log = 0.0f;
    if (sim_time - last_log >= 3.0f) {
        last_log = sim_time;
        std::cout << "[STATUS " << (int)sim_time << "s] Pop: " << stats.population 
                  << " | Herb: " << (int)(stats.herbivore_ratio * 100) << "% | Carn: " << (int)(stats.carnivore_ratio * 100)
                  << "% | Food: " << food.size() << " | Decrees: " << decree_log.size() << std::endl;
    }
}

// ── apply_decree ──────────────────────────────────────────────────────────────
void World::apply_decree(const std::string& type, const DecreeParams& p, const std::string& desc) {
    // Log it
    DecreeLog log;
    log.decree_name = type;
    log.description = desc;
    log.sim_time    = sim_time;
    if (decree_log.size() > 50) decree_log.erase(decree_log.begin());
    decree_log.push_back(log);

    glm::vec2 center = (p.pos.x < 0) ? random_pos() : p.pos;
    float     radius = p.radius;

    // Decree visual
    DecreeVisual vis;
    vis.pos      = center;
    vis.radius   = radius;
    vis.lifetime = vis.max_life = 4.0f;

    if (type == "food_surge") {
        int count = static_cast<int>(p.value1 > 0 ? p.value1 : 300);
        for (int i = 0; i < count; ++i) {
            std::uniform_real_distribution<float> a(0.0f, glm::two_pi<float>());
            std::uniform_real_distribution<float> r(0.0f, radius);
            float ang = a(rng); float rd = r(rng);
            spawn_food(wrap(center + glm::vec2(std::cos(ang), std::sin(ang)) * rd),
                       Config::FOOD_ENERGY * 1.5f);
        }
        vis.color = {0.2f, 0.9f, 0.2f};
        food_spawn_mult = std::min(food_spawn_mult * 1.8f, 5.0f);
    }
    else if (type == "food_famine") {
        int removed = 0;
        for (auto& f : food) {
            if (!f.alive) continue;
            if (torus_dist(f.pos, center) < radius) { f.alive = false; ++removed; }
        }
        food_spawn_mult = std::max(food_spawn_mult * 0.3f, 0.1f);
        vis.color = {0.7f, 0.4f, 0.0f};
    }
    else if (type == "plague") {
        plague_active   = true;
        plague_timer    = p.duration > 0 ? p.duration : 20.0f;
        plague_strength = p.value1   > 0 ? p.value1   : 8.0f;
        vis.color = {0.6f, 0.0f, 0.8f};
        // Immediate hit
        for (auto& org : organisms)
            org.energy -= plague_strength * 0.5f;
    }
    else if (type == "population_cull") {
        // Kill the weakest N% of population
        float pct = p.value1 > 0 ? std::clamp(p.value1, 0.05f, 0.5f) : 0.2f;
        int kill_count = static_cast<int>(organisms.size() * pct);
        std::vector<int> sorted(organisms.size());
        std::iota(sorted.begin(), sorted.end(), 0);
        std::sort(sorted.begin(), sorted.end(), [&](int a, int b) {
            return organisms[a].energy < organisms[b].energy;
        });
        for (int i = 0; i < kill_count && i < (int)sorted.size(); ++i)
            organisms[sorted[i]].alive = false;
        vis.color = {0.9f, 0.1f, 0.1f};
    }
    else if (type == "genetic_drift_boost") {
        global_mutation_boost = p.value1 > 0 ? p.value1 : 3.0f;
        // Apply to existing organisms' mutation rate temporarily
        for (auto& org : organisms)
            org.mutation_rate = std::min(org.mutation_rate * global_mutation_boost, Config::MUTATION_RATE_MAX);
        vis.color = {0.2f, 0.6f, 1.0f};
    }
    else if (type == "predator_wave") {
        int count = static_cast<int>(p.value1 > 0 ? p.value1 : 30);
        for (int i = 0; i < count; ++i) {
            Organism& pred = spawn_organism(random_pos(), next_lineage_id++);
            // Force carnivore traits
            pred.dna.genes[5] = -3.0f;  // herbivore gene → carnivore
            pred.dna.genes[4] = 3.0f;   // aggression → max
            pred.dna.genes[0] = 2.0f;   // fast
            pred.dna.genes[1] = 2.5f;   // large
            pred.dna.express_traits();
            pred.init_from_dna();
        }
        vis.color = {1.0f, 0.2f, 0.0f};
    }
    else if (type == "resource_cluster") {
        // Dense food in one spot
        for (int i = 0; i < 500; ++i) {
            std::uniform_real_distribution<float> a(0.0f, glm::two_pi<float>());
            std::uniform_real_distribution<float> r(0.0f, radius * 0.5f);
            float ang = a(rng); float rd = r(rng);
            spawn_food(wrap(center + glm::vec2(std::cos(ang), std::sin(ang)) * rd));
        }
        vis.color = {0.0f, 1.0f, 0.5f};
    }
    else if (type == "temperature_shift") {
        float mult = p.value1 > 0 ? p.value1 : 1.5f;
        global_metabolism_mul = std::clamp(mult, 0.3f, 3.0f);
        vis.color = {1.0f, 0.5f, 0.0f};
    }
    else if (type == "storm") {
        // Apply random velocities to organisms in area
        std::uniform_real_distribution<float> kick(-300.0f, 300.0f);
        for (auto& org : organisms) {
            if (torus_dist(org.pos, center) < radius) {
                org.pos.x = std::fmod(org.pos.x + kick(rng) * 0.1f + width,  width);
                org.pos.y = std::fmod(org.pos.y + kick(rng) * 0.1f + height, height);
                org.energy -= 5.0f; // storm costs energy
            }
        }
        vis.color = {0.7f, 0.7f, 1.0f};
    }
    // Reset modifiers after time passes (will be handled in tick)
    decree_visuals.push_back(vis);
}

// ── update_factions ───────────────────────────────────────────────────────────
void World::update_factions() {
    if (factions.empty()) return;

    for (auto& f : factions) {
        f.member_count = 0;
        f.military_power = 0.0f;
        f.capital_pos = {0.0f, 0.0f};
    }

    int total_pop = 0;
    for (const auto& org : organisms) {
        if (!org.alive) continue;
        ++total_pop;
        int fid = org.faction_id;
        if (fid >= 0 && fid < (int)factions.size()) {
            factions[fid].member_count++;
            factions[fid].capital_pos += org.pos;
            factions[fid].military_power += (1.0f - org.herbivore) * org.radius * org.speed * 0.01f;
        }
    }

    for (auto& f : factions) {
        if (f.member_count > 0) {
            f.capital_pos /= static_cast<float>(f.member_count);
            f.territory_pct = total_pop > 0 ? (100.0f * f.member_count / total_pop) : 0.0f;
            f.border_radius = 180.0f + f.territory_pct * 5.0f + (float)f.outposts.size() * 40.0f + std::min(f.treasury_food * 0.2f, 200.0f);
            
            // Settlement tier progression
            if (f.member_count >= 150 && f.treasury_food >= 600.0f) f.tier = SettlementTier::METROPOLIS;
            else if (f.member_count >= 80 && f.treasury_food >= 300.0f) f.tier = SettlementTier::TOWN;
            else if (f.member_count >= 35 && f.treasury_food >= 100.0f) f.tier = SettlementTier::VILLAGE;
            else f.tier = SettlementTier::CAMPSITE;
        } else {
            f.territory_pct = 0.0f;
            f.border_radius = 100.0f;
            f.tier = SettlementTier::CAMPSITE;
        }
    }
}

Faction* World::get_faction(int id) {
    if (id >= 0 && id < (int)factions.size()) return &factions[id];
    return nullptr;
}

// ── apply_political_action ───────────────────────────────────────────────────
void World::apply_political_action(const PoliticalAction& act) {
    // Terminal stdout logging for clear POV
    std::cout << "\n========================================================" << std::endl;
    std::cout << "[SIM " << (int)sim_time << "s] 👑 GEOPOLITICAL ACTION: " << act.action_type << std::endl;
    std::cout << "  \"" << act.declaration << "\"" << std::endl;
    std::cout << "========================================================\n" << std::endl;

    // Log political action
    DecreeLog log;
    log.decree_name = act.action_type + ": " + act.declaration;
    log.description = act.declaration;
    log.sim_time    = sim_time;
    if (decree_log.size() > 50) decree_log.erase(decree_log.begin());
    decree_log.push_back(log);

    int fA = act.faction_a;
    int fB = act.faction_b;

    if (act.action_type == "DECLARE_WAR") {
        if (fA >= 0 && fB >= 0 && fA < (int)factions.size() && fB < (int)factions.size()) {
            diplomacy.set_status(fA, fB, DiplomaticStatus::WAR, act.treaty_name);
            diplomacy.add_grievance(fA, fB, act.declaration);
            factions[fB].leader.add_grievance(factions[fA].leader.name + " declared WAR on us!");
            SoundEngine::play_trumpet();
            chronicle.add_article(sim_time, act.declaration, "War has broken out between " + factions[fA].name + " and " + factions[fB].name + "!", "WAR");
            
            // Spawn War Banner at target capital
            WarBanner wb;
            wb.pos = factions[fB].capital_pos;
            wb.attacking_faction = fA;
            wb.target_faction    = fB;
            wb.lifetime = wb.max_life = 45.0f;
            war_banners.push_back(wb);

            DecreeVisual vis;
            vis.pos = factions[fA].capital_pos;
            vis.radius = 400.0f; vis.lifetime = vis.max_life = 5.0f;
            vis.color = {1.0f, 0.1f, 0.1f};
            decree_visuals.push_back(vis);
        }
    }
    else if (act.action_type == "FORM_ALLIANCE") {
        if (fA >= 0 && fB >= 0 && fA < (int)factions.size() && fB < (int)factions.size()) {
            diplomacy.set_status(fA, fB, DiplomaticStatus::ALLIANCE, act.treaty_name);
            SoundEngine::play_chime();
            chronicle.add_article(sim_time, act.declaration, "A grand alliance was forged under the " + act.treaty_name + ".", "ALLIANCE");
            DecreeVisual vis;
            vis.pos = factions[fA].capital_pos;
            vis.radius = 350.0f; vis.lifetime = vis.max_life = 4.0f;
            vis.color = {0.2f, 0.8f, 1.0f};
            decree_visuals.push_back(vis);

            // Spawn a merchant caravan between allied capitals
            spawn_merchant_caravan(fA, fB);
        }
    }
    else if (act.action_type == "PEACE_TREATY") {
        if (fA >= 0 && fB >= 0 && fA < (int)factions.size() && fB < (int)factions.size()) {
            diplomacy.set_status(fA, fB, DiplomaticStatus::PEACE, act.treaty_name);
            chronicle.add_article(sim_time, act.declaration, "Hostilities ended between nations.", "ALLIANCE");
        }
    }
    else if (act.action_type == "CIVIL_WAR") {
        if (fA >= 0 && fA < (int)factions.size()) {
            int new_fac_id = static_cast<int>(factions.size());
            if (new_fac_id < Config::MAX_FACTIONS) {
                Faction rebel = Faction::create_random(new_fac_id, factions[fA].name + " Rebels", {1.0f, 0.5f, 0.0f});
                factions.push_back(rebel);
                diplomacy.set_status(fA, new_fac_id, DiplomaticStatus::WAR, "Civil War Schism");
                factions[fA].leader.add_grievance("Traitorous rebels formed faction " + rebel.name);

                WarBanner wb;
                wb.pos = factions[fA].capital_pos;
                wb.attacking_faction = new_fac_id;
                wb.target_faction    = fA;
                wb.lifetime = wb.max_life = 45.0f;
                war_banners.push_back(wb);

                SoundEngine::play_trumpet();
                chronicle.add_article(sim_time, act.declaration, "Internal rebellion split the empire into two warring factions!", "WAR");

                int converted = 0;
                for (auto& org : organisms) {
                    if (org.alive && org.faction_id == fA && (converted % 2 == 0)) {
                        org.faction_id = new_fac_id;
                        org.color = rebel.color;
                        ++converted;
                    }
                }
            }
        }
    }
    else if (act.action_type == "ADOPT_GOVERNMENT") {
        if (fA >= 0 && fA < (int)factions.size()) {
            factions[fA].government = static_cast<GovernmentType>(std::rand() % 4);
            chronicle.add_article(sim_time, act.declaration, factions[fA].name + " adopted a new system of governance: " + government_type_str(factions[fA].government), "CIVILIZATION");
        }
    }
    else if (act.action_type == "UPGRADE_SETTLEMENT") {
        if (fA >= 0 && fA < (int)factions.size()) {
            factions[fA].treasury_food = std::max(0.0f, factions[fA].treasury_food - 200.0f);
            factions[fA].border_radius += 100.0f;
            SoundEngine::play_wall();
            chronicle.add_article(sim_time, act.declaration, factions[fA].name + " invested heavily in grand urban settlement infrastructure.", "CIVILIZATION");
        }
    }
    else if (act.action_type == "ESTABLISH_ROAD") {
        if (fA >= 0 && fB >= 0 && fA < (int)factions.size() && fB < (int)factions.size()) {
            SoundEngine::play_chime();
            chronicle.add_article(sim_time, act.declaration, factions[fA].name + " paved trade roads towards " + factions[fB].name + ".", "CIVILIZATION");
        }
    }
    else if (act.action_type == "GRANT_ASYLUM") {
        if (fA >= 0 && fA < (int)factions.size()) {
            factions[fA].treasury_food = std::max(0.0f, factions[fA].treasury_food - 50.0f);
            chronicle.add_article(sim_time, act.declaration, factions[fA].name + " granted humanitarian asylum to displaced war refugees.", "CIVILIZATION");
        }
    }
    else if (act.action_type == "TAX_HARVEST") {
        if (fA >= 0 && fA < (int)factions.size()) {
            factions[fA].tax_rate = 0.45f;
            chronicle.add_article(sim_time, act.declaration, factions[fA].name + " raised tax rates to 45% to fund national reserves.", "ECONOMY");
        }
    }
    else if (act.action_type == "BUILD_FORTRESS") {
        if (fA >= 0 && fA < (int)factions.size()) {
            factions[fA].treasury_food = std::max(0.0f, factions[fA].treasury_food - 150.0f);
            spawn_stone_wall(factions[fA].capital_pos, 80.0f);
            Outpost op;
            op.pos = wrap(factions[fA].capital_pos + glm::vec2(120.0f, 0.0f));
            factions[fA].outposts.push_back(op);
            SoundEngine::play_wall();
            chronicle.add_article(sim_time, act.declaration, factions[fA].name + " commissioned a fortress wall and frontier outpost around their capital.", "ECONOMY");
        }
    }
    else if (act.action_type == "EXPAND_BORDERS") {
        if (fA >= 0 && fA < (int)factions.size()) {
            factions[fA].treasury_food = std::max(0.0f, factions[fA].treasury_food - 100.0f);
            factions[fA].border_radius += 80.0f;
            chronicle.add_article(sim_time, act.declaration, factions[fA].name + " expanded their sovereign territorial borders.", "ECONOMY");
        }
    }
    else if (act.action_type == "SUBSIDIZE_GROWTH") {
        if (fA >= 0 && fA < (int)factions.size()) {
            float dist_amount = std::min(factions[fA].treasury_food, 150.0f);
            factions[fA].treasury_food -= dist_amount;
            int members = factions[fA].member_count;
            if (members > 0) {
                float per_head = dist_amount / static_cast<float>(members);
                for (auto& o : organisms) {
                    if (o.alive && o.faction_id == fA) {
                        o.energy = std::min(Config::MAX_ENERGY, o.energy + per_head);
                    }
                }
            }
            SoundEngine::play_genesis();
            chronicle.add_article(sim_time, act.declaration, factions[fA].name + " distributed grain subsidies to spur economic growth.", "ECONOMY");
        }
    }
    else if (act.action_type == "HIRE_MERCENARIES") {
        if (fA >= 0 && fA < (int)factions.size()) {
            factions[fA].treasury_food = std::max(0.0f, factions[fA].treasury_food - 120.0f);
            for (int i = 0; i < 3; ++i) {
                auto& merc = spawn_organism(wrap(factions[fA].capital_pos + glm::vec2(i * 30.0f, 0.0f)));
                merc.faction_id = fA;
                merc.color = factions[fA].color;
                merc.herbivore = 0.1f; // aggressive carnivore mercenary
                merc.speed *= 1.3f;
                merc.citizen.profession = "Mercenary Vanguard";
            }
            SoundEngine::play_trumpet();
            chronicle.add_article(sim_time, act.declaration, factions[fA].name + " hired elite carnivore mercenaries to defend the realm.", "WAR");
        }
    }
}

// ── God Powers & Miracle Wand ─────────────────────────────────────────────────
void World::execute_smite(glm::vec2 pos, float radius) {
    SoundEngine::play_smite();
    SmiteEffect eff;
    eff.pos    = pos;
    eff.radius = radius;
    god_powers.smite_effects.push_back(eff);

    int struck = 0;
    for (auto& org : organisms) {
        if (!org.alive) continue;
        if (torus_dist(org.pos, pos) <= radius) {
            org.energy -= 75.0f; // Massive smite damage!
            ++struck;
        }
    }
    chronicle.add_article(sim_time, "DIVINE SMITE STRIKES THE EARTH!",
                          "Lightning rained down from the heavens, striking " + std::to_string(struck) + " citizens!", "DECREE");
}

void World::spawn_stone_wall(glm::vec2 pos, float radius) {
    SoundEngine::play_wall();
    StoneWall w;
    w.pos    = wrap(pos);
    w.radius = radius;
    god_powers.stone_walls.push_back(w);
}

void World::paint_genesis_food(glm::vec2 pos, int count) {
    SoundEngine::play_genesis();
    std::uniform_real_distribution<float> angle_dist(0.0f, glm::two_pi<float>());
    std::uniform_real_distribution<float> r_dist(0.0f, 60.0f);
    for (int i = 0; i < count; ++i) {
        float a = angle_dist(rng);
        float r = r_dist(rng);
        spawn_food(wrap(pos + glm::vec2(std::cos(a), std::sin(a)) * r), Config::FOOD_ENERGY * 1.2f);
    }
}

// ── Merchant Caravans ─────────────────────────────────────────────────────────
void World::spawn_merchant_caravan(int home_fac, int dest_fac) {
    if (home_fac < 0 || home_fac >= (int)factions.size()) return;
    if (dest_fac < 0 || dest_fac >= (int)factions.size()) return;

    MerchantCaravan c;
    c.id                     = next_id++;
    c.faction_id             = home_fac;
    c.destination_faction_id = dest_fac;
    c.pos                    = factions[home_fac].capital_pos;
    c.target_pos             = factions[dest_fac].capital_pos;
    c.energy_cargo           = 200.0f;
    caravans.push_back(c);
}

void World::update_caravans(float dt) {
    for (auto& c : caravans) {
        if (!c.alive) continue;

        // Move caravan towards target capital
        float d = torus_dist(c.pos, c.target_pos);
        if (d < 15.0f) {
            // Arrived at capital!
            if (!c.returning) {
                // Deposit cargo at destination capital
                factions[c.destination_faction_id].treasury_food += c.energy_cargo;
                c.returning = true;
                c.target_pos = factions[c.faction_id].capital_pos;
            } else {
                // Returned home
                c.alive = false; // completed journey
            }
        } else {
            glm::vec2 dir = torus_dir(c.pos, c.target_pos);
            c.pos = wrap(c.pos + dir * c.speed * dt);
        }

        // Check if warring enemy organisms attack the caravan
        auto nearby = nearby_organisms(c.pos, 30.0f);
        for (int oid : nearby) {
            if (oid < 0 || oid >= (int)organisms.size()) continue;
            Organism& o = organisms[oid];
            if (!o.alive) continue;
            if (diplomacy.is_at_war(o.faction_id, c.faction_id)) {
                // Raid caravan!
                c.alive = false;
                SoundEngine::play_clash();
                spawn_food(c.pos, Config::FOOD_ENERGY * 3.0f);
                chronicle.add_article(sim_time, "MERCHANT CARAVAN RAIDED!",
                    factions[o.faction_id].name + " warriors ambushed a trade caravan from " + factions[c.faction_id].name + "!", "WAR");
                break;
            }
        }
    }

    caravans.erase(
        std::remove_if(caravans.begin(), caravans.end(),
            [](const MerchantCaravan& c) { return !c.alive; }),
        caravans.end());
}


