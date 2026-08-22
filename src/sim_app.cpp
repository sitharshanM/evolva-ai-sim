#include "sim_app.h"
#include "config.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <chrono>
#include <algorithm>

#include <iostream>
#include <thread>

SimApp::~SimApp() { shutdown(); }

// ── init ──────────────────────────────────────────────────────────────────────
bool SimApp::init(bool headless) {
    if (!headless) {
        if (!renderer_.init("Digital Life Simulator  ·  Ollama World God",
                            Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT))
            return false;
    }

    world_.init();
    ollama_.start(Config::OLLAMA_MODEL);
    god_timer_ = Config::GOD_DECREE_INTERVAL;

    return true;
}

// ── run_headless ──────────────────────────────────────────────────────────────
void SimApp::run_headless() {
    using clock = std::chrono::high_resolution_clock;
    auto last_time = clock::now();

    std::cout << "\n========================================================" << std::endl;
    std::cout << "🌍 PURE HEADLESS TERMINAL AI WORLD SIMULATOR ACTIVE" << std::endl;
    std::cout << "========================================================\n" << std::endl;

    while (true) {
        auto now = clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;
        dt = std::min(dt, 0.05f);

        world_.tick(dt);
        handle_ollama(dt);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

// ── run ───────────────────────────────────────────────────────────────────────
void SimApp::run() {
    using clock = std::chrono::high_resolution_clock;
    auto last_time = clock::now();

    while (!renderer_.should_close()) {
        auto now = clock::now();
        float dt = std::chrono::duration<float>(now - last_time).count();
        last_time = now;
        dt = std::min(dt, 0.05f); // cap at 50ms to avoid spiral-of-death

        // FPS (smoothed)
        fps_ = fps_ * 0.9f + (dt > 0.0001f ? 0.1f / dt : 60.0f);

        // ── Simulation ticks ──────────────────────────────────────────────────
        if (!paused_) {
            for (int i = 0; i < speed_mult_; ++i)
                world_.tick(dt / speed_mult_);
        }

        handle_ollama(dt);
        handle_input(dt);

        // Decay narrator toast
        if (narrator_timer_ > 0.0f)
            narrator_timer_ = std::max(0.0f, narrator_timer_ - dt);

        // ── Render ────────────────────────────────────────────────────────────
        renderer_.begin_frame();
        renderer_.draw_world(world_, selected_id_);
        renderer_.draw_ui(world_, selected_id_, speed_mult_, paused_,
                          fps_, ollama_status_, narrator_text_);
        renderer_.end_frame();
    }
}

// ── handle_ollama ─────────────────────────────────────────────────────────────
void SimApp::handle_ollama(float dt) {
    ollama_status_ = ollama_.is_busy() ? ollama_.status() : "Idle";

    // Countdown to next decree
    if (!paused_) {
        god_timer_ -= dt * speed_mult_;
        if (god_timer_ <= 0.0f && !ollama_.is_busy() && !world_.factions.empty()) {
            current_leader_idx_ = current_leader_idx_ % static_cast<int>(world_.factions.size());
            const auto& active_leader = world_.factions[current_leader_idx_].leader;
            ollama_.request_decree(world_.stats, world_.sim_time, active_leader);
            current_leader_idx_ = (current_leader_idx_ + 1) % static_cast<int>(world_.factions.size());
            god_timer_ = Config::GOD_DECREE_INTERVAL;
        }
    }

    // Apply ready decree
    if (auto decree = ollama_.poll_decree()) {
        world_.apply_decree(decree->type, decree->params, decree->description);
        
        // Update Dynasty Leader speech quote
        if (decree->leader_faction_id >= 0 && decree->leader_faction_id < (int)world_.factions.size()) {
            if (!decree->speech.empty())
                world_.factions[decree->leader_faction_id].leader.current_quote = decree->speech;
        }

        std::string event = decree->leader_name + ": " + decree->type + " — " + decree->description;

        if (decree->has_political) {
            PoliticalAction act;
            act.action_type = decree->pol_action_type;
            act.faction_a   = decree->faction_a;
            act.faction_b   = decree->faction_b;
            act.treaty_name = decree->treaty_name;
            act.declaration = decree->declaration;
            world_.apply_political_action(act);
            event += " | Political Event: " + act.action_type + " (" + act.declaration + ")";

            // Trigger SPONTANEOUS EMERGENCY RETALIATION for target leader!
            if (act.action_type == "DECLARE_WAR" && act.faction_b >= 0 && act.faction_b < (int)world_.factions.size()) {
                const auto& target_leader = world_.factions[act.faction_b].leader;
                std::string attacker_name = (act.faction_a >= 0 && act.faction_a < (int)world_.factions.size())
                    ? world_.factions[act.faction_a].leader.name : "Enemy Ruler";
                ollama_.request_emergency_decree(world_.stats, world_.sim_time, target_leader,
                    "Rival " + attacker_name + " declared war on your empire under treaty: " + act.treaty_name + "!", attacker_name);
            }
        }

        // Request narrator commentary
        ollama_.request_narrator(event);
    }

    // Apply narrator line
    if (auto narr = ollama_.poll_narrator()) {
        narrator_text_  = *narr;
        narrator_timer_ = 6.0f;
    }

    // Apply citizen live thought
    if (auto res = ollama_.poll_citizen_thought()) {
        uint64_t org_id = res->first;
        std::string thought = res->second;
        for (auto& o : world_.organisms) {
            if (o.id == org_id) {
                o.citizen.live_thought = thought;
                break;
            }
        }
    }
}

// ── handle_input ──────────────────────────────────────────────────────────────
void SimApp::handle_input(float dt) {
    GLFWwindow* win = renderer_.window();
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return; // ImGui has focus

    // ── Camera pan (right mouse drag) ─────────────────────────────────────────
    if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        double mx, my;
        glfwGetCursorPos(win, &mx, &my);
        if (rmb_down_) {
            float dx = static_cast<float>(mx - rmb_last_x_) / renderer_.zoom;
            float dy = static_cast<float>(my - rmb_last_y_) / renderer_.zoom;
            renderer_.camera_pos.x -= dx;
            renderer_.camera_pos.y += dy;  // flip Y
        }
        rmb_last_x_ = mx; rmb_last_y_ = my;
        rmb_down_ = true;
    } else {
        rmb_down_ = false;
    }

    // ── Zoom (scroll) ─────────────────────────────────────────────────────────
    // Handled via GLFW scroll callback set up below

    // ── God Power Hotkeys (1=Smite, 2=Genesis, 3=Stone Wall, 4=Rally, 0=None) ──
    if (glfwGetKey(win, GLFW_KEY_1) == GLFW_PRESS) world_.god_powers.active_power = GodPowerType::SMITE;
    if (glfwGetKey(win, GLFW_KEY_2) == GLFW_PRESS) world_.god_powers.active_power = GodPowerType::GENESIS;
    if (glfwGetKey(win, GLFW_KEY_3) == GLFW_PRESS) world_.god_powers.active_power = GodPowerType::STONE_WALL;
    if (glfwGetKey(win, GLFW_KEY_4) == GLFW_PRESS) world_.god_powers.active_power = GodPowerType::RALLY;
    if (glfwGetKey(win, GLFW_KEY_0) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) world_.god_powers.active_power = GodPowerType::NONE;

    // ── Left click: execute God Power OR select organism ───────────────────────
    if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        static bool click_down = false;
        if (!click_down) {
            click_down = true;
            double mx, my;
            glfwGetCursorPos(win, &mx, &my);

            // Screen to world position
            float hw = (float)Config::WINDOW_WIDTH * 0.5f / renderer_.zoom;
            float hh = (float)Config::WINDOW_HEIGHT * 0.5f / renderer_.zoom;
            float wx = renderer_.camera_pos.x - hw + static_cast<float>(mx) / renderer_.zoom;
            float wy = renderer_.camera_pos.y + hh - static_cast<float>(my) / renderer_.zoom;
            glm::vec2 world_pos = world_.wrap({wx, wy});

            if (world_.god_powers.active_power == GodPowerType::SMITE) {
                world_.execute_smite(world_pos);
                if (!world_.factions.empty() && !ollama_.is_busy()) {
                    int fid = current_leader_idx_ % (int)world_.factions.size();
                    ollama_.request_emergency_decree(world_.stats, world_.sim_time, world_.factions[fid].leader,
                        "A divine bolt of SMITE struck our realm!", "The Omnipotent God");
                }
            } else if (world_.god_powers.active_power == GodPowerType::GENESIS) {
                world_.paint_genesis_food(world_pos);
                if (!world_.factions.empty() && !ollama_.is_busy()) {
                    int fid = current_leader_idx_ % (int)world_.factions.size();
                    ollama_.request_emergency_decree(world_.stats, world_.sim_time, world_.factions[fid].leader,
                        "A divine blessing of GENESIS spawned lush food in our realm!", "The Omnipotent God");
                }
            } else if (world_.god_powers.active_power == GodPowerType::STONE_WALL) {
                world_.spawn_stone_wall(world_pos);
            } else if (world_.god_powers.active_power == GodPowerType::RALLY) {
                SoundEngine::play_trumpet();
                // Rally nearby organisms to point
                for (auto& o : world_.organisms) {
                    if (o.alive && world_.torus_dist(o.pos, world_pos) < 400.0f) {
                        o.angle = std::atan2(world_pos.x - o.pos.x, world_pos.y - o.pos.y);
                    }
                }
            } else {
                // Select organism
                int new_sel = renderer_.pick_organism(world_, mx, my);
                if (new_sel >= 0 && new_sel != selected_id_) {
                    selected_id_ = new_sel;
                    for (const auto& o : world_.organisms) {
                        if (o.id == static_cast<uint64_t>(selected_id_)) {
                            std::string fname = (o.faction_id >= 0 && o.faction_id < (int)world_.factions.size()) ? world_.factions[o.faction_id].name : "Nomad Tribe";
                            std::string lname = (o.faction_id >= 0 && o.faction_id < (int)world_.factions.size()) ? world_.factions[o.faction_id].leader.name : "High Chieftain";
                            ollama_.request_citizen_thought(o.id, o.citizen.name, o.citizen.profession, fname, lname, o.region.name, o.energy / Config::MAX_ENERGY, o.citizen.kills);
                            break;
                        }
                    }
                } else if (new_sel < 0) {
                    selected_id_ = -1;
                }
            }
        }
    } else {
        // Reset click_down flag handled by GLFW state machine
    }

    // ── Keyboard ──────────────────────────────────────────────────────────────
    static bool space_prev = false;
    bool space_now = (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS);
    if (space_now && !space_prev) paused_ = !paused_;
    space_prev = space_now;

    static bool g_prev = false;
    bool g_now = (glfwGetKey(win, GLFW_KEY_G) == GLFW_PRESS);
    if (g_now && !g_prev && !ollama_.is_busy()) {
        ollama_.request_decree(world_.stats, world_.sim_time);
        god_timer_ = Config::GOD_DECREE_INTERVAL;
    }
    g_prev = g_now;

    static bool r_prev = false;
    bool r_now = (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS);
    if (r_now && !r_prev) { world_.init(); selected_id_ = -1; }
    r_prev = r_now;

    // Speed keys
    if (glfwGetKey(win, GLFW_KEY_EQUAL) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_KP_ADD) == GLFW_PRESS)
        speed_mult_ = std::min(speed_mult_ + 1, Config::MAX_TICKS_PER_FRAME);
    if (glfwGetKey(win, GLFW_KEY_MINUS) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS)
        speed_mult_ = std::max(speed_mult_ - 1, 1);
}

// ── shutdown ──────────────────────────────────────────────────────────────────
void SimApp::shutdown() {
    ollama_.stop();
    renderer_.shutdown();
}
