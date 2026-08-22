#include "renderer.h"
#include "world.h"
#include "config.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cstring>

// ── File shader loading ───────────────────────────────────────────────────────
static std::string read_file(const char* path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

unsigned int Renderer::compile_shader(const char* path, unsigned int type) {
    std::string src = read_file(path);
    if (src.empty()) {
        // Inline fallback for robustness
        if (type == GL_VERTEX_SHADER)
            src = "#version 330 core\nvoid main(){gl_Position=vec4(0);}";
        else
            src = "#version 330 core\nout vec4 c;void main(){c=vec4(1);}";
    }
    const char* csrc = src.c_str();
    unsigned int sh = glCreateShader(type);
    glShaderSource(sh, 1, &csrc, nullptr);
    glCompileShader(sh);
    int ok; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(sh, 512, nullptr, log);
        // Silently continue; fallback geometry will show
    }
    return sh;
}

unsigned int Renderer::link_program(unsigned int vs, unsigned int fs) {
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ── Camera / view ─────────────────────────────────────────────────────────────
glm::mat4 Renderer::view_proj() const {
    float hw = win_w_ * 0.5f / zoom;
    float hh = win_h_ * 0.5f / zoom;
    glm::mat4 proj = glm::ortho(
        camera_pos.x - hw, camera_pos.x + hw,
        camera_pos.y - hh, camera_pos.y + hh,
        -1.0f, 1.0f);
    return proj;
}

// ── init ──────────────────────────────────────────────────────────────────────
bool Renderer::init(const char* title, int width, int height) {
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, Config::MSAA_SAMPLES);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) { glfwTerminate(); return false; }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // vsync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return false;

    win_w_ = width; win_h_ = height;
    camera_pos = { Config::WORLD_WIDTH * 0.5f, Config::WORLD_HEIGHT * 0.5f };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.04f, 0.04f, 0.08f, 1.0f);

    // ── Compile shaders ───────────────────────────────────────────────────────
    prog_org_    = link_program(compile_shader("shaders/organism.vert", GL_VERTEX_SHADER),
                                compile_shader("shaders/organism.frag", GL_FRAGMENT_SHADER));
    prog_food_   = link_program(compile_shader("shaders/food.vert", GL_VERTEX_SHADER),
                                compile_shader("shaders/food.frag", GL_FRAGMENT_SHADER));
    prog_grid_   = link_program(compile_shader("shaders/grid.vert", GL_VERTEX_SHADER),
                                compile_shader("shaders/grid.frag", GL_FRAGMENT_SHADER));
    prog_decree_ = link_program(compile_shader("shaders/decree.vert", GL_VERTEX_SHADER),
                                compile_shader("shaders/decree.frag", GL_FRAGMENT_SHADER));

    init_org_buffers();
    init_food_buffers();
    init_decree_buffers();
    init_grid_buffers(Config::WORLD_WIDTH, Config::WORLD_HEIGHT);

    // ── ImGui ─────────────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 15.0f);

    // Dark Cyber-Fantasy Gold Style
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 5.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding      = 4.0f;
    style.WindowBorderSize  = 1.0f;
    style.Alpha             = 0.94f;
    style.Colors[ImGuiCol_WindowBg]        = ImVec4(0.06f, 0.06f, 0.09f, 0.94f);
    style.Colors[ImGuiCol_Border]          = ImVec4(0.35f, 0.28f, 0.12f, 0.50f);
    style.Colors[ImGuiCol_TitleBg]         = ImVec4(0.08f, 0.06f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]   = ImVec4(0.22f, 0.18f, 0.08f, 1.00f);
    style.Colors[ImGuiCol_Header]          = ImVec4(0.24f, 0.20f, 0.10f, 0.80f);
    style.Colors[ImGuiCol_HeaderHovered]   = ImVec4(0.45f, 0.35f, 0.15f, 0.90f);
    style.Colors[ImGuiCol_HeaderActive]    = ImVec4(0.60f, 0.48f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_Button]          = ImVec4(0.20f, 0.16f, 0.10f, 0.85f);
    style.Colors[ImGuiCol_ButtonHovered]   = ImVec4(0.42f, 0.32f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]    = ImVec4(0.65f, 0.52f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]       = ImVec4(0.90f, 0.75f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]   = ImVec4(0.85f, 0.65f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_FrameBg]         = ImVec4(0.12f, 0.11f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.22f, 0.18f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive]   = ImVec4(0.35f, 0.28f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]       = ImVec4(0.95f, 0.80f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]      = ImVec4(0.85f, 0.70f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]= ImVec4(1.00f, 0.85f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_Tab]             = ImVec4(0.14f, 0.12f, 0.18f, 0.85f);
    style.Colors[ImGuiCol_TabHovered]      = ImVec4(0.35f, 0.28f, 0.15f, 1.00f);
    style.Colors[ImGuiCol_TabActive]       = ImVec4(0.50f, 0.40f, 0.18f, 1.00f);

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return true;
}

