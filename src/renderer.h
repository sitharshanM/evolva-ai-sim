#pragma once
#include <string>
#include <vector>
#include <optional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
struct GLFWwindow;
struct World;

// Per-instance GPU data for organisms
struct OrgInstance {
    glm::vec2 pos;
    float     angle;
    float     radius;
    glm::vec3 color;
    float     energy_norm; // 0–1
    float     is_selected; // 0 or 1
};

// Per-instance GPU data for food
struct FoodInstance {
    glm::vec2 pos;
    float     radius;
    float     alpha;
};

// Per-instance GPU data for decree circles
struct DecreeInstance {
    glm::vec2 pos;
    float     radius;
    float     t_norm; // lifetime normalized [0,1]
    glm::vec3 color;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Renderer  —  Manages OpenGL context, shaders, instanced drawing, and ImGui
// ─────────────────────────────────────────────────────────────────────────────
class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool init(const char* title, int width, int height);
    void begin_frame();
    void end_frame();
    void shutdown();

    bool should_close() const;

    // Draw the world
    void draw_world(const World& world, int selected_id);
    // Draw ImGui panels (call between begin_frame/end_frame)
    void draw_ui(World& world, int& selected_id, int& speed_mult,
                 bool& paused, float fps,
                 const std::string& ollama_status,
                 const std::string& narrator_text);

    // Camera
    glm::vec2 camera_pos{0.0f, 0.0f};
    float     zoom        = 0.35f;

    // Mouse picking: returns organism index at screen pos, -1 if none
    int pick_organism(const World& world, double sx, double sy);

    GLFWwindow* window() const { return window_; }

    // Narrator toast
    std::string narrator_msg;
    float       narrator_timer = 0.0f;

private:
    GLFWwindow* window_ = nullptr;

    // ── Shader programs ───────────────────────────────────────────────────────
    unsigned int prog_org_    = 0;
    unsigned int prog_food_   = 0;
    unsigned int prog_grid_   = 0;
    unsigned int prog_decree_ = 0;

    // ── Organism instanced draw ───────────────────────────────────────────────
    unsigned int vao_org_ = 0;
    unsigned int vbo_org_template_ = 0;  // 3-vertex triangle
    unsigned int vbo_org_inst_     = 0;  // per-instance data
    int org_instance_capacity_     = 0;

    // ── Food instanced draw ───────────────────────────────────────────────────
    unsigned int vao_food_ = 0;
    unsigned int vbo_food_template_ = 0;  // 4-vertex quad
    unsigned int vbo_food_inst_     = 0;
    int food_instance_capacity_     = 0;

    // ── Decree rings ──────────────────────────────────────────────────────────
    unsigned int vao_decree_ = 0;
    unsigned int vbo_decree_template_ = 0;
    unsigned int vbo_decree_inst_     = 0;
    int decree_instance_capacity_     = 0;

    // ── Grid ──────────────────────────────────────────────────────────────────
    unsigned int vao_grid_ = 0;
    unsigned int vbo_grid_ = 0;
    int grid_vertex_count_ = 0;

    // ── Helpers ───────────────────────────────────────────────────────────────
    unsigned int compile_shader(const char* path, unsigned int type);
    unsigned int link_program  (unsigned int vs, unsigned int fs);
    glm::mat4    view_proj()   const;

    void init_org_buffers();
    void init_food_buffers();
    void init_decree_buffers();
    void init_grid_buffers(float world_w, float world_h);

    void upload_grow_buffer(unsigned int vbo, const void* data,
                            size_t byte_size, int& capacity);

    int win_w_ = 0, win_h_ = 0;
};