// ── Buffer init ───────────────────────────────────────────────────────────────
void Renderer::init_org_buffers() {
    // Triangle template (unit, pointing +Y)
    float verts[] = {
         0.0f,  1.0f,
        -0.6f, -0.7f,
         0.6f, -0.7f
    };
    glGenVertexArrays(1, &vao_org_);
    glBindVertexArray(vao_org_);

    // Template VBO
    glGenBuffers(1, &vbo_org_template_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_org_template_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    // Instance VBO
    glGenBuffers(1, &vbo_org_inst_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_org_inst_);

    int stride = sizeof(OrgInstance);
    auto offset = [](int o) { return reinterpret_cast<void*>(static_cast<intptr_t>(o)); };

    // layout(location=1) pos
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, offset(0));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);
    // layout(location=2) angle
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, offset(8));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);
    // layout(location=3) radius
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, offset(12));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    // layout(location=4) color
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, offset(16));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);
    // layout(location=5) energy_norm
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, offset(28));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);
    // layout(location=6) is_selected
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, stride, offset(32));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
    org_instance_capacity_ = 0;
}

void Renderer::init_food_buffers() {
    // Quad template [-1,1]
    float verts[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f,
    };
    glGenVertexArrays(1, &vao_food_);
    glBindVertexArray(vao_food_);

    glGenBuffers(1, &vbo_food_template_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_food_template_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vbo_food_inst_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_food_inst_);
    int stride = sizeof(FoodInstance);
    auto offset = [](int o) { return reinterpret_cast<void*>(static_cast<intptr_t>(o)); };
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, offset(0));
    glEnableVertexAttribArray(1); glVertexAttribDivisor(1, 1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, offset(8));
    glEnableVertexAttribArray(2); glVertexAttribDivisor(2, 1);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, offset(12));
    glEnableVertexAttribArray(3); glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);
    food_instance_capacity_ = 0;
}

void Renderer::init_decree_buffers() {
    // Circle ring template (line loop, 64 segments)
    const int SEG = 64;
    std::vector<float> ring;
    ring.reserve(SEG * 2);
    for (int i = 0; i < SEG; ++i) {
        float a = glm::two_pi<float>() * i / SEG;
        ring.push_back(std::cos(a));
        ring.push_back(std::sin(a));
    }

    glGenVertexArrays(1, &vao_decree_);
    glBindVertexArray(vao_decree_);

    glGenBuffers(1, &vbo_decree_template_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_decree_template_);
    glBufferData(GL_ARRAY_BUFFER, ring.size() * sizeof(float), ring.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &vbo_decree_inst_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_decree_inst_);
    int stride = sizeof(DecreeInstance);
    auto offset = [](int o) { return reinterpret_cast<void*>(static_cast<intptr_t>(o)); };
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, offset(0));
    glEnableVertexAttribArray(1); glVertexAttribDivisor(1, 1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, offset(8));
    glEnableVertexAttribArray(2); glVertexAttribDivisor(2, 1);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, offset(12));
    glEnableVertexAttribArray(3); glVertexAttribDivisor(3, 1);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, offset(16));
    glEnableVertexAttribArray(4); glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
    decree_instance_capacity_ = 0;
}

void Renderer::init_grid_buffers(float world_w, float world_h) {
    std::vector<float> lines;
    float step = 200.0f;
    for (float x = 0; x <= world_w; x += step) {
        lines.push_back(x); lines.push_back(0);
        lines.push_back(x); lines.push_back(world_h);
    }
    for (float y = 0; y <= world_h; y += step) {
        lines.push_back(0);       lines.push_back(y);
        lines.push_back(world_w); lines.push_back(y);
    }
    grid_vertex_count_ = (int)lines.size() / 2;

    glGenVertexArrays(1, &vao_grid_);
    glBindVertexArray(vao_grid_);
    glGenBuffers(1, &vbo_grid_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_grid_);
    glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(float), lines.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// ── Dynamic VBO resize helper ─────────────────────────────────────────────────
void Renderer::upload_grow_buffer(unsigned int vbo, const void* data,
                                   size_t byte_size, int& capacity) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    int needed = static_cast<int>(byte_size);
    if (needed > capacity) {
        capacity = std::max(needed, capacity * 2 + 1024);
        glBufferData(GL_ARRAY_BUFFER, capacity, nullptr, GL_DYNAMIC_DRAW);
    }
    if (byte_size > 0)
        glBufferSubData(GL_ARRAY_BUFFER, 0, byte_size, data);
}

// ── begin / end frame ─────────────────────────────────────────────────────────
void Renderer::begin_frame() {
    glfwPollEvents();
    glfwGetFramebufferSize(window_, &win_w_, &win_h_);
    glViewport(0, 0, win_w_, win_h_);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Renderer::end_frame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_);
}

bool Renderer::should_close() const {
    return glfwWindowShouldClose(window_);
}

// ── draw_world ────────────────────────────────────────────────────────────────
void Renderer::draw_world(const World& world, int selected_id) {
    glm::mat4 vp = view_proj();

    // ── Grid ──────────────────────────────────────────────────────────────────
    glUseProgram(prog_grid_);
    glUniformMatrix4fv(glGetUniformLocation(prog_grid_, "view_proj"), 1, GL_FALSE, glm::value_ptr(vp));
    glBindVertexArray(vao_grid_);
    glDrawArrays(GL_LINES, 0, grid_vertex_count_);

    // ── Food & Resource Nodes (instanced quads) ───────────────────────────────
    std::vector<FoodInstance> food_inst;
    food_inst.reserve(world.food.size() + world.resource_nodes.size());
    for (const auto& f : world.food) {
        if (!f.alive) continue;
        FoodInstance fi;
        fi.pos    = f.pos;
        fi.radius = f.radius;
        fi.alpha  = 0.85f;
        food_inst.push_back(fi);
    }

    // Resource Nodes (Wood/Iron/Gold)
    for (const auto& rn : world.resource_nodes) {
        if (rn.amount <= 0.0f) continue;
        FoodInstance fi;
        fi.pos    = rn.pos;
        fi.radius = (rn.type == 1) ? 9.0f : 7.0f;
        fi.alpha  = 0.95f;
        food_inst.push_back(fi);
    }

    if (!food_inst.empty()) {
        glUseProgram(prog_food_);
        glUniformMatrix4fv(glGetUniformLocation(prog_food_, "view_proj"), 1, GL_FALSE, glm::value_ptr(vp));
        glBindVertexArray(vao_food_);
        upload_grow_buffer(vbo_food_inst_, food_inst.data(),
                           food_inst.size() * sizeof(FoodInstance),
                           food_instance_capacity_);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, (GLsizei)food_inst.size());
    }

    // ── Decree rings, War Banners & Territory Glows (instanced line loops) ────
    std::vector<DecreeInstance> dec_inst;
    dec_inst.reserve(world.decree_visuals.size() + world.war_banners.size() + world.factions.size() + 2);
    
    // Active decree visuals
    for (const auto& dv : world.decree_visuals) {
        DecreeInstance di;
        di.pos    = dv.pos;
        di.radius = dv.radius;
        di.t_norm = dv.lifetime / dv.max_life;
        di.color  = dv.color;
        dec_inst.push_back(di);
    }

    // Active War Banners (pulsing red/gold flags)
    for (const auto& wb : world.war_banners) {
        DecreeInstance di;
        di.pos    = wb.pos;
        di.radius = 80.0f + 20.0f * std::sin(world.sim_time * 6.0f);
        di.t_norm = wb.lifetime / wb.max_life;
        di.color  = (wb.attacking_faction >= 0 && wb.attacking_faction < (int)world.factions.size())
            ? world.factions[wb.attacking_faction].color
            : glm::vec3(1.0f, 0.2f, 0.2f);
        dec_inst.push_back(di);
    }

    // Faction Territory Capital & Outpost Glows
    for (const auto& f : world.factions) {
        if (f.member_count == 0) continue;
        DecreeInstance di;
        di.pos    = f.capital_pos;
        di.radius = f.border_radius;
        di.t_norm = 0.5f + 0.2f * std::sin(world.sim_time * 2.0f + f.id);
        di.color  = f.color;
        dec_inst.push_back(di);

        for (const auto& op : f.outposts) {
            DecreeInstance op_di;
            op_di.pos    = op.pos;
            op_di.radius = 45.0f;
            op_di.t_norm = 0.8f;
            op_di.color  = f.color;
            dec_inst.push_back(op_di);
        }
    }

    // Selected Organism Vision Cone / Sensing Range
    if (selected_id >= 0) {
        for (const auto& o : world.organisms) {
            if (o.alive && o.id == static_cast<uint64_t>(selected_id)) {
                DecreeInstance di;
                di.pos    = o.pos;
                di.radius = o.vision_range;
                di.t_norm = 0.9f;
                di.color  = glm::vec3(0.2f, 1.0f, 0.4f);
                dec_inst.push_back(di);
                break;
            }
        }
    }

    if (!dec_inst.empty()) {
        glUseProgram(prog_decree_);
        glUniformMatrix4fv(glGetUniformLocation(prog_decree_, "view_proj"), 1, GL_FALSE, glm::value_ptr(vp));
        glBindVertexArray(vao_decree_);
        upload_grow_buffer(vbo_decree_inst_, dec_inst.data(),
                           dec_inst.size() * sizeof(DecreeInstance),
                           decree_instance_capacity_);
        glLineWidth(2.5f);
        glDrawArraysInstanced(GL_LINE_LOOP, 0, 64, (GLsizei)dec_inst.size());
        glLineWidth(1.0f);
    }

    // ── Organisms (instanced triangles) ───────────────────────────────────────
    std::vector<OrgInstance> org_inst;
    org_inst.reserve(world.organisms.size());
    for (const auto& o : world.organisms) {
        if (!o.alive) continue;
        OrgInstance oi;
        oi.pos         = o.pos;
        oi.angle       = o.angle;
        oi.radius      = o.radius;
        oi.color       = o.color;
        oi.energy_norm = o.energy / Config::MAX_ENERGY;
        oi.is_selected = (o.id == static_cast<uint64_t>(selected_id)) ? 1.0f : 0.0f;
        org_inst.push_back(oi);
    }
    if (!org_inst.empty()) {
        glUseProgram(prog_org_);
        glUniformMatrix4fv(glGetUniformLocation(prog_org_, "view_proj"), 1, GL_FALSE, glm::value_ptr(vp));
        glBindVertexArray(vao_org_);
        upload_grow_buffer(vbo_org_inst_, org_inst.data(),
                           org_inst.size() * sizeof(OrgInstance),
                           org_instance_capacity_);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 3, (GLsizei)org_inst.size());
    }
}

// ── draw_ui ───────────────────────────────────────────────────────────────────
void Renderer::draw_ui(World& world, int& selected_id, int& speed_mult,
                        bool& paused, float fps,
                        const std::string& ollama_status,
                        const std::string& narrator_text)
{
    const Stats& s = world.stats;
    ImGuiIO& io = ImGui::GetIO();

    // ── Stats panel (top-left) ────────────────────────────────────────────────
    ImGui::SetNextWindowPos({10, 10}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({320, 0}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.88f);
    ImGui::Begin("##stats", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    ImGui::TextColored({0.4f,0.9f,0.5f,1}, "🧬 DIGITAL LIFE SIMULATOR");
    ImGui::Separator();

    ImGui::Columns(2, "statcols", false);
    ImGui::Text("Year");          ImGui::NextColumn(); ImGui::TextColored({1.0f,0.85f,0.3f,1.0f}, "Year %d (12s)", world.history_engine.sim_year); ImGui::NextColumn();
    ImGui::Text("FPS");           ImGui::NextColumn(); ImGui::Text("%.0f", fps); ImGui::NextColumn();
    ImGui::Text("Sim Time");      ImGui::NextColumn(); ImGui::Text("%.1f s", s.sim_time); ImGui::NextColumn();
    ImGui::Text("Population");    ImGui::NextColumn();
    ImGui::TextColored({0.4f,0.9f,0.5f,1}, "%d", s.population);
    ImGui::NextColumn();
    ImGui::Text("Generation");    ImGui::NextColumn(); ImGui::Text("%d", s.generation_max); ImGui::NextColumn();
    ImGui::Text("Food");          ImGui::NextColumn(); ImGui::Text("%d", s.food_count); ImGui::NextColumn();
    ImGui::Text("Species");       ImGui::NextColumn(); ImGui::Text("%d", s.lineage_count); ImGui::NextColumn();
    ImGui::Text("Births/Deaths"); ImGui::NextColumn();
    ImGui::Text("%lld / %lld", s.total_births, s.total_deaths);
    ImGui::NextColumn();
    ImGui::Columns(1);
    ImGui::Spacing();

    // Herbivore / Carnivore bar
    ImGui::TextColored({0.5f,1.0f,0.5f,1}, "Herb %.0f%%", s.herbivore_ratio * 100.0f);
    ImGui::SameLine();
    ImGui::TextColored({1.0f,0.4f,0.3f,1}, "Carn %.0f%%", s.carnivore_ratio * 100.0f);
    float bar_w = ImGui::GetContentRegionAvail().x;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.3f, 0.9f, 0.4f, 1.0f));
    ImGui::ProgressBar(s.herbivore_ratio, ImVec2(bar_w, 8.0f), "");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Population history graph
    auto pop = s.pop_array();
    if (!pop.empty()) {
        float max_pop = *std::max_element(pop.begin(), pop.end());
        ImGui::TextColored({0.7f,0.7f,1.0f,1}, "Population History");
        ImGui::PlotLines("##pop", pop.data(), (int)pop.size(), 0, nullptr,
                         0.0f, std::max(max_pop * 1.1f, 10.0f),
                         ImVec2(bar_w, 55));
    }

    // Herbivore ratio history
    auto herb = s.herb_array();
    if (!herb.empty()) {
        ImGui::TextColored({0.5f,1.0f,0.5f,1}, "Herbivore Ratio");
        ImGui::PlotLines("##herb", herb.data(), (int)herb.size(), 0, nullptr,
                         0.0f, 1.0f, ImVec2(bar_w, 40));
    }

    // Avg speed / aggression
    auto spd = s.spd_array();
    auto agg = s.agg_array();
    if (!spd.empty()) {
        ImGui::TextColored({0.8f,0.8f,0.3f,1}, "Avg Speed");
        float max_s = *std::max_element(spd.begin(), spd.end());
        ImGui::PlotLines("##spd", spd.data(), (int)spd.size(), 0, nullptr,
                         0.0f, std::max(max_s * 1.1f, 10.0f),
                         ImVec2(bar_w, 35));
    }
    if (!agg.empty()) {
        ImGui::TextColored({1.0f,0.5f,0.3f,1}, "Avg Aggression");
        ImGui::PlotLines("##agg", agg.data(), (int)agg.size(), 0, nullptr,
                         0.0f, 1.0f, ImVec2(bar_w, 35));
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Controls ──────────────────────────────────────────────────────────────
    if (paused) {
        if (ImGui::Button("▶ Resume", {100,0})) paused = false;
    } else {
        if (ImGui::Button("⏸ Pause",  {100,0})) paused = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("↺ Reset", {80,0})) { world.init(); selected_id = -1; }

    ImGui::Text("Speed:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    int sm = speed_mult;
    if (ImGui::SliderInt("##spd_mult", &sm, 1, Config::MAX_TICKS_PER_FRAME, "%dx"))
        speed_mult = sm;
    ImGui::SameLine();
    if (ImGui::Button("MAX")) speed_mult = Config::MAX_TICKS_PER_FRAME;

    ImGui::Spacing();
    ImGui::End();

    // ── Ollama World God panel (bottom-left) ──────────────────────────────────
    float decree_h = 200.0f;
    ImGui::SetNextWindowPos({10, (float)win_h_ - decree_h - 10}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({360, decree_h}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.88f);
    ImGui::Begin("🌍 Ollama World God", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored({0.8f,0.5f,1.0f,1}, "Model: %s", Config::OLLAMA_MODEL);
    ImGui::SameLine();
    if (ollama_status.find("Idle") != std::string::npos)
        ImGui::TextColored({0.4f,0.9f,0.4f,1}, "● Idle");
    else
        ImGui::TextColored({1.0f,0.8f,0.2f,1}, "⟳ %s", ollama_status.c_str());

    ImGui::Separator();
    ImGui::TextColored({0.6f,0.6f,0.8f,1}, "Decree & Geopolitical Log:");
    ImGui::BeginChild("##decree_log", ImVec2(0, 120), false);
    for (int i = (int)world.decree_log.size() - 1; i >= 0; --i) {
        const auto& d = world.decree_log[i];
        ImGui::TextColored({0.9f,0.7f,0.2f,1}, "[%.0fs] %s", d.sim_time, d.decree_name.c_str());
        ImGui::PushTextWrapPos();
        ImGui::TextColored({0.7f,0.7f,0.7f,1}, "  %s", d.description.c_str());
        ImGui::PopTextWrapPos();
    }
    ImGui::EndChild();
    ImGui::End();

    // ── Geopolitics & Diplomacy Matrix Panel (top-right) ──────────────────────
    ImGui::SetNextWindowPos({(float)win_w_ - 360, 10}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({350, 240}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.88f);
    ImGui::Begin("👑 Factions & Diplomatic Matrix", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    if (ImGui::BeginTabBar("##geotabs")) {
        if (ImGui::BeginTabItem("Leaders Council")) {
            for (const auto& f : world.factions) {
                if (f.member_count == 0 && f.territory_pct == 0) continue;
                ImGui::ColorButton("##faccol", ImVec4(f.color.r, f.color.g, f.color.b, 1.0f), 0, ImVec2(10, 10));
                ImGui::SameLine();
                ImGui::TextColored({f.color.r, f.color.g, f.color.b, 1.0f}, "%s (%s)", f.leader.name.c_str(), f.leader.title.c_str());
                ImGui::SameLine();
                ImGui::TextColored({0.6f,0.6f,0.6f,1.0f}, "[%s]", f.leader.model.c_str());
                if (!f.leader.current_quote.empty()) {
                    ImGui::PushTextWrapPos();
                    ImGui::TextColored({0.9f, 0.8f, 0.3f, 1.0f}, "  \"%s\"", f.leader.current_quote.c_str());
                    ImGui::PopTextWrapPos();
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Factions & Economy")) {
            for (const auto& f : world.factions) {
                if (f.member_count == 0 && f.territory_pct == 0) continue;
                ImGui::ColorButton("##faccol", ImVec4(f.color.r, f.color.g, f.color.b, 1.0f), 0, ImVec2(12, 12));
                ImGui::SameLine();
                ImGui::TextColored({f.color.r, f.color.g, f.color.b, 1.0f}, "%s [%s]", f.name.c_str(), settlement_tier_str(f.tier));
                ImGui::TextColored({0.7f, 0.9f, 1.0f, 1.0f}, "  Gov: %s", government_type_str(f.government));
                ImGui::TextColored({0.8f, 0.8f, 0.8f, 1.0f}, "  Pop: %d | Terr: %.0f%% | Border: %.0f", f.member_count, f.territory_pct, f.border_radius);
                ImGui::TextColored({0.9f, 0.8f, 0.2f, 1.0f}, "  Grain: %.0f | Wood: %.0f | Iron: %.0f | Gold: %.0f", f.treasury_food, f.resource_wood, f.resource_iron, f.resource_gold);
                ImGui::Separator();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Diplomacy Matrix")) {
            int nfac = std::min((int)world.factions.size(), 6);
            if (nfac > 1) {
                ImGui::Columns(nfac + 1, "dipgrid", true);
                ImGui::Text("F#"); ImGui::NextColumn();
                for (int j = 0; j < nfac; ++j) {
                    ImGui::Text("F%d", j); ImGui::NextColumn();
                }
                ImGui::Separator();

                for (int i = 0; i < nfac; ++i) {
                    ImGui::Text("F%d", i); ImGui::NextColumn();
                    for (int j = 0; j < nfac; ++j) {
                        if (i == j) {
                            ImGui::TextDisabled("-");
                        } else {
                            DiplomaticStatus st = world.diplomacy.get_status(i, j);
                            if (st == DiplomaticStatus::WAR) {
                                ImGui::TextColored({1.0f,0.2f,0.2f,1.0f}, "WAR");
                            } else if (st == DiplomaticStatus::ALLIANCE) {
                                ImGui::TextColored({0.2f,0.8f,1.0f,1.0f}, "ALLY");
                            } else {
                                ImGui::TextColored({0.6f,0.6f,0.6f,1.0f}, "Peace");
                            }
                        }
                        ImGui::NextColumn();
                    }
                }
                ImGui::Columns(1);
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("🎯 Military & Siege")) {
            ImGui::TextColored({1.0f,0.7f,0.2f,1.0f}, "Active Siege Engines: %d", (int)world.siege_engines.size());
            ImGui::TextColored({1.0f,0.3f,0.3f,1.0f}, "Active War Banners: %d", (int)world.war_banners.size());
            ImGui::Separator();
            for (const auto& f : world.factions) {
                if (f.member_count == 0) continue;
                ImGui::TextColored({f.color.r, f.color.g, f.color.b, 1.0f}, "%s Military:", f.name.c_str());
                ImGui::Text("  Power Rating: %.1f | Fort Outposts: %d", f.military_power, (int)f.outposts.size());
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("🌋 Environment & Hazards")) {
            ImGui::TextColored({1.0f, 0.4f, 0.2f, 1.0f}, "Active Disasters: %d", (int)world.disasters.size());
            ImGui::TextColored({0.3f, 0.9f, 1.0f, 1.0f}, "Resource Veins: %d", (int)world.resource_nodes.size());
            ImGui::TextColored({0.2f, 0.9f, 0.4f, 1.0f}, "Farm Fields: %d", (int)world.farm_plots.size());
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("🧠 Ruler AI & Goals")) {
            for (const auto& r : world.rulers) {
                ImGui::TextColored({0.9f, 0.8f, 0.3f, 1.0f}, "👑 %s (Faction #%d)", r.ruler_name.c_str(), r.faction_id);
                ImGui::Text("  Primary Goal: %s", r.primary_goal.c_str());
                ImGui::TextColored({0.8f, 0.4f, 0.9f, 1.0f}, "  Hidden Secret: %s", r.belief.secret_goal.c_str());
                ImGui::Text("  Perceived Enemy Army: %d troops", r.belief.perceived_enemy_army);
                ImGui::Separator();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("📈 Markets & Economy")) {
            ImGui::TextColored({0.2f, 0.9f, 0.4f, 1.0f}, "Global Inflation: %.1f%% | GDP Growth: %.1f%%", world.economy_engine.global_inflation * 100.0f, world.economy_engine.global_gdp_growth * 100.0f);
            ImGui::Separator();
            for (int i = 0; i <= (int)ResourceType::LABOR; ++i) {
                const auto& p = world.economy_engine.market_prices[i];
                ImGui::Text("%-18s Price: %5.1f Gold | Supp: %.0f | Dem: %.0f", resource_type_str(static_cast<ResourceType>(i)), p.price, p.supply, p.demand);
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("📜 1,000-Year Timeline")) {
            ImGui::TextColored({1.0f, 0.85f, 0.3f, 1.0f}, "Simulation Year: %d (12s = 1 Year)", world.history_engine.sim_year);
            ImGui::BeginChild("##timeline_child", ImVec2(0, 140), false);
            for (int i = (int)world.history_engine.timeline.size() - 1; i >= 0; --i) {
                const auto& ev = world.history_engine.timeline[i];
                ImGui::TextColored({0.4f, 0.8f, 1.0f, 1.0f}, "[Year %d] 📜 %s", ev.year, ev.headline.c_str());
                ImGui::PushTextWrapPos();
                ImGui::TextColored({0.8f, 0.8f, 0.8f, 1.0f}, "  %s", ev.details.c_str());
                ImGui::PopTextWrapPos();
                ImGui::Separator();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Daily Chronicle")) {
            ImGui::BeginChild("##chronicle_child", ImVec2(0, 160), false);
            for (int i = (int)world.chronicle.articles().size() - 1; i >= 0; --i) {
                const auto& art = world.chronicle.articles()[i];
                ImGui::TextColored({1.0f, 0.85f, 0.3f, 1.0f}, "[%.0fs] 📰 %s", art.sim_time, art.headline.c_str());
                ImGui::PushTextWrapPos();
                ImGui::TextColored({0.75f, 0.75f, 0.75f, 1.0f}, "  %s", art.body.c_str());
                ImGui::PopTextWrapPos();
                ImGui::Separator();
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    // ── God Powers Toolbar (top center) ───────────────────────────────────────
    ImGui::SetNextWindowPos({win_w_ * 0.5f - 180.0f, 10}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({360, 50}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.92f);
    ImGui::Begin("⚡ God Miracle Wand", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    GodPowerType p = world.god_powers.active_power;
    if (ImGui::RadioButton("⚡ Smite (1)", p == GodPowerType::SMITE)) world.god_powers.active_power = GodPowerType::SMITE;
    ImGui::SameLine();
    if (ImGui::RadioButton("🍇 Genesis (2)", p == GodPowerType::GENESIS)) world.god_powers.active_power = GodPowerType::GENESIS;
    ImGui::SameLine();
    if (ImGui::RadioButton("🏔 Wall (3)", p == GodPowerType::STONE_WALL)) world.god_powers.active_power = GodPowerType::STONE_WALL;
    ImGui::SameLine();
    if (ImGui::RadioButton("🎺 Rally (4)", p == GodPowerType::RALLY)) world.god_powers.active_power = GodPowerType::RALLY;

    ImGui::End();

    // ── Selected organism inspector (right side) ───────────────────────────────
    if (selected_id >= 0) {
        const Organism* sel = nullptr;
        for (const auto& o : world.organisms)
            if (o.id == static_cast<uint64_t>(selected_id)) { sel = &o; break; }

        if (sel) {
            ImGui::SetNextWindowPos({(float)win_w_ - 290, 260}, ImGuiCond_Always);
            ImGui::SetNextWindowSize({280, 0}, ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.90f);
            ImGui::Begin("👤 Citizen Dossier", nullptr,
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

            ImGui::TextColored({sel->color.r, sel->color.g, sel->color.b, 1},
                               "%s", sel->citizen.name.c_str());
            ImGui::TextColored({0.7f, 0.7f, 0.9f, 1.0f}, "Role: %s", sel->citizen.profession.c_str());
            ImGui::TextColored({0.9f, 0.8f, 0.3f, 1.0f}, "Drive: %s", cognitive_drive_str(sel->current_drive));
            ImGui::TextColored({sel->region.color.r, sel->region.color.g, sel->region.color.b, 1.0f}, "Region: %s", sel->region.name.c_str());
            ImGui::Text("Kills: %d  |  Gen %d", sel->citizen.kills, sel->generation);
            ImGui::Separator();

            if (!sel->citizen.live_thought.empty()) {
                ImGui::TextColored({0.9f, 0.8f, 0.3f, 1.0f}, "🧠 Inner Thought:");
                ImGui::PushTextWrapPos();
                ImGui::TextColored({0.8f, 0.8f, 0.8f, 1.0f}, "  \"%s\"", sel->citizen.live_thought.c_str());
                ImGui::PopTextWrapPos();
                ImGui::Separator();
            }

            ImGui::Text("Energy:");
            ImGui::ProgressBar(sel->energy / Config::MAX_ENERGY,
                               ImVec2(-1, 8), "");
            ImGui::Text("Hunger:");
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f,0.4f,0.2f,1.0f));
            ImGui::ProgressBar(sel->hunger, ImVec2(-1, 8), "");
            ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::Columns(2, "inspector", false);
            ImGui::Text("Age");         ImGui::NextColumn(); ImGui::Text("%.1f s", sel->age);          ImGui::NextColumn();
            ImGui::Text("Speed");       ImGui::NextColumn(); ImGui::Text("%.1f",   sel->speed);         ImGui::NextColumn();
            ImGui::Text("Vision");      ImGui::NextColumn(); ImGui::Text("%.1f",   sel->vision_range);  ImGui::NextColumn();
            ImGui::Text("Metabolism");  ImGui::NextColumn(); ImGui::Text("%.2f",   sel->metabolism);    ImGui::NextColumn();
            ImGui::Text("Aggression");  ImGui::NextColumn(); ImGui::Text("%.2f",   sel->aggression);    ImGui::NextColumn();
            ImGui::Text("Herbivore");   ImGui::NextColumn(); ImGui::Text("%.2f",   sel->herbivore);     ImGui::NextColumn();
            ImGui::Text("Mutation Rt"); ImGui::NextColumn(); ImGui::Text("%.3f",   sel->mutation_rate); ImGui::NextColumn();
            ImGui::Text("Children");    ImGui::NextColumn(); ImGui::Text("%d",      sel->children_count);ImGui::NextColumn();
            ImGui::Columns(1);

            // NN hidden state visualization (12 neurons)
            ImGui::Spacing();
            ImGui::TextColored({0.6f,0.8f,1.0f,1}, "GRU Memory State:");
            const auto& hstate = sel->nn.h;
            float bar_w2 = ImGui::GetContentRegionAvail().x / (float)hstate.size();
            for (int i = 0; i < (int)hstate.size(); ++i) {
                float v = (hstate[i] + 1.0f) * 0.5f; // [-1,1] → [0,1]
                ImVec4 col = (hstate[i] > 0)
                    ? ImVec4(0.2f, 0.7f * v + 0.3f, 0.2f, 1.0f)
                    : ImVec4(0.7f * (1.0f-v) + 0.3f, 0.2f, 0.2f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, col);
                ImGui::ProgressBar(v, ImVec2(bar_w2 - 2, 12), "");
                ImGui::PopStyleColor();
                if (i < (int)hstate.size() - 1) ImGui::SameLine(0, 2);
            }

            if (ImGui::Button("Deselect")) selected_id = -1;
            ImGui::End();
        } else {
            selected_id = -1;
        }
    }

    // ── Narrator toast (top-center) ────────────────────────────────────────────
    if (!narrator_text.empty() && narrator_timer > 0.0f) {
        float alpha = std::min(1.0f, narrator_timer / 1.0f); // fade in/out
        ImGui::SetNextWindowPos({win_w_ * 0.5f - 250.0f, 80}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({500, 0}, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.85f * alpha);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f,0.05f,0.15f,0.9f));
        ImGui::Begin("##narrator", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs);
        ImGui::PushTextWrapPos(480);
        ImGui::TextColored({0.9f * alpha, 0.7f * alpha, 1.0f * alpha, alpha},
                           "📜 %s", narrator_text.c_str());
        ImGui::PopTextWrapPos();
        ImGui::End();
        ImGui::PopStyleColor();
    }

    // ── Help tooltip ──────────────────────────────────────────────────────────
    ImGui::SetNextWindowPos({(float)win_w_ - 240, (float)win_h_ - 80}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("##help", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextColored({0.5f,0.5f,0.5f,1},
        "LClick:Select  RDrag:Pan  Scroll:Zoom\n"
        "Space:Pause  G:Force Decree  R:Reset");
    ImGui::End();
}

// ── pick_organism ─────────────────────────────────────────────────────────────
int Renderer::pick_organism(const World& world, double sx, double sy) {
    // Convert screen coords to world coords
    float hw = win_w_ * 0.5f / zoom;
    float hh = win_h_ * 0.5f / zoom;
    float wx = camera_pos.x - hw + static_cast<float>(sx) / zoom;
    float wy = camera_pos.y - hh + static_cast<float>(sy) / zoom;
    // Note: OpenGL Y is flipped vs screen Y
    wy = camera_pos.y + hh - static_cast<float>(sy) / zoom;

    float best_d  = 20.0f; // pixel pick radius
    int   best_id = -1;
    for (const auto& o : world.organisms) {
        if (!o.alive) continue;
        float d = glm::length(o.pos - glm::vec2(wx, wy));
        if (d < o.radius + best_d && d < best_d + o.radius) {
            best_d  = d;
            best_id = static_cast<int>(o.id);
        }
    }
    return best_id;
}

// ── shutdown / destructor ─────────────────────────────────────────────────────
void Renderer::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteProgram(prog_org_);
    glDeleteProgram(prog_food_);
    glDeleteProgram(prog_grid_);
    glDeleteProgram(prog_decree_);

    glDeleteVertexArrays(1, &vao_org_);
    glDeleteVertexArrays(1, &vao_food_);
    glDeleteVertexArrays(1, &vao_decree_);
    glDeleteVertexArrays(1, &vao_grid_);

    unsigned int bufs[] = {
        vbo_org_template_, vbo_org_inst_,
        vbo_food_template_, vbo_food_inst_,
        vbo_decree_template_, vbo_decree_inst_,
        vbo_grid_
    };
    glDeleteBuffers(7, bufs);

    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

Renderer::~Renderer() {
    // shutdown() is called explicitly from SimApp
}
