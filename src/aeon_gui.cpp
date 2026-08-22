#include "aeon_gui.h"
#include "aeon_ollama.h"
#include "aeon_gemini.h"
#include "aeon_types.h"


// OpenGL & GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Windows GDI+ for OpenStreetMap Tile Image Loading
#include <windows.h>
#include <objbase.h>
#include <propidl.h>
#include <gdiplus.h>


// ImGui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>


#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>


namespace Aeon {

AeonGUI::~AeonGUI() {
    shutdown();
}

bool AeonGUI::init(const char* title, int width, int height) {
    if (!glfwInit()) {
        std::cerr << "[GUI ERROR] Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        std::cerr << "[GUI ERROR] Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable VSync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[GUI ERROR] Failed to initialize GLAD\n";
        return false;
    }

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    apply_modern_theme();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return true;
}

void AeonGUI::apply_modern_theme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 8.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;
    style.ItemSpacing       = ImVec2(10.0f, 6.0f);
    style.WindowPadding     = ImVec2(12.0f, 12.0f);
    style.FramePadding      = ImVec2(8.0f, 4.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]           = ImVec4(0.043f, 0.055f, 0.078f, 1.00f); // #0B0E14
    colors[ImGuiCol_ChildBg]            = ImVec4(0.062f, 0.078f, 0.106f, 1.00f); // #10141B
    colors[ImGuiCol_PopupBg]            = ImVec4(0.078f, 0.098f, 0.133f, 0.98f);
    colors[ImGuiCol_Border]             = ImVec4(0.157f, 0.204f, 0.275f, 0.50f);
    colors[ImGuiCol_BorderShadow]       = ImVec4(0.000f, 0.000f, 0.000f, 0.00f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.094f, 0.122f, 0.165f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.141f, 0.184f, 0.247f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.188f, 0.243f, 0.325f, 1.00f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.043f, 0.055f, 0.078f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.078f, 0.098f, 0.133f, 1.00f);
    colors[ImGuiCol_MenuBarBg]          = ImVec4(0.062f, 0.078f, 0.106f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.043f, 0.055f, 0.078f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.157f, 0.204f, 0.275f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.000f, 0.700f, 0.850f, 1.00f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.000f, 0.898f, 1.000f, 1.00f); // Neon Cyan #00E5FF
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.000f, 0.898f, 1.000f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.000f, 1.000f, 0.800f, 1.00f);
    colors[ImGuiCol_Button]             = ImVec4(0.094f, 0.157f, 0.235f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.000f, 0.500f, 0.700f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.000f, 0.700f, 0.900f, 1.00f);
    colors[ImGuiCol_Header]             = ImVec4(0.094f, 0.157f, 0.235f, 1.00f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.000f, 0.500f, 0.700f, 1.00f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.000f, 0.700f, 0.900f, 1.00f);
    colors[ImGuiCol_Tab]                = ImVec4(0.078f, 0.098f, 0.133f, 1.00f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.000f, 0.600f, 0.800f, 1.00f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.000f, 0.450f, 0.650f, 1.00f);
    colors[ImGuiCol_TabUnfocused]       = ImVec4(0.055f, 0.070f, 0.095f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.078f, 0.118f, 0.165f, 1.00f);
}

void AeonGUI::shutdown() {
    if (window_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
    }
}

bool AeonGUI::should_close() const {
    return window_ ? glfwWindowShouldClose(window_) : true;
}

void AeonGUI::render_frame(AeonEngine& engine) {
    if (!window_) return;

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Set full screen dock window for AEON GUI
    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));

    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("AEON Dashboard", nullptr, win_flags);

    // Top Header & Controls
    draw_control_bar(engine);
    ImGui::Separator();

    // ── 2-Level Executive Category Navigation System ──
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 4.0f));

    const char* categories[] = {
        " [GOV] Politics & State ",
        " [DEF] Defense & Tech ",
        " [ECON] Market & Industry ",
        " [MAP] Map & Disasters ",
        " [DIPL] Diplomacy & Logs ",
        " ⚡ [GOD] Divine Powers "
    };

    for (int c = 0; c < 6; ++c) {
        if (c > 0) ImGui::SameLine();
        bool is_selected = (active_category_ == c);
        if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.48f, 0.88f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.58f, 0.98f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.18f, 0.28f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.32f, 0.48f, 1.0f));
        }

        if (ImGui::Button(categories[c])) {
            active_category_ = c;
        }
        ImGui::PopStyleColor(2);
    }
    ImGui::PopStyleVar(2);
    ImGui::Separator();

    // Render Sub-Tabs for Active Category
    if (ImGui::BeginTabBar("SubCategoryTabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
        if (active_category_ == 0) { // GOV: Politics & State
            if (ImGui::BeginTabItem("Presidential Oval Office")) { draw_president_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Individual Citizens"))      { draw_citizens_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Royal Dynasties & Lineage")){ draw_dynasty_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Congress & Parliament"))    { draw_parliament_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("United Nations Council"))   { draw_un_council_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("NATO & OPEC Alliances"))    { draw_alliances_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Demographics & Transit"))   { draw_demographics_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Internal Factions"))        { draw_factions_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("National Policies"))        { draw_policies_tab(engine); ImGui::EndTabItem(); }
        }
        else if (active_category_ == 1) { // DEF: Defense & Tech
            if (ImGui::BeginTabItem("AI Agent & Governor"))      { draw_agent_control_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Military & Logistics"))     { draw_military_logistics_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Civil Wars & Rebellions"))  { draw_rebellion_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Nuclear Triad & MAD"))      { draw_nuclear_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Space Race & Mining"))      { draw_space_race_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Technology Tree"))          { draw_tech_tree_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Coalitions & Espionage"))   { draw_coalitions_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Empires & AI Rulers"))      { draw_empires_tab(engine); ImGui::EndTabItem(); }
        }
        else if (active_category_ == 2) { // ECON: Market & Industry
            if (ImGui::BeginTabItem("Central Bank & Currencies")){ draw_central_banking_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Stock Exchange & Bank"))    { draw_stock_market_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Mega-Engineering Wonders")) { draw_megawonders_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Maritime Shipping & Piracy")){ draw_maritime_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Supply & Demand Physics"))  { draw_supply_demand_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Commodity Prices & Trade")) { draw_economy_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Caravans & Map Nodes"))     { draw_trade_routes_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("World Wonders Engine"))     { draw_wonders_tab(engine); ImGui::EndTabItem(); }
        }
        else if (active_category_ == 3) { // MAP: Map & Disasters
            if (ImGui::BeginTabItem("Interactive 2D World Map")) { draw_world_map_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("GIS Elevation & Climate"))  { draw_gis_climate_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Global Disasters"))         { draw_disasters_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Naval Fleets & Rigs"))      { draw_naval_tab(engine); ImGui::EndTabItem(); }
        }
        else if (active_category_ == 4) { // DIPL: Diplomacy & Logs
            if (ImGui::BeginTabItem("LLM Ruler Negotiation"))    { draw_ruler_chat_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("World Religions & Faith"))  { draw_religion_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Historical Analytics"))     { draw_analytics_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Imperial Chronicle"))       { draw_chronicle_tab(engine); ImGui::EndTabItem(); }
            if (ImGui::BeginTabItem("Save / Load Universe"))     { draw_persistence_tab(engine); ImGui::EndTabItem(); }
        }
        else if (active_category_ == 5) { // GOD: Divine Powers
            if (ImGui::BeginTabItem("God Mode Control Console")) { draw_god_mode_tab(engine); ImGui::EndTabItem(); }
        }
        ImGui::EndTabBar();
    }

    draw_interactive_event_modal(engine);

    ImGui::End();

    // Rendering
    ImGui::Render();
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.10f, 0.12f, 0.15f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_);
}

// ─── Controls Header ──────────────────────────────────────────────────────────
void AeonGUI::draw_control_bar(AeonEngine& engine) {
    ImGui::Text("AEON SIMULATOR  |  Year %d  |  Season: %s", engine.year, engine.current_season());
    ImGui::SameLine(330);

    if (engine.paused) {
        if (ImGui::Button(" ▶ Resume ")) engine.paused = false;
    } else {
        if (ImGui::Button(" ⏸ Pause ")) engine.paused = true;
    }

    ImGui::SameLine();
    if (ImGui::Button(" Step 1 Yr ")) {
        engine.tick_second(1.0f / engine.speed);
    }

    ImGui::SameLine();
    if (ImGui::Button(" Run 10 Yrs ")) {
        for (int i = 0; i < 10; ++i) engine.tick_second(1.0f / engine.speed);
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("Speed", &engine.speed, 1.0f, 100.0f, "%.0fx");

    // 👑 Active Empire Selector Dropdown
    ImGui::SameLine();
    ImGui::Text(" |  Empire:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    std::vector<const char*> empire_names;
    for (const auto& c : engine.civs) {
        if (c.id < 5) empire_names.push_back(c.name.c_str());
    }
    int current_civ = engine.president_game.player_civ_id;
    if (current_civ < 0 || current_civ >= (int)empire_names.size()) current_civ = 0;

    if (!empire_names.empty() && ImGui::Combo("##ActiveEmpire", &current_civ, empire_names.data(), (int)empire_names.size())) {
        selected_civ_id_ = current_civ;
        engine.president_game.player_civ_id = current_civ;
    }

    ImGui::SameLine();
    if (ImGui::Button(" 🎲 Random Empire ")) {
        int r_civ = rand() % (empire_names.empty() ? 1 : (int)empire_names.size());
        selected_civ_id_ = r_civ;
        engine.president_game.player_civ_id = r_civ;
    }


    ImGui::SameLine();
    bool ollama_ok = AeonOllama::is_available();
    if (ollama_ok) {
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), " [Ollama Active]");
    } else {
        ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.2f, 1.0f), " [Rule Fallback]");
    }

    // 📊 Executive Telemetry Metrics Row
    int active_wars = 0;
    for (const auto& c : engine.civs) {
        if (c.at_war) active_wars++;
    }
    float avg_temp = engine.gis_climate_engine.get_temperature_at(MAP_WIDTH / 2, MAP_HEIGHT / 2);

    ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "LIVE TELEMETRY:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "🌡️ Global Temp: %.1f°C", avg_temp);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), " |  📈 Market Index: %.0f Pts", engine.economy_market_engine.market_index_points);
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), " |  ⚔️ Active Wars: %d", active_wars / 2);
    ImGui::SameLine();
    ImGui::TextColored(engine.nuclear_engine.global_nuclear_winter ? ImVec4(1.0f,0.2f,0.2f,1.0f) : ImVec4(0.3f,0.9f,0.3f,1.0f),
        " |  ☢️ DEFCON: %s", engine.nuclear_engine.global_nuclear_winter ? "1 (NUCLEAR WINTER)" : "5 (SAFE)");
}

// ─── National Policies & Diplomacy Tab ───────────────────────────────────────
void AeonGUI::draw_policies_tab(AeonEngine& engine) {
    auto& pg  = engine.president_game;
    auto& civ = engine.civs[pg.player_civ_id];

    // ── Header status bar ──
    ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f),
        "Nation: %s   |   Treasury: %.0f Gold   |   Stability: %.0f%%   |   Army: %.0f",
        civ.name.c_str(), pg.treasury_gold, civ.stability, civ.army_size);
    ImGui::Separator();

    ImGui::Columns(3, "PoliciesColumns", true);

    // ════════════════════════════════════════════════════════
    // COLUMN 1: INFRASTRUCTURE INVESTMENTS
    // ════════════════════════════════════════════════════════
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "🏗️ INFRASTRUCTURE");
    ImGui::Separator();
    ImGui::TextDisabled("Spend Gold to develop your nation");
    ImGui::Spacing();

    struct Infra { const char* icon; const char* name; const char* desc; float cost; };
    static const Infra infra_list[] = {
        {"🛤️",  "Build Road Network",      "+GDP 15%, +Trade",           300.0f},
        {"🏥",  "National Hospital Fund",   "+Pop Growth +0.3%%",         250.0f},
        {"🏫",  "University Expansion",     "+Tech Research +20/yr",      400.0f},
        {"🏰",  "Frontier Fortresses",      "+Army 1500, +Defense",       350.0f},
        {"🌾",  "Agricultural Reform",      "+Food 500, +Stability 8",    200.0f},
        {"⚡",  "Power Grid Project",       "+GDP 10%, +Happiness 5",     500.0f},
        {"🚢",  "Naval Expansion",          "+Trade 20%, +Territory",     450.0f},
        {"🌆",  "City Beautification",      "+Happiness 10, +Stability 5",150.0f},
    };

    for (const auto& inf : infra_list) {
        bool can_afford = pg.treasury_gold >= inf.cost;
        if (!can_afford) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f,0.1f,0.1f,1));
        else             ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f,0.35f,0.15f,1));

        std::string btn_label = std::string(inf.icon) + " " + inf.name + " (" + std::to_string((int)inf.cost) + "g)";
        if (ImGui::Button(btn_label.c_str(), ImVec2(-1, 0)) && can_afford) {
            pg.treasury_gold -= inf.cost;
            // Apply outcomes
            if (std::string(inf.name).find("Road")     != std::string::npos) { civ.economy.gdp *= 1.15f; }
            if (std::string(inf.name).find("Hospital") != std::string::npos) { civ.population.birth_rate += 0.003f; }
            if (std::string(inf.name).find("University")!= std::string::npos){ civ.tech.research_pts += 200.0f; }
            if (std::string(inf.name).find("Fortress") != std::string::npos) { civ.army_size += 1500.0f; civ.military_power += 150.0f; }
            if (std::string(inf.name).find("Agricultural")!=std::string::npos){ civ.resources.food += 500.0f; civ.stability = std::min(100.0f, civ.stability + 8.0f); }
            if (std::string(inf.name).find("Power")    != std::string::npos) { civ.economy.gdp *= 1.10f; civ.population.happiness = std::min(100.0f, civ.population.happiness + 5.0f); }
            if (std::string(inf.name).find("Naval")    != std::string::npos) { civ.territory_tiles += 8.0f; }
            if (std::string(inf.name).find("City")     != std::string::npos) { civ.population.happiness = std::min(100.0f, civ.population.happiness + 10.0f); civ.stability = std::min(100.0f, civ.stability + 5.0f); }
            pg.last_news_headline = std::string("🏗️ PROJECT: ") + inf.name + " completed! " + inf.desc;
            engine.history.record(engine.year, engine.month, "POLICY",
                std::string(inf.name) + " enacted", inf.desc, pg.player_civ_id);
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.3f,1,0.3f,1), "Effect: %s", inf.desc);
            ImGui::Text("Cost: %.0f Gold", inf.cost);
            ImGui::EndTooltip();
        }
    }

    ImGui::NextColumn();

    // ════════════════════════════════════════════════════════
    // COLUMN 2: NATIONAL EDICTS
    // ════════════════════════════════════════════════════════
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "📜 NATIONAL EDICTS");
    ImGui::Separator();
    ImGui::TextDisabled("Policies with tradeoffs");
    ImGui::Spacing();

    struct Edict { const char* icon; const char* name; const char* plus; const char* minus; float cost; };
    static const Edict edicts[] = {
        {"⚔️",  "Emergency Conscription",  "+Army 3000",         "-Happiness 15",  100.0f},
        {"📢",  "Propaganda Campaign",      "+Approval 15%%",     "-Stability 5",   150.0f},
        {"🌾",  "Food Rationing",           "+Food 300",          "-Happiness 10",   80.0f},
        {"🔓",  "Open Borders Policy",      "+Pop Growth +0.5%%", "-Security 10",    50.0f},
        {"🔒",  "Border Lockdown",          "+Stability 10",      "-Trade 20%%",    100.0f},
        {"💰",  "Emergency Tax Levy",       "+Gold 400",          "-Happiness 12",    0.0f},
        {"🛡️",  "Martial Law",             "+Stability 20",      "-Happiness 20",  200.0f},
        {"🎉",  "National Festival",        "+Happiness 20",      "-Gold 200",      200.0f},
        {"🔬",  "Science Mobilization",     "+Research 300",      "-Economy 5%%",   250.0f},
        {"🕊️",  "Amnesty Declaration",      "+Stability 15",      "-Military 500",   50.0f},
    };

    for (const auto& ed : edicts) {
        bool can_afford = pg.treasury_gold >= ed.cost;
        if (!can_afford) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f,0.1f,0.1f,1));
        else             ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f,0.20f,0.05f,1));

        std::string lbl = std::string(ed.icon) + " " + ed.name;
        if (ed.cost > 0) lbl += " (" + std::to_string((int)ed.cost) + "g)";
        if (ImGui::Button(lbl.c_str(), ImVec2(-1, 0)) && can_afford) {
            pg.treasury_gold -= ed.cost;
            std::string nm(ed.name);
            if (nm.find("Conscription") != std::string::npos) {
                civ.army_size += 3000.0f; civ.population.happiness = std::max(10.0f, civ.population.happiness - 15.0f);
            } else if (nm.find("Propaganda") != std::string::npos) {
                pg.approval_rating = std::min(100.0f, pg.approval_rating + 15.0f);
                civ.stability = std::max(15.0f, civ.stability - 5.0f);
            } else if (nm.find("Rationing") != std::string::npos) {
                civ.resources.food += 300.0f; civ.population.happiness = std::max(10.0f, civ.population.happiness - 10.0f);
            } else if (nm.find("Open Borders") != std::string::npos) {
                civ.population.birth_rate += 0.005f;
            } else if (nm.find("Lockdown") != std::string::npos) {
                civ.stability = std::min(100.0f, civ.stability + 10.0f); civ.economy.gdp *= 0.80f;
            } else if (nm.find("Tax Levy") != std::string::npos) {
                pg.treasury_gold += 400.0f; civ.population.happiness = std::max(10.0f, civ.population.happiness - 12.0f);
            } else if (nm.find("Martial Law") != std::string::npos) {
                civ.stability = std::min(100.0f, civ.stability + 20.0f); civ.population.happiness = std::max(10.0f, civ.population.happiness - 20.0f);
            } else if (nm.find("Festival") != std::string::npos) {
                pg.treasury_gold -= 200.0f; civ.population.happiness = std::min(100.0f, civ.population.happiness + 20.0f);
            } else if (nm.find("Science") != std::string::npos) {
                civ.tech.research_pts += 300.0f; civ.economy.gdp *= 0.95f;
            } else if (nm.find("Amnesty") != std::string::npos) {
                civ.stability = std::min(100.0f, civ.stability + 15.0f); civ.army_size = std::max(0.0f, civ.army_size - 500.0f);
            }
            pg.last_news_headline = std::string("📜 EDICT: ") + ed.name + " enacted! " + ed.plus + " | " + ed.minus;
            engine.history.record(engine.year, engine.month, "EDICT", nm, std::string(ed.plus) + " / " + ed.minus, pg.player_civ_id);
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextColored(ImVec4(0.3f,1,0.3f,1), "✅ %s", ed.plus);
            ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "❌ %s", ed.minus);
            if (ed.cost > 0) ImGui::Text("Cost: %.0f Gold", ed.cost);
            ImGui::EndTooltip();
        }
    }

    ImGui::NextColumn();

    // ════════════════════════════════════════════════════════
    // COLUMN 3: DIPLOMACY & FOREIGN ACTIONS
    // ════════════════════════════════════════════════════════
    ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "🤝 DIPLOMACY");
    ImGui::Separator();
    ImGui::TextDisabled("Interact with other nations");
    ImGui::Spacing();

    for (int i = 0; i < (int)engine.civs.size(); ++i) {
        auto& target = engine.civs[i];
        if (i == pg.player_civ_id || target.is_alive <= 0.0f) continue;

        // Relation status
        DiplomacyStatus rel = DiplomacyStatus::NEUTRAL;
        auto it = civ.relations.find(i);
        if (it != civ.relations.end()) rel = it->second;

        const char* rel_str = "Neutral";
        ImVec4 rel_col = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        if (rel == DiplomacyStatus::ALLY)    { rel_str = "Allied ✅";     rel_col = {0.2f,1,0.2f,1}; }
        if (rel == DiplomacyStatus::AT_WAR)  { rel_str = "AT WAR ⚔️";    rel_col = {1,0.2f,0.2f,1}; }
        if (rel == DiplomacyStatus::RIVAL)   { rel_str = "Rival ⚡";      rel_col = {1,0.6f,0.2f,1}; }
        if (rel == DiplomacyStatus::EMBARGOED){ rel_str = "Embargoed 🚫"; rel_col = {0.8f,0.4f,0.1f,1}; }

        ImGui::TextColored(ImVec4(1,1,0.4f,1), "── %s", target.name.c_str());
        ImGui::SameLine();
        ImGui::TextColored(rel_col, "[%s]", rel_str);
        ImGui::Text("  Pop: %lldM | Army: %.0f | Stab: %.0f%%",
            target.population.total/1000000, target.army_size, target.stability);

        // Action buttons — 2 per row
        ImGui::PushID(i * 100 + 1);
        if (rel != DiplomacyStatus::ALLY && rel != DiplomacyStatus::AT_WAR) {
            if (ImGui::SmallButton("🤝 Ally (-200g)") && pg.treasury_gold >= 200.0f) {
                pg.treasury_gold -= 200.0f;
                civ.relations[i]   = DiplomacyStatus::ALLY;
                target.relations[pg.player_civ_id] = DiplomacyStatus::ALLY;
                pg.last_news_headline = "🤝 ALLIANCE: " + civ.name + " and " + target.name + " form a mutual defense pact!";
                engine.history.record(engine.year, engine.month, "DIPLOMACY", "Alliance formed with " + target.name, "", pg.player_civ_id, i);
            }
        }
        ImGui::PopID();
        ImGui::SameLine();

        ImGui::PushID(i * 100 + 2);
        if (pg.treasury_gold >= 150.0f) {
            if (ImGui::SmallButton("💰 Send Aid (-150g)")) {
                pg.treasury_gold -= 150.0f;
                target.stability = std::min(100.0f, target.stability + 15.0f);
                target.population.happiness = std::min(100.0f, target.population.happiness + 10.0f);
                civ.relations[i] = DiplomacyStatus::ALLY;
                pg.last_news_headline = "💰 AID: Sent 150 Gold in foreign aid to " + target.name + ". Relations improved!";
            }
        }
        ImGui::PopID();

        if (rel == DiplomacyStatus::AT_WAR) {
            ImGui::PushID(i * 100 + 3);
            if (ImGui::SmallButton("🕊️ Offer Peace")) {
                civ.at_war = false; civ.war_with_civ = -1;
                target.at_war = false; target.war_with_civ = -1;
                civ.relations[i]   = DiplomacyStatus::RIVAL;
                target.relations[pg.player_civ_id] = DiplomacyStatus::RIVAL;
                pg.last_news_headline = "🕊️ PEACE OFFER accepted: ceasefire with " + target.name + "!";
                engine.history.record(engine.year, engine.month, "WAR", "Peace with " + target.name, "Player-initiated ceasefire.", pg.player_civ_id, i);
            }
            ImGui::PopID();
            ImGui::SameLine();

            // Demand tribute if winning
            ImGui::PushID(i * 100 + 4);
            if (civ.military_power > target.military_power) {
                if (ImGui::SmallButton("👑 Demand Tribute")) {
                    float tribute = target.economy.gdp * 0.10f;
                    pg.treasury_gold += tribute;
                    target.economy.gdp *= 0.90f;
                    target.stability = std::max(15.0f, target.stability - 10.0f);
                    pg.last_news_headline = "👑 TRIBUTE: Collected " + std::to_string(int(tribute)) + " Gold from " + target.name + " as war reparations!";
                }
            }
            ImGui::PopID();
        }

        // Spy action: destabilize
        ImGui::PushID(i * 100 + 5);
        if (ImGui::SmallButton("🕵️ Destabilize (-100g)") && pg.treasury_gold >= 100.0f) {
            pg.treasury_gold -= 100.0f;
            float effect = 8.0f + float((engine.year * 7 + i * 3) % 15);
            target.stability = std::max(15.0f, target.stability - effect);
            target.population.happiness = std::max(10.0f, target.population.happiness - 8.0f);
            pg.last_news_headline = "🕵️ COVERT OP: Agents destabilized " + target.name + " by " + std::to_string(int(effect)) + "% stability!";
        }
        ImGui::PopID();
        ImGui::SameLine();

        // Annex puppet (if ally and they're small)
        ImGui::PushID(i * 100 + 6);
        if (rel == DiplomacyStatus::ALLY && target.territory_tiles < civ.territory_tiles * 0.3f) {
            if (ImGui::SmallButton("🏴 Annex Puppet")) {
                civ.territory_tiles += target.territory_tiles;
                civ.population.total += target.population.total;
                pg.treasury_gold += 200.0f;
                target.is_alive = 0.0f;
                civ.relations.erase(i);
                pg.last_news_headline = "🏴 ANNEXATION: " + target.name + " peacefully integrated into " + civ.name + "!";
                engine.history.record(engine.year, engine.month, "COLLAPSE",
                    target.name + " annexed by " + civ.name, "Peaceful annexation of a puppet state.", pg.player_civ_id, i);
            }
        }
        ImGui::PopID();
        ImGui::Separator();
    }

    ImGui::Columns(1);
}


// ─── OpenStreetMap PNG Picture Map GPU Texture Loader ─────────────────────────
static GLuint load_png_to_gl_texture(const wchar_t* wpath) {
    static bool gdi_initialized = false;
    if (!gdi_initialized) {
        ULONG_PTR token;
        Gdiplus::GdiplusStartupInput gdiInput;
        Gdiplus::GdiplusStartup(&token, &gdiInput, NULL);
        gdi_initialized = true;
    }

    Gdiplus::Bitmap bitmap(wpath);
    if (bitmap.GetLastStatus() != Gdiplus::Ok) {
        return 0;
    }

    UINT width  = bitmap.GetWidth();
    UINT height = bitmap.GetHeight();
    if (width == 0 || height == 0) return 0;

    Gdiplus::Rect rect(0, 0, width, height);
    Gdiplus::BitmapData bmpData;
    if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bmpData) != Gdiplus::Ok) {
        return 0;
    }

    std::vector<uint8_t> rgba(width * height * 4);
    const uint8_t* bgra = static_cast<const uint8_t*>(bmpData.Scan0);
    int stride = bmpData.Stride;

    for (UINT y = 0; y < height; ++y) {
        for (UINT x = 0; x < width; ++x) {
            size_t src_idx = y * stride + x * 4;
            size_t dst_idx = (y * width + x) * 4;
            rgba[dst_idx + 0] = bgra[src_idx + 2]; // R
            rgba[dst_idx + 1] = bgra[src_idx + 1]; // G
            rgba[dst_idx + 2] = bgra[src_idx + 0]; // B
            rgba[dst_idx + 3] = bgra[src_idx + 3]; // A
        }
    }

    bitmap.UnlockBits(&bmpData);

    GLuint tex_id = 0;
    glGenTextures(1, &tex_id);
    glBindTexture(GL_TEXTURE_2D, tex_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    return tex_id;
}

void AeonGUI::load_osm_map_picture_textures() {
    if (osm_dark_texture_id_ == 0) {
        osm_dark_texture_id_ = load_png_to_gl_texture(L"assets/osm_dark_world.png");
    }
    if (osm_street_texture_id_ == 0) {
        osm_street_texture_id_ = load_png_to_gl_texture(L"assets/osm_street_world.png");
    }
    if (osm_satellite_texture_id_ == 0) {
        osm_satellite_texture_id_ = load_png_to_gl_texture(L"assets/osm_satellite_world.png");
    }
}

// ─── World Map Tab (Real OpenStreetMap & Satellite Picture API Map) ──────────
void AeonGUI::draw_world_map_tab(AeonEngine& engine) {
    // ── Left panel: interactive 2D color tile map ──
    float panel_w = ImGui::GetContentRegionAvail().x - 250.0f;
    float panel_h = ImGui::GetContentRegionAvail().y;

    ImGui::BeginChild("ColorMapCanvas", ImVec2(panel_w, panel_h), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 canvas_pos  = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    ImDrawList* dl     = ImGui::GetWindowDrawList();

    // Dark background
    dl->AddRectFilled(canvas_pos,
        ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
        IM_COL32(10, 12, 18, 255));

    // Ensure real OpenStreetMap & Satellite GPU picture textures are loaded
    load_osm_map_picture_textures();

    GLuint active_tex = 0;
    if (osm_map_provider_ == 0 && osm_dark_texture_id_ != 0) active_tex = osm_dark_texture_id_;
    else if (osm_map_provider_ == 1 && osm_street_texture_id_ != 0) active_tex = osm_street_texture_id_;
    else if (osm_map_provider_ == 2 && osm_satellite_texture_id_ != 0) active_tex = osm_satellite_texture_id_;
    else active_tex = osm_dark_texture_id_;


    // ── Mouse wheel zoom ──
    if (ImGui::IsWindowHovered()) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            float old_zoom = map_zoom_;
            map_zoom_ = std::max(2.0f, std::min(20.0f, map_zoom_ + wheel * 0.5f));
            // Zoom toward mouse cursor
            ImVec2 mouse = ImGui::GetIO().MousePos;
            float mx = (mouse.x - canvas_pos.x - map_pan_x_) / old_zoom;
            float my = (mouse.y - canvas_pos.y - map_pan_y_) / old_zoom;
            map_pan_x_ -= mx * (map_zoom_ - old_zoom);
            map_pan_y_ -= my * (map_zoom_ - old_zoom);
        }
    }

    // ── Click-drag panning ──
    ImGui::InvisibleButton("MapDragCatcher", canvas_size);
    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            map_dragging_ = true;
            map_drag_start_x_ = ImGui::GetIO().MousePos.x;
            map_drag_start_y_ = ImGui::GetIO().MousePos.y;
            map_pan_start_x_ = map_pan_x_;
            map_pan_start_y_ = map_pan_y_;
        }
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            map_pan_x_ += ImGui::GetIO().MouseDelta.x;
            map_pan_y_ += ImGui::GetIO().MouseDelta.y;
        }
    }
    if (map_dragging_) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            map_pan_x_ = map_pan_start_x_ + (ImGui::GetIO().MousePos.x - map_drag_start_x_);
            map_pan_y_ = map_pan_start_y_ + (ImGui::GetIO().MousePos.y - map_drag_start_y_);
        } else {
            map_dragging_ = false;
        }
    }

    float ts = map_zoom_; // tile size in pixels
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    int hovered_tx = -1, hovered_ty = -1;

    // ── Render Photorealistic Real Earth OpenStreetMap / Satellite Picture Texture ──
    ImVec2 map_min = ImVec2(canvas_pos.x + map_pan_x_, canvas_pos.y + map_pan_y_);
    ImVec2 map_max = ImVec2(map_min.x + MAP_WIDTH * ts, map_min.y + MAP_HEIGHT * ts);
    if (active_tex != 0) {
        dl->AddImage((ImTextureID)(uintptr_t)active_tex, map_min, map_max);
    }

    // Calculate hovered tile index over satellite picture
    if (mouse_pos.x >= map_min.x && mouse_pos.x < map_max.x &&
        mouse_pos.y >= map_min.y && mouse_pos.y < map_max.y) {
        hovered_tx = int((mouse_pos.x - map_min.x) / ts);
        hovered_ty = int((mouse_pos.y - map_min.y) / ts);
        hovered_tx = std::max(0, std::min(MAP_WIDTH - 1, hovered_tx));
        hovered_ty = std::max(0, std::min(MAP_HEIGHT - 1, hovered_ty));

        // Highlight hovered tile boundary
        float hx = map_min.x + hovered_tx * ts;
        float hy = map_min.y + hovered_ty * ts;
        dl->AddRect(ImVec2(hx, hy), ImVec2(hx + ts, hy + ts), IM_COL32(255, 255, 255, 220), 0.0f, 0, 1.5f);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            selected_tile_x_ = hovered_tx;
            selected_tile_y_ = hovered_ty;
        }
    }

    // Selected tile highlight box
    if (selected_tile_x_ >= 0 && selected_tile_y_ >= 0) {
        float sx = map_min.x + selected_tile_x_ * ts;
        float sy = map_min.y + selected_tile_y_ * ts;
        dl->AddRect(ImVec2(sx, sy), ImVec2(sx + ts, sy + ts), IM_COL32(255, 220, 0, 255), 0.0f, 0, 2.0f);
    }


    // ── Draw civilization capitals as glowing dots ──
    for (const auto& civ : engine.civs) {
        if (civ.is_alive <= 0.0f) continue;
        float px = canvas_pos.x + map_pan_x_ + civ.capital_x * ts + ts * 0.5f;
        float py = canvas_pos.y + map_pan_y_ + civ.capital_y * ts + ts * 0.5f;
        float r  = std::max(3.0f, ts * 0.5f);

        static const ImU32 civ_colors[] = {
            IM_COL32(255, 80, 80, 255),
            IM_COL32( 80,140,255, 255),
            IM_COL32(255,210, 30, 255),
            IM_COL32( 80,220, 80, 255),
            IM_COL32(210, 80,240, 255),
            IM_COL32(255,150, 30, 255),
        };
        ImU32 cc = civ_colors[civ.id % 6];

        // Glow ring
        dl->AddCircleFilled(ImVec2(px,py), r + 3.0f, IM_COL32(255,255,255,40));
        dl->AddCircleFilled(ImVec2(px,py), r, cc);
        dl->AddCircle(ImVec2(px,py), r, IM_COL32(255,255,255,200), 0, 1.5f);

        // Label when zoomed in
        if (ts >= 6.0f) {
            dl->AddText(ImVec2(px + r + 2.0f, py - 6.0f),
                civ.at_war ? IM_COL32(255,80,80,255) : IM_COL32(255,255,255,230),
                civ.name.c_str());
        }
    }

    // ── Draw Real Earth Strategic Landmarks & Canals ──
    for (const auto& lm : engine.world_map.landmarks) {
        float px = canvas_pos.x + map_pan_x_ + lm.x * ts + ts * 0.5f;
        float py = canvas_pos.y + map_pan_y_ + lm.y * ts + ts * 0.5f;
        if (px < canvas_pos.x || px > canvas_pos.x + canvas_size.x) continue;
        if (py < canvas_pos.y || py > canvas_pos.y + canvas_size.y) continue;

        // Golden diamond landmark marker
        float lm_r = std::max(2.5f, ts * 0.4f);
        dl->AddCircleFilled(ImVec2(px, py), lm_r + 2.0f, IM_COL32(255, 215, 0, 80));
        dl->AddCircleFilled(ImVec2(px, py), lm_r, IM_COL32(255, 215, 0, 255));
        dl->AddCircle(ImVec2(px, py), lm_r, IM_COL32(255, 255, 255, 220), 0, 1.0f);

        if (ts >= 7.0f) {
            dl->AddText(ImVec2(px + lm_r + 2.0f, py - 5.0f), IM_COL32(255, 235, 120, 240), lm.name.c_str());
        }
    }


    // ── Draw Autonomous Controllable Agent Avatar & Path ──
    const auto& agent = engine.agent_engine.agent;
    float a_px = canvas_pos.x + map_pan_x_ + agent.x * ts + ts * 0.5f;
    float a_py = canvas_pos.y + map_pan_y_ + agent.y * ts + ts * 0.5f;
    float a_r  = std::max(4.0f, ts * 0.6f);

    // Draw path trace trail
    for (size_t i = 1; i < agent.path_history.size(); ++i) {
        float p1_x = canvas_pos.x + map_pan_x_ + agent.path_history[i-1].first * ts + ts * 0.5f;
        float p1_y = canvas_pos.y + map_pan_y_ + agent.path_history[i-1].second * ts + ts * 0.5f;
        float p2_x = canvas_pos.x + map_pan_x_ + agent.path_history[i].first * ts + ts * 0.5f;
        float p2_y = canvas_pos.y + map_pan_y_ + agent.path_history[i].second * ts + ts * 0.5f;
        dl->AddLine(ImVec2(p1_x, p1_y), ImVec2(p2_x, p2_y), IM_COL32(0, 240, 255, 120), 2.0f);
    }

    // Glowing cyan/yellow agent star avatar
    dl->AddCircleFilled(ImVec2(a_px, a_py), a_r + 4.0f, IM_COL32(0, 255, 255, 60));
    dl->AddCircleFilled(ImVec2(a_px, a_py), a_r, IM_COL32(255, 230, 0, 255));
    dl->AddCircle(ImVec2(a_px, a_py), a_r + 1.0f, IM_COL32(255, 255, 255, 255), 0, 2.0f);
    if (ts >= 5.0f) {
        dl->AddText(ImVec2(a_px + a_r + 3.0f, a_py - 7.0f), IM_COL32(0, 255, 255, 255), "⭐ Agent Envoy");
    }

    // ── Hover tooltip ──
    if (hovered_tx >= 0 && hovered_ty >= 0 && ImGui::IsWindowHovered()) {
        const MapTile& ht = engine.world_map.tile(hovered_tx, hovered_ty);
        ImGui::BeginTooltip();
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f),
            "Tile [%d, %d] — %s", hovered_tx, hovered_ty, biome_name(ht.biome));
        ImGui::Text("Elevation: %.2f  Moisture: %.2f  Fertility: %.2f",
            ht.elevation, ht.moisture, ht.fertility);
        if (ht.owner_civ >= 0 && ht.owner_civ < (int)engine.civs.size())
            ImGui::TextColored(ImVec4(1,0.8f,0.3f,1), "Owner: %s",
                engine.civs[ht.owner_civ].name.c_str());
        if (!ht.resources.empty()) {
            ImGui::Text("Resources:");
            for (auto r : ht.resources)
                ImGui::BulletText("%s", resource_name(r));
        }
        ImGui::EndTooltip();
    }

    // ── Mini controls overlay ──
    dl->AddRectFilled(
        ImVec2(canvas_pos.x + 6, canvas_pos.y + 6),
        ImVec2(canvas_pos.x + 200, canvas_pos.y + 48),
        IM_COL32(0,0,0,160), 6.0f);
    dl->AddText(ImVec2(canvas_pos.x + 12, canvas_pos.y + 10),
        IM_COL32(200,240,255,240),
        "🖱 Drag: pan   Wheel: zoom   Right-click: inspect");
    char zoom_str[32];
    snprintf(zoom_str, sizeof(zoom_str), "Zoom: %.1fx", map_zoom_ / 6.0f);
    dl->AddText(ImVec2(canvas_pos.x + 12, canvas_pos.y + 30),
        IM_COL32(160,200,200,200), zoom_str);

    ImGui::EndChild();

    ImGui::SameLine();

    // ── Right panel: OpenStreetMap provider selector + legend + tile inspector ──
    ImGui::BeginChild("MapPanel", ImVec2(0, 0), true);

    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "🌐 OPENSTREETMAP API PROVIDERS");
    ImGui::RadioButton("CartoDB Dark (OSM API)", &osm_map_provider_, 0);
    ImGui::RadioButton("Standard OpenStreetMap", &osm_map_provider_, 1);
    ImGui::RadioButton("Esri World Satellite", &osm_map_provider_, 2);
    ImGui::Separator();

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "🔮 MAP OVERLAY MODES");
    ImGui::RadioButton("Sovereign Borders", &map_overlay_mode_, 0);
    ImGui::RadioButton("GIS Climate/Temp", &map_overlay_mode_, 1);
    ImGui::RadioButton("Refugees & Pop", &map_overlay_mode_, 2);
    ImGui::RadioButton("Military & Fronts", &map_overlay_mode_, 3);
    ImGui::Separator();


    ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "🗺️ MAP LEGEND");
    ImGui::Separator();

    struct BiomeLegend { const char* name; ImU32 col; };
    static const BiomeLegend legend[] = {
        {"🌊 Deep Ocean",    IM_COL32( 10, 50,120,255)},
        {"🟦 Shallow Sea",   IM_COL32( 30, 80,160,255)},
        {"🏖️ Beach/Coast",   IM_COL32(200,190,150,255)},
        {"🌿 Plains",        IM_COL32(170,200, 90,255)},
        {"🌾 Grassland",     IM_COL32(100,180, 60,255)},
        {"🌳 Forest",        IM_COL32( 34,100, 34,255)},
        {"🌴 Jungle",        IM_COL32( 10, 60, 20,255)},
        {"🏜️ Desert",        IM_COL32(220,190,120,255)},
        {"⛰️ Mountains",     IM_COL32(130,120,130,255)},
        {"🌋 Volcano",       IM_COL32(200, 50, 10,255)},
        {"❄️ Snow/Ice",      IM_COL32(230,245,255,255)},
        {"🌿 Swamp/Marsh",   IM_COL32( 60, 80, 30,255)},
    };
    for (const auto& le : legend) {
        ImGui::ColorButton("##b", ImVec4(
            ((le.col>>0)&0xFF)/255.0f,
            ((le.col>>8)&0xFF)/255.0f,
            ((le.col>>16)&0xFF)/255.0f, 1.0f),
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker,
            ImVec2(14,14));
        ImGui::SameLine();
        ImGui::Text("%s", le.name);
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "🏙️ CIVILIZATION CAPITALS");
    static const ImU32 civ_legend_col[] = {
        IM_COL32(255, 80, 80, 255), IM_COL32(80,140,255,255),
        IM_COL32(255,210, 30,255),  IM_COL32(80,220, 80,255),
        IM_COL32(210, 80,240,255),  IM_COL32(255,150, 30,255),
    };
    for (size_t i = 0; i < engine.civs.size(); ++i) {
        const auto& civ = engine.civs[i];
        if (civ.is_alive <= 0.0f) continue;
        ImGui::ColorButton("##c", ImVec4(
            ((civ_legend_col[i%6]>>0)&0xFF)/255.0f,
            ((civ_legend_col[i%6]>>8)&0xFF)/255.0f,
            ((civ_legend_col[i%6]>>16)&0xFF)/255.0f, 1.0f),
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoPicker,
            ImVec2(14,14));
        ImGui::SameLine();
        if (civ.at_war)
            ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "%s ⚔️  [%d,%d]",
                civ.name.c_str(), civ.capital_x, civ.capital_y);
        else
            ImGui::Text("%s  [%d,%d]", civ.name.c_str(), civ.capital_x, civ.capital_y);
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "🔍 SELECTED TILE");
    if (selected_tile_x_ >= 0 && selected_tile_y_ >= 0) {
        std::string tile_info = engine.inspect_tile(selected_tile_x_, selected_tile_y_);
        ImGui::Text("Position: [%d, %d]", selected_tile_x_, selected_tile_y_);
        ImGui::Separator();
        ImGui::TextUnformatted(tile_info.c_str());
    } else {
        ImGui::TextDisabled("Right-click a tile on the map\nto inspect it here.");
    }

    ImGui::EndChild();
}


// ─── Empires Tab ──────────────────────────────────────────────────────────────
void AeonGUI::draw_empires_tab(AeonEngine& engine) {
    ImGui::BeginChild("EmpiresList", ImVec2(400, 0), true);
    ImGui::Text("Civilizations");
    ImGui::Separator();

    for (size_t i = 0; i < engine.civs.size(); ++i) {
        const auto& c = engine.civs[i];
        std::string label = c.name + (c.at_war ? " [AT WAR]" : "");
        if (ImGui::Selectable(label.c_str(), selected_civ_id_ == (int)i)) {
            selected_civ_id_ = (int)i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("CivDetails", ImVec2(0, 0), true);
    if (selected_civ_id_ >= 0 && selected_civ_id_ < (int)engine.civs.size()) {
        auto& c  = engine.civs[selected_civ_id_];
        auto& ai = engine.ai_controllers[selected_civ_id_];

        ImGui::Text("Civilization: %s (#%d)", c.name.c_str(), c.id);
        ImGui::Separator();
        ImGui::Text("Government : %s", gov_form_name(c.government));
        ImGui::Text("Tech Era   : %s (%.0f%% progress)", tech_era_name(c.tech.era), c.tech.progress);
        ImGui::Text("Population : %lld k", c.population.total / 1000);
        ImGui::Text("Stability  : %.0f%%", c.stability);
        ImGui::Text("Army Size  : %.0f", c.army_size);
        ImGui::Text("GDP        : %.0f gold", c.economy.gdp);

        ImGui::Separator();
        ImGui::Text("Primary Goal: %s", ai.primary_goal.c_str());
        ImGui::Text("Hidden Goal : %s", ai.hidden_goal.c_str());

        ImGui::Separator();
        ImGui::Text("Assigned AI Model: %s", ai.model_name.c_str());
        ImGui::InputText("New Model", model_change_buf_, IM_ARRAYSIZE(model_change_buf_));
        ImGui::SameLine();
        if (ImGui::Button("Assign Model")) {
            ai.model_name = std::string(model_change_buf_);
        }

        // Ruler Psychological Profile & Trauma Matrix
        auto& psyche = engine.ruler_psyche_engine;
        for (const auto& prof : psyche.profiles) {
            if (prof.civ_id == c.id) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.9f, 0.4f, 1.0f, 1.0f), "🧠 Ruler Psychological Profile: %s", prof.ruler_name.c_str());
                ImGui::Text("   Mental Status: %s", prof.mental_state_status.c_str());

                ImGui::ProgressBar(prof.paranoia / 100.0f, ImVec2(0.0f, 0.0f), "Paranoia Level");
                ImGui::ProgressBar(prof.megalomania / 100.0f, ImVec2(0.0f, 0.0f), "Megalomania Level");
                ImGui::ProgressBar(prof.ptsd_trauma / 100.0f, ImVec2(0.0f, 0.0f), "War Trauma PTSD");
                ImGui::ProgressBar(prof.pragmatism / 100.0f, ImVec2(0.0f, 0.0f), "Pragmatism");

                if (ImGui::Button(("⚠️ Induce War Trauma Shock##" + std::to_string(c.id)).c_str())) {
                    psyche.trigger_war_trauma(c.id, 25.0f, engine);
                }
                ImGui::SameLine();
                if (ImGui::Button(("🚨 Order Cabinet Purge##" + std::to_string(c.id)).c_str())) {
                    psyche.trigger_cabinet_purge(c.id, engine);
                }
                break;
            }
        }

        // Google Gemini 2.5 Flash Strategic Brain Panel
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "✨ GOOGLE GEMINI 2.5 FLASH STRATEGIC BRAIN");
        static char gemini_key_buf[128] = "";
        if (!AeonGemini::is_configured()) {
            ImGui::InputText("Gemini API Key", gemini_key_buf, sizeof(gemini_key_buf), ImGuiInputTextFlags_Password);
            ImGui::SameLine();
            if (ImGui::Button("Set Key")) {
                AeonGemini::api_key = std::string(gemini_key_buf);
                std::ofstream f("gemini_key.txt");
                if (f.is_open()) { f << AeonGemini::api_key; }
            }

        } else {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "  Gemini 2.5 Flash API Key Configured! ✅");
        }

        if (ImGui::Button(("✨ Consult Gemini 2.5 Strategic Brain (" + c.name + ")").c_str())) {
            GeminiStrategyResponse res = AeonGemini::plan_ruler_strategy(
                "Sovereign of " + c.name, c.name, ai.primary_goal,
                "Global stability is " + std::to_string((int)c.stability) + "%",
                c.army_size, c.economy.reserve_currency_held, c.economy.gdp, c.at_war
            );
            engine.history.record(engine.year, engine.month, "GEMINI_AI",
                "Gemini 2.5 Flash Strategic Plan for " + c.name, res.strategic_plan, c.id);
        }

    }


    ImGui::EndChild();
}

// ─── Economy Tab ──────────────────────────────────────────────────────────────
void AeonGUI::draw_economy_tab(AeonEngine& engine) {
    auto& p = engine.president_game;
    float col_w = (ImGui::GetContentRegionAvail().x - 15.0f) * 0.5f;

    ImGui::BeginChild("EconLeft", ImVec2(col_w, 0), true);
    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "📈 GLOBAL COMMODITY PRICES & TRADE ROUTES");
    ImGui::Separator();
    ImGui::TextUnformatted(engine.market_engine.market_report().c_str());
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("EconRight", ImVec2(0, 0), true);
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "⚖️ PRESIDENTIAL TRADE DIPLOMACY & SANCTIONS");
    ImGui::Separator();

    for (size_t i = 0; i < engine.civs.size(); ++i) {
        if ((int)i == p.player_civ_id) continue;
        const auto& target = engine.civs[i];
        if (target.is_alive <= 0.0f) continue;

        ImGui::Text("Nation: %s (GDP: $%.0f)", target.name.c_str(), target.economy.gdp);

        char trade_btn[64], embargo_btn[64];
        snprintf(trade_btn, sizeof(trade_btn), "📦 Sign Trade Deal with %s", target.name.c_str());
        snprintf(embargo_btn, sizeof(embargo_btn), "🚫 Impose Sanctions & Embargo on %s", target.name.c_str());

        if (ImGui::Button(trade_btn, ImVec2(-1, 0))) {
            engine.market_engine.establish_manual_trade_route(engine.civs, p.player_civ_id, (int)i, engine.year);
        }
        ImGui::SetItemTooltip("Establish high-volume trade pact (+500 Gold/yr trade volume & boost GDP).");

        if (ImGui::Button(embargo_btn, ImVec2(-1, 0))) {
            engine.market_engine.cancel_trade_route_sanctions(engine.civs, p.player_civ_id, (int)i, engine.year);
        }
        ImGui::SetItemTooltip("Sever trade relations and slash target nation's GDP by -20%%.");
        ImGui::Separator();
    }
    ImGui::EndChild();
}

// ─── Factions Tab ─────────────────────────────────────────────────────────────
void AeonGUI::draw_factions_tab(AeonEngine& engine) {
    ImGui::Text("Internal Political Factions & Rebellion Risk");
    ImGui::Separator();

    for (const auto& c : engine.civs) {
        if (c.is_alive <= 0.0f) continue;
        if (ImGui::TreeNode(c.name.c_str())) {
            for (const auto& f : c.factions) {
                ImGui::Text("%s (%s)", f.name.c_str(), faction_type_name(f.type));
                ImGui::ProgressBar(f.influence / 100.0f, ImVec2(200, 0), "Influence");
                ImGui::SameLine();
                ImGui::ProgressBar(f.loyalty / 100.0f, ImVec2(200, 0), "Loyalty");
                ImGui::SameLine();
                ImGui::ProgressBar(f.rebellion_risk / 100.0f, ImVec2(200, 0), "Rebellion Risk");
            }
            ImGui::TreePop();
        }
    }
}

void AeonGUI::draw_religion_tab(AeonEngine& engine) {
    auto& r = engine.aeon_religion_engine;
    ImGui::TextColored(ImVec4(0.9f, 0.7f, 1.0f, 1.0f), "📜 WORLD RELIGIONS, FAITH & HOLY WAR CRUSADES");
    ImGui::Separator();

    ImGui::Text("Faith Points: %.0f | Active Missionaries: %d | Sacred Relics Collected: %zu",
        r.player_faith_points, r.active_missionaries, r.holy_relics_collected.size());
    ImGui::Separator();

    ImGui::Text("FOUNDED WORLD FAITHS:");
    for (const auto& rel : r.religions) {
        ImGui::Text("✝ %s (Seat: %s) | Followers: %.0f%% | Faith Power: %.0f | Relic: %s",
            rel.name.c_str(), rel.holy_city.c_str(), rel.global_followers_pct, rel.faith_power, rel.sacred_relic.c_str());
    }

    ImGui::Separator();
    ImGui::Text("SACRED ACTIONS:");
    if (ImGui::Button(" ✝ Found New World Religion (300 Faith) ", ImVec2(-1, 0))) {
        r.found_religion(engine, "Solaris Path of Light", "Sacred Arc of Sol");
    }
    if (ImGui::Button(" 🕊️ Dispatch Missionary to Foreign Land (150 Faith) ", ImVec2(-1, 0))) {
        r.dispatch_missionary(engine, 1);
    }
    if (ImGui::Button(" ⚔️ Declare Grand Holy Crusade (400 Faith) ", ImVec2(-1, 0))) {
        r.declare_holy_war(engine, 1);
    }
    if (ImGui::Button(" 🕍 Consecrate Grand Cathedral ($12,000 Gold) ", ImVec2(-1, 0))) {
        r.consecrate_shrine(engine);
    }
}

// ─── Chronicle Tab ────────────────────────────────────────────────────────────
void AeonGUI::draw_chronicle_tab(AeonEngine& engine) {
    ImGui::Text("LLM Imperial Chronicle Book");
    ImGui::Separator();
    ImGui::BeginChild("ChronicleScroll");
    ImGui::TextUnformatted(engine.chronicler.get_chronicle().c_str());
    ImGui::EndChild();
}

// ─── Save / Load Tab ──────────────────────────────────────────────────────────
void AeonGUI::draw_persistence_tab(AeonEngine& engine) {
    ImGui::Text("Multi-Universe Save & Load State");
    ImGui::Separator();

    ImGui::InputText("Save/Load Name", save_filename_buf_, IM_ARRAYSIZE(save_filename_buf_));

    if (ImGui::Button(" 💾 Save Universe ")) {
        engine.save_world(save_filename_buf_);
    }

    ImGui::SameLine();
    if (ImGui::Button(" 📂 Load Universe ")) {
        engine.load_world(save_filename_buf_);
    }
}

// ─── Presidential Office Tab ──────────────────────────────────────────────────
void AeonGUI::draw_president_tab(AeonEngine& engine) {
    auto& p = engine.president_game;
    if (p.player_civ_id < 0 || p.player_civ_id >= (int)engine.civs.size()) return;
    auto& civ = engine.civs[p.player_civ_id];

    // ── 1. Presidential Executive Header Bar ──
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.14f, 0.18f, 1.0f));
    ImGui::BeginChild("PresHeader", ImVec2(0, 110), true);

    ImGui::Text("🏛️ EXECUTIVE OVAL OFFICE  ·  Nation: %s  ·  %s", civ.name.c_str(), p.president_name.c_str());
    ImGui::SameLine(700);
    ImGui::Text("Year %d  ·  Term %d (Year %d/4)", engine.year, p.elections_won, p.term_counter);

    ImGui::Separator();

    // Stats Meters
    ImGui::Columns(6, "PresStats", false);

    // Approval Rating
    ImGui::Text("Approval Rating");
    ImVec4 app_col = p.approval_rating >= 60.0f ? ImVec4(0.2f, 0.9f, 0.3f, 1.0f) : (p.approval_rating >= 40.0f ? ImVec4(0.9f, 0.8f, 0.2f, 1.0f) : ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, app_col);
    char app_str[32]; snprintf(app_str, sizeof(app_str), "%.1f%%", p.approval_rating);
    ImGui::ProgressBar(p.approval_rating / 100.0f, ImVec2(-1, 0), app_str);
    ImGui::PopStyleColor();

    ImGui::NextColumn();

    // Coup Risk
    ImGui::Text("Coup Risk");
    ImVec4 coup_col = p.coup_risk > 50.0f ? ImVec4(0.9f, 0.2f, 0.2f, 1.0f) : ImVec4(0.3f, 0.7f, 0.9f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, coup_col);
    char coup_str[32]; snprintf(coup_str, sizeof(coup_str), "%.1f%%", p.coup_risk);
    ImGui::ProgressBar(p.coup_risk / 100.0f, ImVec2(-1, 0), coup_str);
    ImGui::PopStyleColor();

    ImGui::NextColumn();

    // Treasury Gold
    ImGui::Text("Treasury Gold");
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "💰 $%.0f Gold", p.treasury_gold);

    ImGui::NextColumn();

    // National GDP
    ImGui::Text("National GDP");
    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "📈 $%.0f GDP", civ.economy.gdp);

    ImGui::NextColumn();

    // Army Power
    ImGui::Text("Military Power");
    ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.3f, 1.0f), "⚔️ %.0f (%.0f Troops)", civ.military_power, civ.army_size);

    ImGui::NextColumn();

    // Stability
    ImGui::Text("Nation Stability");
    ImGui::ProgressBar(civ.stability / 100.0f, ImVec2(-1, 0), "Stability");

    ImGui::Columns(1);
    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "🗞️ NEWS TICKER: %s", p.last_news_headline.c_str());

    ImGui::EndChild();
    ImGui::PopStyleColor();

    // ── 2. Three Column Presidential Command Room ──
    float col_width = (ImGui::GetContentRegionAvail().x - 20) / 3.0f;

    // --- LEFT COLUMN: Ollama AI Cabinet & Press Briefings ---
    ImGui::BeginChild("PresCabinet", ImVec2(col_width, 0), true);
    ImGui::Text("🗣️ CABINET BRIEFINGS & CRISES");
    ImGui::Separator();

    if (p.is_overthrown) {
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "🚨 MILITARY COUP DETECTED!");
        ImGui::Text("Your administration was overthrown due to low stability.");
        if (ImGui::Button(" 🔄 Re-establish Administration ")) {
            p.init();
        }
    } else if (p.election_loss) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "🗳️ ELECTION LOST!");
        ImGui::Text("The voters chose the opposition party.");
        if (ImGui::Button(" 🗳️ Run Next Campaign ")) {
            p.election_loss = false;
            p.approval_rating = 55.0f;
            p.term_counter = 0;
        }
    } else {
        if (p.current_crisis.active) {
            ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.2f, 1.0f), "CRISIS: %s", p.current_crisis.title.c_str());
            ImGui::TextWrapped("%s", p.current_crisis.description.c_str());
            ImGui::Separator();

            ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "ADVISOR OPINIONS:");
            ImGui::TextWrapped("• %s", p.current_crisis.advisor_defense.c_str());
            ImGui::TextWrapped("• %s", p.current_crisis.advisor_economy.c_str());
            ImGui::TextWrapped("• %s", p.current_crisis.advisor_opposition.c_str());
            ImGui::Separator();

            ImGui::Text("EXECUTIVE CHOICES:");
            for (size_t opt_idx = 0; opt_idx < p.current_crisis.options.size(); ++opt_idx) {
                std::string btn_label = "[" + std::to_string(opt_idx + 1) + "] " + p.current_crisis.options[opt_idx];
                if (ImGui::Button(btn_label.c_str(), ImVec2(-1, 0))) {
                    p.resolve_crisis_option(engine, (int)opt_idx);
                }
            }
        } else {
            ImGui::Text("No emergency active. Your cabinet is monitoring national affairs.");
            if (ImGui::Button(" 🗣️ Convene Emergency Cabinet Meeting ")) {
                p.trigger_ollama_crisis(engine);
            }
        }

        ImGui::Separator();
        ImGui::Text("📜 DRAFT CUSTOM EXECUTIVE LAW");
        ImGui::InputText("##CustomLaw", custom_law_buf_, IM_ARRAYSIZE(custom_law_buf_));
        if (ImGui::Button(" Enact Custom Presidential Decree ", ImVec2(-1, 0))) {
            if (strlen(custom_law_buf_) > 0) {
                p.enact_decree(engine, DecreeType::CUSTOM_DECREE, custom_law_buf_);
                custom_law_buf_[0] = '\0';
            }
        }
    }

    ImGui::EndChild();

    ImGui::SameLine();

    // --- MIDDLE COLUMN: Policy Sliders & Decrees ---
    ImGui::BeginChild("PresPolicies", ImVec2(col_width, 0), true);
    ImGui::Text("⚖️ FISCAL POLICY & EXECUTIVE DECREES");
    ImGui::Separator();

    ImGui::Text("Tax Policy (%% Rate):");
    ImGui::SliderFloat("Income Tax", &p.income_tax, 0.0f, 50.0f, "%.1f%%");
    ImGui::SliderFloat("Corporate Tax", &p.corporate_tax, 0.0f, 50.0f, "%.1f%%");
    ImGui::SliderFloat("Import Tariffs", &p.import_tariff, 0.0f, 30.0f, "%.1f%%");

    ImGui::Separator();
    ImGui::Text("Federal Department Budgets:");
    ImGui::SliderFloat("Defense", &p.budget_defense, 0.0f, 50.0f, "%.0f%%");
    ImGui::SliderFloat("Healthcare", &p.budget_healthcare, 0.0f, 50.0f, "%.0f%%");
    ImGui::SliderFloat("Education", &p.budget_education, 0.0f, 50.0f, "%.0f%%");
    ImGui::SliderFloat("Infrastructure", &p.budget_infrastructure, 0.0f, 50.0f, "%.0f%%");
    ImGui::SliderFloat("Welfare", &p.budget_welfare, 0.0f, 50.0f, "%.0f%%");

    ImGui::Separator();
    ImGui::Text("PRE-BUILT EXECUTIVE DECREES:");
    if (ImGui::Button(" 💵 Economic Stimulus ($25,000 Gold) ", ImVec2(-1, 0))) {
        p.enact_decree(engine, DecreeType::ECONOMIC_STIMULUS);
    }
    ImGui::SetItemTooltip("Inject $25,000 Gold into commercial infrastructure & business grants.");

    if (ImGui::Button(" 🛡️ Emergency Conscription ($15,000 / +5,000 Army) ", ImVec2(-1, 0))) {
        p.enact_decree(engine, DecreeType::EMERGENCY_DRAFT);
    }
    ImGui::SetItemTooltip("Draft and equip 5,000 troops into active military service ($15,000 cost).");

    if (ImGui::Button(" 🔬 Science R&D Grant ($20,000 Gold) ", ImVec2(-1, 0))) {
        p.enact_decree(engine, DecreeType::RESEARCH_SUBSIDY);
    }
    ImGui::SetItemTooltip("Grant $20,000 Gold to national research laboratories.");

    if (ImGui::Button(" 🕊️ Global Peace Envoy ", ImVec2(-1, 0))) {
        p.enact_decree(engine, DecreeType::DIPLOMATIC_ENVOY);
    }

    if (ImGui::Button(" 🌾 Food Relief Distribution ($18,000 Gold) ", ImVec2(-1, 0))) {
        p.enact_decree(engine, DecreeType::FOOD_RELIEF);
    }
    ImGui::SetItemTooltip("Purchase and distribute $18,000 Gold worth of emergency food supplies.");

    if (ImGui::Button(" 🛂 Border Security Lockout ", ImVec2(-1, 0))) {
        p.enact_decree(engine, DecreeType::BORDER_LOCKOUT);
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "⚔️ DECLARE WAR ON FOREIGN NATION:");
    for (size_t i = 0; i < engine.civs.size(); ++i) {
        if ((int)i == p.player_civ_id) continue;
        const auto& enemy = engine.civs[i];
        if (enemy.is_alive <= 0.0f) continue;

        char war_btn_label[64];
        if (civ.relations[enemy.id] == DiplomacyStatus::AT_WAR) {
            snprintf(war_btn_label, sizeof(war_btn_label), "🔥 AT WAR WITH %s 🔥", enemy.name.c_str());
            ImGui::BeginDisabled();
            ImGui::Button(war_btn_label, ImVec2(-1, 0));
            ImGui::EndDisabled();
        } else {
            snprintf(war_btn_label, sizeof(war_btn_label), "⚔️ Declare War on %s", enemy.name.c_str());
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.15f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.25f, 0.25f, 1.0f));
            if (ImGui::Button(war_btn_label, ImVec2(-1, 0))) {
                p.declare_war(engine, (int)i);
            }
            ImGui::PopStyleColor(2);
        }
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "⚡ QUICK ACCESS & CHEATS (GOD MODE):");
    if (ImGui::Button(" 💰 +$1,000 Gold Treasury ", ImVec2(-1, 0))) {
        p.treasury_gold += 1000.0f;
    }
    ImGui::SetItemTooltip("Inject $1,000 Gold into national treasury.");

    if (ImGui::Button(" 🪖 +5,000 Military Army ", ImVec2(-1, 0))) {
        civ.army_size += 5000.0f;
        civ.military_power = civ.army_size * 0.12f;
    }
    ImGui::SetItemTooltip("Recruit 5,000 military troops instantly.");

    if (ImGui::Button(" 📈 +$5,000 GDP Economic Boost ", ImVec2(-1, 0))) {
        civ.economy.gdp += 5000.0f;
    }
    ImGui::SetItemTooltip("Boost national GDP by $5,000.");

    if (ImGui::Button(" ❤️ Max 100%% Approval & Stability ", ImVec2(-1, 0))) {
        p.approval_rating = 100.0f;
        p.coup_risk = 0.0f;
        civ.stability = 100.0f;
    }
    ImGui::SetItemTooltip("Set Approval and Stability to 100%% and clear Coup Risk.");

    if (ImGui::Button(" 🕊️ Force Universal Peace (All Nations) ", ImVec2(-1, 0))) {
        for (auto& c : engine.civs) {
            c.at_war = false;
            for (auto& rel : c.relations) {
                rel.second = DiplomacyStatus::NEUTRAL;
            }
        }
        p.last_news_headline = "DIPLOMACY: Universal peace declared across all nations.";
    }
    ImGui::SetItemTooltip("End all active wars globally and restore neutral diplomacy.");

    ImGui::EndChild();

    ImGui::SameLine();

    // --- RIGHT COLUMN: 2D Tactical Territory Map & History ---
    ImGui::BeginChild("PresMapHistory", ImVec2(0, 0), true);
    ImGui::Text("🗺️ 2D NATIONAL MAP & LOG");
    ImGui::Separator();

    // 2D Tactical Map Frame
    ImGui::BeginChild("MiniMap", ImVec2(0, 180), true);
    ImGui::Text("Territory Map: %s", civ.name.c_str());
    std::string mini_map_str = engine.world_map.render(civ.capital_x - 10, civ.capital_y - 8, 22, 12);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
    ImGui::TextUnformatted(mini_map_str.c_str());
    ImGui::PopStyleColor();
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::Text("📜 EXECUTIVE DECREE LOG");
    ImGui::BeginChild("DecreeLog");
    for (int idx = (int)p.decree_history.size() - 1; idx >= 0; --idx) {
        const auto& rec = p.decree_history[idx];
        ImGui::Text("[Yr %d] %s", rec.year, rec.title.c_str());
        ImGui::TextWrapped("  %s", rec.summary.c_str());
        if (rec.approval_delta >= 0.0f) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "  Approval: +%.1f%%", rec.approval_delta);
        } else {
            ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "  Approval: %.1f%%", rec.approval_delta);
        }
        ImGui::Separator();
    }
    ImGui::EndChild();

    ImGui::EndChild();
}

void AeonGUI::draw_space_espionage_tab(AeonEngine& engine) {
    auto& se = engine.space_espionage;
    auto& p  = engine.president_game;
    float col_w = (ImGui::GetContentRegionAvail().x - 20.0f) / 3.0f;

    // --- COLUMN 1: REALISTIC MEGAPROJECTS & SPACE RACE ---
    ImGui::BeginChild("SpaceCol", ImVec2(col_w, 0), true);
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "🚀 HISTORIC MEGAPROJECTS & SPACE RACE");
    ImGui::Separator();

    for (size_t idx = 0; idx < se.megaprojects.size(); ++idx) {
        auto& mp = se.megaprojects[idx];
        if (ImGui::TreeNode(mp.name.c_str())) {
            if (mp.completed) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "STATUS: COMPLETED 🏆");
            } else {
                ImGui::Text("Phase %d of %d | Risk: %.0f%%", mp.current_phase + 1, mp.max_phases, mp.failure_risk * 100.0f);
                int phase = mp.current_phase;
                float current_p = mp.phase_progress[phase];
                float total_p = mp.phase_cost[phase];
                ImGui::Text("Phase Cost: $%.0f Gold (Funded: $%.0f)", total_p, current_p);
                ImGui::ProgressBar(current_p / total_p, ImVec2(-1, 0));

                char fund_1k[64], fund_10k[64], launch_btn[64];
                snprintf(fund_1k, sizeof(fund_1k), "Invest $5,000 Gold##%d", (int)idx);
                snprintf(fund_10k, sizeof(fund_10k), "Invest $20,000 Gold##%d", (int)idx);
                snprintf(launch_btn, sizeof(launch_btn), "🚀 Launch Phase %d Test##%d", phase + 1, (int)idx);

                if (ImGui::Button(fund_1k, ImVec2(-1, 0))) {
                    se.invest_in_megaproject(engine, (int)idx, 5000.0f);
                }
                if (ImGui::Button(fund_10k, ImVec2(-1, 0))) {
                    se.invest_in_megaproject(engine, (int)idx, 20000.0f);
                }

                if (current_p >= total_p) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.7f, 0.2f, 1.0f));
                    if (ImGui::Button(launch_btn, ImVec2(-1, 0))) {
                        se.attempt_phase_launch(engine, (int)idx);
                    }
                    ImGui::PopStyleColor();
                } else {
                    ImGui::TextDisabled("Fully fund current phase to attempt test launch.");
                }
            }
            ImGui::TreePop();
        }
        ImGui::Separator();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // --- COLUMN 2: NUCLEAR DETERRENT ---
    ImGui::BeginChild("NukeCol", ImVec2(col_w, 0), true);
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "⚛️ ICBM NUCLEAR ARSENAL");
    ImGui::Separator();

    ImGui::Text("Silos Built: %d", se.nuke.silos_built);
    ImGui::Text("ICBM Stockpile: %d Warheads", se.nuke.icbm_stockpile);

    ImGui::Separator();
    if (ImGui::Button(" ⚛️ Build ICBM Silo ($120,000 Gold) ", ImVec2(-1, 0))) {
        se.build_nuke_silo(engine);
    }
    ImGui::SetItemTooltip("Construct strategic underground nuclear missile silo ($120,000 cost).");

    if (ImGui::Button(" 🚀 Assemble ICBM Warhead ($35,000 Gold) ", ImVec2(-1, 0))) {
        se.construct_icbm(engine);
    }
    ImGui::SetItemTooltip("Assemble thermonuclear ICBM missile ($35,000 cost).");

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "💥 LAUNCH ICBM STRIKE:");
    for (size_t i = 0; i < engine.civs.size(); ++i) {
        if ((int)i == p.player_civ_id) continue;
        const auto& target = engine.civs[i];
        if (target.is_alive <= 0.0f) continue;

        char nuke_label[64];
        snprintf(nuke_label, sizeof(nuke_label), "💥 Launch ICBM on %s", target.name.c_str());
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Button(nuke_label, ImVec2(-1, 0))) {
            se.launch_icbm_strike(engine, (int)i);
        }
        ImGui::PopStyleColor();
        ImGui::SetItemTooltip("Devastate enemy nation with thermonuclear strike (wipes 50%% population & army).");
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // --- COLUMN 3: SECRET INTELLIGENCE AGENCY ---
    ImGui::BeginChild("EspionageCol", ImVec2(0, 0), true);
    ImGui::TextColored(ImVec4(0.8f, 0.4f, 1.0f, 1.0f), "🕵️ SECRET INTELLIGENCE AGENCY");
    ImGui::Separator();

    ImGui::Text("Active Spies: %d Agents", se.espionage.active_spies);
    ImGui::Separator();

    ImGui::Text("COVERT OPERATIONS:");
    for (size_t i = 0; i < engine.civs.size(); ++i) {
        if ((int)i == p.player_civ_id) continue;
        const auto& target = engine.civs[i];
        if (target.is_alive <= 0.0f) continue;

        ImGui::Text("Target: %s", target.name.c_str());

        char coup_btn[64], ass_btn[64], dis_btn[64];
        snprintf(coup_btn, sizeof(coup_btn), "⚡ Stage Coup in %s ($200,000)", target.name.c_str());
        snprintf(ass_btn, sizeof(ass_btn), "🗡️ Assassinate %s Leader ($60,000)", target.name.c_str());
        snprintf(dis_btn, sizeof(dis_btn), "📜 Disinformation in %s ($15,000)", target.name.c_str());

        if (ImGui::Button(coup_btn, ImVec2(-1, 0))) {
            se.stage_foreign_coup(engine, (int)i);
        }
        ImGui::SetItemTooltip("Overthrow foreign ruler and install an allied puppet regime!");

        if (ImGui::Button(ass_btn, ImVec2(-1, 0))) {
            se.assassinate_ruler(engine, (int)i);
        }
        ImGui::SetItemTooltip("Eliminate foreign ruler to cause succession collapse.");

        if (ImGui::Button(dis_btn, ImVec2(-1, 0))) {
            se.inject_disinformation(engine, (int)i);
        }
        ImGui::SetItemTooltip("Degrade social cohesion and stability in target nation.");
        ImGui::Separator();
    }
    ImGui::EndChild();
}

void AeonGUI::draw_ruler_chat_tab(AeonEngine& engine) {
    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "💬 INTERACTIVE LLM RULER DIPLOMACY & CHAT");
    ImGui::Separator();

    static int target_civ_idx = 0;
    if (target_civ_idx >= (int)engine.civs.size()) target_civ_idx = 0;

    ImGui::Text("Select Empire Ruler:");
    for (size_t i = 0; i < engine.civs.size(); ++i) {
        if (engine.civs[i].is_commons) continue;
        char name_label[128];
        snprintf(name_label, sizeof(name_label), "%s (%s)", engine.civs[i].name.c_str(), engine.civs[i].is_alive > 0.0f ? "Active" : "Fallen");
        if (ImGui::RadioButton(name_label, target_civ_idx == (int)i)) {
            target_civ_idx = (int)i;
        }
        if (i < engine.civs.size() - 1) ImGui::SameLine();
    }
    ImGui::Separator();

    const auto& civ = engine.civs[target_civ_idx];
    ImGui::Text("Ruler Dossier: %s | Aggression: %.2f | Science: %.2f | Diplo: %.2f",
        civ.name.c_str(), civ.aggression, civ.science_pref, civ.diplomacy_pref);

    ImGui::BeginChild("ChatLog", ImVec2(0, 180), true);
    if (ruler_chat_history_.empty()) {
        ImGui::TextDisabled("No diplomatic conversation yet. Enter a proposal or declaration below.");
    } else {
        ImGui::TextUnformatted(ruler_chat_history_.c_str());
    }
    ImGui::EndChild();

    ImGui::InputText("Message", ruler_chat_buf_, sizeof(ruler_chat_buf_));
    ImGui::SameLine();
    if (ImGui::Button("Send to Ruler") && strlen(ruler_chat_buf_) > 0) {
        ruler_chat_history_ += "\nDiplomat: " + std::string(ruler_chat_buf_) + "\n";
        std::string reply = AeonOllama::chat_with_ruler("Ruler of " + civ.name, civ.name, civ.primary_goal, ruler_chat_buf_);
        if (reply.empty()) reply = "[" + civ.name + " Ruler responds via envoy]: We acknowledge your message, Ambassador.";
        ruler_chat_history_ += civ.name + " Ruler: " + reply + "\n";
        ruler_chat_buf_[0] = '\0';
        engine.history.record(engine.year, engine.month, "DIPLOMACY", "DIPLOMATIC CHAT", "Negotiated with " + civ.name);
    }
}


void AeonGUI::draw_coalitions_tab(AeonEngine& engine) {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "🛡️ INTERNATIONAL COALITIONS & ESPIONAGE NETWORK");
    ImGui::Separator();

    ImGui::Text("ACTIVE DEFENSIVE COALITION ALLIANCES:");
    for (const auto& c : engine.coalition_engine.coalitions) {
        ImGui::Text("  🛡️ %s (Founder Civ: %d)", c.name.c_str(), c.founder_civ_id);
    }
    ImGui::Separator();



    // Deep State & Secret Societies Engine
    if (ImGui::CollapsingHeader("👁️ Deep State & Secret Societies Engine", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& deep = engine.deep_state_engine;
        for (const auto& cell : deep.cells) {
            ImGui::TextColored(ImVec4(0.8f, 0.3f, 1.0f, 1.0f), "👁️ Empire: %s — %s", cell.civ_name.c_str(), shadow_society_name(cell.society));
            ImGui::Text("  Shadow Treasury: $%.0f Gold | Infiltration Level: %.1f%%", cell.shadow_funds_gold, cell.infiltration_pct);
            ImGui::ProgressBar(cell.infiltration_pct / 100.0f, ImVec2(-1, 0));

            if (ImGui::Button(("🗡️ Orchestrate Covert Coup##" + std::to_string(cell.civ_id)).c_str())) {
                deep.launch_shadow_coup(cell.civ_id, cell.society, engine);
            }
            ImGui::SameLine();
            if (ImGui::Button(("💼 Siphon State Treasury ($500 Gold)##" + std::to_string(cell.civ_id)).c_str())) {
                deep.siphoning_treasury(cell.civ_id, 500.0f, engine);
            }
            ImGui::Separator();
        }
    }

    // Multi-Agent LLM Diplomatic Round-Table Summits
    if (ImGui::CollapsingHeader("🤖 Multi-Agent LLM Diplomatic Summits", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& summit = engine.diplomatic_summit_engine;
        static char summit_topic_buf[256] = "Global Ceasefire & Disarmament Treaty of 2026";
        ImGui::InputText("Summit Agenda Topic", summit_topic_buf, sizeof(summit_topic_buf));
        if (ImGui::Button("🌐 Convene World Peace Summit")) {
            summit.convene_summit(engine, summit_topic_buf);
        }

        if (summit.current_summit.active) {
            ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "CURRENT SUMMIT IN SESSION: %s", summit.current_summit.topic.c_str());
            if (ImGui::Button("▶ Hear Next World Leader Speech")) {
                summit.process_summit_step(engine);
            }

            ImGui::Separator();
            for (const auto& sp : summit.current_summit.speeches) {
                ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "🗣️ %s (%s):", sp.speaker_title.c_str(), sp.civ_name.c_str());
                ImGui::TextWrapped("   \"%s\"", sp.speech.c_str());
                const char* v_str = (sp.vote == SummitVote::SUPPORT) ? "SUPPORT ✅" : ((sp.vote == SummitVote::OPPOSE) ? "OPPOSE ❌" : "ABSTAIN ⚪");
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "   Vote: %s", v_str);
            }
        }
    }
}



void AeonGUI::draw_tech_tree_tab(AeonEngine& engine) {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "🔬 MULTI-ERA TECHNOLOGY TREE & POLICY BRANCHES");
    ImGui::Separator();

    for (auto& civ : engine.civs) {
        if (civ.is_commons || civ.is_alive <= 0.0f) continue;
        if (ImGui::TreeNode(civ.name.c_str())) {
            ImGui::Text("Current Era: %s | Tech Progress: %.1f%%", tech_era_name(civ.tech.era), civ.tech.progress);
            ImGui::ProgressBar(civ.tech.progress / 100.0f, ImVec2(-1, 0));

            for (const auto& node : engine.tech_tree_engine.nodes) {
                ImGui::BulletText("[%s] %s (Cost: %.0f R&D) - %s",
                    tech_era_name(node.era), node.name.c_str(), node.research_cost, node.description.c_str());
            }
            ImGui::TreePop();
        }
    }
}



void AeonGUI::draw_trade_routes_tab(AeonEngine& engine) {
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "🚚 PHYSICAL TRADE CARAVANS & STRATEGIC MAP RESOURCES");
    ImGui::Separator();

    ImGui::Text("STRATEGIC MAP RESOURCE NODES:");
    for (const auto& node : engine.caravan_engine.nodes) {
        ImGui::Text("Node #%d: %s at Pos (%.1f, %.1f) | Reserve: %.0f",
            node.id, strategic_resource_name(node.type), node.pos.x, node.pos.y, node.remaining_reserve);
    }
    ImGui::Separator();

    ImGui::Text("ACTIVE MAP CARAVANS:");
    if (engine.caravan_engine.caravans.empty()) {
        ImGui::Text("No active merchant caravans on the road.");
    } else {
        for (const auto& c : engine.caravan_engine.caravans) {
            ImGui::Text("Caravan #%d [%s] Val: $%.0f | Pos: (%.1f, %.1f)", c.id, c.cargo_type.c_str(), c.cargo_value, c.pos.x, c.pos.y);
            ImGui::SameLine();
            char btn_id[64];
            snprintf(btn_id, sizeof(btn_id), "Raid #%d", c.id);
            if (ImGui::Button(btn_id)) {
                engine.caravan_engine.raid_caravan(c.id, 0, engine);
            }
        }
    }

    if (ImGui::Button("Dispatch New Merchant Caravan")) {
        engine.caravan_engine.spawn_caravan(0, 1, glm::vec2(20.0f, 10.0f), glm::vec2(90.0f, 10.0f), "Gold & Silks", 250.0f);
    }
}

void AeonGUI::draw_analytics_tab(AeonEngine& engine) {
    auto& analytics = engine.analytics_engine;
    ImGui::TextColored(ImVec4(0.2f, 0.9f, 1.0f, 1.0f), "📈 LIVE HISTORICAL LINE CHARTS & FINANCIAL TELEMETRY");
    ImGui::Text("Real-time 200-year multi-empire comparative metrics and telemetry graphs.");
    ImGui::Separator();

    if (analytics.global_years.empty()) {
        ImGui::Text("No historical data recorded yet. Run the simulation to view telemetry trends.");
        return;
    }

    static int active_metric = 0;
    ImGui::RadioButton("Gross Domestic Product (GDP)", &active_metric, 0); ImGui::SameLine();
    ImGui::RadioButton("Population Growth (Thousands)", &active_metric, 1); ImGui::SameLine();
    ImGui::RadioButton("Military Division Power", &active_metric, 2); ImGui::SameLine();
    ImGui::RadioButton("State Treasury Gold", &active_metric, 3);
    ImGui::Separator();

    // Custom ImGui Line Chart Canvas
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    float chart_w = ImGui::GetContentRegionAvail().x;
    float chart_h = 300.0f;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Chart Background
    dl->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + chart_w, canvas_pos.y + chart_h), IM_COL32(15, 20, 30, 255), 6.0f);
    dl->AddRect(canvas_pos, ImVec2(canvas_pos.x + chart_w, canvas_pos.y + chart_h), IM_COL32(50, 70, 100, 255), 6.0f);

    static const ImU32 civ_colors[] = {
        IM_COL32(255, 80, 80, 255),  // Nordra (Red)
        IM_COL32( 80, 140, 255, 255),// Eldoria (Blue)
        IM_COL32(255, 210, 30, 255), // Valoria (Gold)
        IM_COL32( 80, 220, 80, 255), // Drakor (Green)
        IM_COL32(210, 80, 240, 255), // Solaria (Purple)
        IM_COL32(255, 150, 30, 255)  // Commons (Orange)
    };

    // Find max metric value for Y-axis scaling
    float max_val = 1.0f;
    for (const auto& series : analytics.series_data) {
        const std::vector<float>* vec = &series.gdp;
        if (active_metric == 1) vec = &series.population;
        else if (active_metric == 2) vec = &series.military_power;
        else if (active_metric == 3) vec = &series.treasury;

        for (float v : *vec) {
            if (v > max_val) max_val = v;
        }
    }

    // Draw grid lines
    for (int i = 1; i <= 4; ++i) {
        float gy = canvas_pos.y + chart_h - (i / 4.0f) * (chart_h - 40.0f) - 20.0f;
        dl->AddLine(ImVec2(canvas_pos.x + 10, gy), ImVec2(canvas_pos.x + chart_w - 10, gy), IM_COL32(40, 50, 70, 150));
        char y_lbl[32];
        snprintf(y_lbl, sizeof(y_lbl), "%.0f", max_val * (i / 4.0f));
        dl->AddText(ImVec2(canvas_pos.x + 15, gy - 12), IM_COL32(140, 160, 180, 200), y_lbl);
    }

    // Draw line curves for each empire
    for (size_t s_idx = 0; s_idx < analytics.series_data.size(); ++s_idx) {
        const auto& series = analytics.series_data[s_idx];
        if (series.years.size() < 2) continue;

        const std::vector<float>* vec = &series.gdp;
        if (active_metric == 1) vec = &series.population;
        else if (active_metric == 2) vec = &series.military_power;
        else if (active_metric == 3) vec = &series.treasury;

        ImU32 col = civ_colors[series.civ_id % 6];
        size_t n = vec->size();

        for (size_t i = 1; i < n; ++i) {
            float x1 = canvas_pos.x + 20.0f + ((i - 1) / float(n - 1)) * (chart_w - 40.0f);
            float y1 = canvas_pos.y + chart_h - 20.0f - ((*vec)[i - 1] / max_val) * (chart_h - 40.0f);
            float x2 = canvas_pos.x + 20.0f + (i / float(n - 1)) * (chart_w - 40.0f);
            float y2 = canvas_pos.y + chart_h - 20.0f - ((*vec)[i] / max_val) * (chart_h - 40.0f);

            dl->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), col, 2.5f);
        }
    }

    ImGui::Dummy(ImVec2(chart_w, chart_h + 10.0f));

    // Legend
    ImGui::Text("EMPIRE LEGEND:");
    for (size_t i = 0; i < analytics.series_data.size(); ++i) {
        const auto& s = analytics.series_data[i];
        ImVec4 col = ImColor(civ_colors[s.civ_id % 6]);
        ImGui::TextColored(col, "  ██ %s", s.civ_name.c_str());
        ImGui::SameLine();
    }
    ImGui::NewLine();
}


// ─── 🌋 Disasters & Weather Tab ────────────────────────────────────────────────
void AeonGUI::draw_disasters_tab(AeonEngine& engine) {
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "🌋 GLOBAL NATURAL DISASTERS & WEATHER CONTROL");
    ImGui::Separator();

    ImGui::Text("ACTIVE PLANETARY DISASTERS:");
    if (engine.disaster_engine.active_disasters.empty()) {
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), "No active catastrophic disasters reported.");
    } else {
        for (const auto& d : engine.disaster_engine.active_disasters) {
            if (!d.is_active) continue;
            ImGui::Text("⚠️ [%s] Location: (%d, %d) | Severity: %.1f | Years Remaining: %d",
                d.name.c_str(), d.target_x, d.target_y, d.severity, d.duration_years);
        }
    }

    ImGui::Separator();
    ImGui::Text("FEDERAL CRISIS INTERVENTION DECREES:");
    if (ImGui::Button(" ☣️ Enact National Bio-Quarantine ($10,000 Gold) ", ImVec2(-1, 0))) {
        engine.disaster_engine.enact_quarantine(engine);
    }
    if (ImGui::Button(" 💉 Deploy Vaccine Research ($25,000 Gold) ", ImVec2(-1, 0))) {
        engine.disaster_engine.research_plague_vaccine(engine);
    }
    if (ImGui::Button(" 🚚 Dispatch Disaster Relief Convoys ($15,000 Gold) ", ImVec2(-1, 0))) {
        engine.disaster_engine.deploy_disaster_relief(engine);
    }

    ImGui::Separator();
    ImGui::Text("DISASTER LOG:");
    for (int i = (int)engine.disaster_engine.disaster_history_log.size() - 1; i >= 0; --i) {
        ImGui::TextWrapped("  • %s", engine.disaster_engine.disaster_history_log[i].c_str());
    }
}

// ─── 🏛️ World Wonders Tab ──────────────────────────────────────────────────────
void AeonGUI::draw_wonders_tab(AeonEngine& engine) {
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "🏛️ WORLD WONDERS CONSTRUCTION ENGINE");
    ImGui::Separator();

    for (const auto& w : engine.wonder_engine.wonders) {
        ImGui::Text("%s - %s", w.name.c_str(), w.description.c_str());
        ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "  Perk: %s", w.passive_perk_text.c_str());

        if (w.is_built) {
            ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "  Status: BUILT in Year %d by Civ #%d", w.built_year, w.builder_civ_id);
        } else {
            float progress = w.invested_gold / w.cost_gold;
            ImGui::ProgressBar(progress, ImVec2(-1, 0));
            char btn1[64], btn2[64];
            snprintf(btn1, sizeof(btn1), "Invest $10,000 Gold into %s", w.name.c_str());
            snprintf(btn2, sizeof(btn2), "Invest $50,000 Gold into %s", w.name.c_str());
            if (ImGui::Button(btn1)) {
                engine.wonder_engine.invest_in_wonder(engine, w.id, 10000.0f);
            }
            ImGui::SameLine();
            if (ImGui::Button(btn2)) {
                engine.wonder_engine.invest_in_wonder(engine, w.id, 50000.0f);
            }
        }
        ImGui::Separator();
    }
}

// ─── 🏦 Stock Market Tab ───────────────────────────────────────────────────────
void AeonGUI::draw_stock_market_tab(AeonEngine& engine) {
    auto& m = engine.economy_market_engine;
    auto& p = engine.president_game;
    (void)p;

    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "🏦 GLOBAL STOCK EXCHANGE & CENTRAL BANK");
    ImGui::Separator();

    ImGui::Text("Market Index: %.0f Pts | Central Bank Interest Rate: %.1f%% | Inflation Rate: %.1f%%",
        m.market_index_points, m.central_bank_interest_rate, m.inflation_rate);
    ImGui::Separator();

    ImGui::Text("LISTED CORPORATIONS:");
    for (const auto& s : m.stocks) {
        float delta = s.share_price - s.prev_price;
        ImGui::Text("[%s] %s - Share Price: $%.2f (Delta: %+.2f) | Owned: %d shares",
            s.ticker.c_str(), s.name.c_str(), s.share_price, delta, s.player_shares_owned);

        char buy10[64], sell10[64];
        snprintf(buy10, sizeof(buy10), "Buy 10 Shares of %s", s.ticker.c_str());
        snprintf(sell10, sizeof(sell10), "Sell 10 Shares of %s", s.ticker.c_str());

        if (ImGui::Button(buy10)) {
            m.buy_stock(engine, s.id, 10);
        }
        ImGui::SameLine();
        if (ImGui::Button(sell10)) {
            m.sell_stock(engine, s.id, 10);
        }
    }
}

// ─── ⚓ Naval Fleets Tab ────────────────────────────────────────────────────────
void AeonGUI::draw_naval_tab(AeonEngine& engine) {
    auto& n = engine.naval_engine;
    ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "⚓ NAVAL FLEETS, SUBMARINES & DEEP-SEA RIGS");
    ImGui::Separator();

    ImGui::Text("Offshore Oil Rigs Operational: %d (Generates +$%.0f Gold/yr)",
        n.offshore_oil_rigs, n.offshore_oil_rigs * 8000.0f);

    if (ImGui::Button(" 🛢️ Construct Deep-Sea Oil Rig ($30,000 Gold) ", ImVec2(-1, 0))) {
        n.construct_offshore_rig(engine);
    }

    ImGui::Separator();
    ImGui::Text("ACTIVE WARSHIP FLEETS:");
    for (const auto& f : n.fleets) {
        ImGui::Text("Fleet #%d - Destroyers: %d | Submarines: %d | Aircraft Carriers: %d | Blockading: %s",
            f.id, f.destroyers, f.submarines, f.aircraft_carriers, f.is_blockading ? "YES" : "NO");
    }

    ImGui::Separator();
    ImGui::Text("WARSHIP SHIPYARD:");
    if (ImGui::Button(" 🛳️ Commission Destroyer ($15,000 Gold) ")) {
        n.build_warship(engine, "DESTROYER");
    }
    ImGui::SameLine();
    if (ImGui::Button(" 🌊 Commission Attack Submarine ($25,000 Gold) ")) {
        n.build_warship(engine, "SUBMARINE");
    }
    ImGui::SameLine();
    if (ImGui::Button(" 🚢 Commission Supercarrier ($75,000 Gold) ")) {
        n.build_warship(engine, "CARRIER");
    }
}

// ─── 🏛️ Parliament & Congress Tab ─────────────────────────────────────────────
void AeonGUI::draw_parliament_tab(AeonEngine& engine) {
    auto& par = engine.parliament_engine;
    ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "🏛️ NATIONAL CONGRESS & PARLIAMENTARY PARTIES");
    ImGui::Separator();

    ImGui::Text("100-SEAT HOUSE OF REPRESENTATIVES:");
    ImGui::Text("🔷 Liberal Democrats: %d | 🟥 Conservative Union: %d | 🦅 National Security: %d",
        par.seats_liberal, par.seats_conservative, par.seats_security);
    ImGui::Text("⚛️ Technocrat Progress: %d | 🌿 Green Ecology: %d",
        par.seats_technocrat, par.seats_green);
    ImGui::Separator();

    ImGui::Text("SUBMIT LEGISLATIVE BILL TO CONGRESSIONAL VOTE:");
    if (ImGui::Button(" 💵 Submit Infrastructure Stimulus Bill ($25,000 Gold) ", ImVec2(-1, 0))) {
        par.submit_bill_to_vote(engine, "National Transit Infrastructure Act", "Injects $25k into transit highways", 25000.0f);
    }
    if (ImGui::Button(" 🛡️ Submit Emergency Defense Authorization Bill ($40,000 Gold) ", ImVec2(-1, 0))) {
        par.submit_bill_to_vote(engine, "National Defense Reauthorization Act", "Expands army divisions & missile silos", 40000.0f);
    }
    if (ImGui::Button(" 📣 Organize Party Campaign Rally ($8,000 Gold) ", ImVec2(-1, 0))) {
        par.execute_campaign_rally(engine, PoliticalParty::LIBERAL_DEMOCRATS);
    }

    ImGui::Separator();
    ImGui::Text("CONGRESSIONAL VOTING HISTORY:");
    for (int i = (int)par.bill_history.size() - 1; i >= 0; --i) {
        const auto& b = par.bill_history[i];
        ImGui::Text("[%s] Bill #%d: %s (%d Ayes vs %d Nays)",
            b.passed ? "PASSED" : "REJECTED", b.id, b.title.c_str(), b.votes_for, b.votes_against);
    }
}

// ─── 🇺🇳 United Nations Tab ────────────────────────────────────────────────────
void AeonGUI::draw_un_council_tab(AeonEngine& engine) {
    auto& un = engine.un_council_engine;
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "🇺🇳 UNITED NATIONS GENERAL ASSEMBLY & SECURITY COUNCIL");
    ImGui::Separator();

    ImGui::Text("Active UN Peacekeeper Divisions: %d", un.peacekeeper_divisions);
    ImGui::Separator();

    ImGui::Text("PASSED UN RESOLUTIONS:");
    for (const auto& r : un.active_resolutions) {
        ImGui::Text("Resolution #%d: %s [%s]", r.id, r.title.c_str(), r.passed ? "ACTIVE" : "FAILED");
    }

    ImGui::Separator();
    ImGui::Text("INTERNATIONAL DIPLOMACY ACTIONS:");
    if (ImGui::Button(" 📜 Propose Global Climate Treaty Resolution ($15,000 Gold) ", ImVec2(-1, 0))) {
        un.propose_resolution(engine, "Global Climate Accord", "Enforces international emission caps");
    }
    if (ImGui::Button(" 🕊️ Deploy UN Peacekeeper Division ($20,000 Gold) ", ImVec2(-1, 0))) {
        un.deploy_peacekeepers(engine, 1);
    }
    if (ImGui::Button(" 🚫 Impose Multilateral UN Sanctions ($12,000 Gold) ", ImVec2(-1, 0))) {
        un.enact_un_sanctions(engine, 1);
    }
}

// ─── 🌐 NATO & OPEC Alliances Tab ──────────────────────────────────────────────
void AeonGUI::draw_alliances_tab(AeonEngine& engine) {
    auto& al = engine.alliance_engine;
    ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f), "🌐 GLOBAL ALLIANCE BLOCS (NATO & OPEC)");
    ImGui::Separator();

    for (const auto& b : al.blocs) {
        ImGui::Text("Bloc: %s (Type: %s) | Article 5 Active: %s",
            b.name.c_str(), b.type.c_str(), b.article_5_active ? "YES" : "NO");
        ImGui::Text("  Member Civs: %zu countries signed", b.member_civ_ids.size());
        ImGui::Separator();
    }

    ImGui::Text("ALLIANCE ACTIONS:");
    if (ImGui::Button(" 🛡️ Sign NATO Article 5 Mutual Defense Pact ($25,000 Gold) ", ImVec2(-1, 0))) {
        al.sign_nato_treaty(engine, 1);
    }
    if (ImGui::Button(" 🛢️ Enforce OPEC Oil Production Quota ($30,000 Gold) ", ImVec2(-1, 0))) {
        al.form_opec_cartel(engine);
    }
}

// ─── 🌐 GIS Elevation & Climate Physics Tab ──────────────────────────────────
void AeonGUI::draw_gis_climate_tab(AeonEngine& engine) {
    auto& gis = engine.gis_climate_engine;
    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.9f, 1.0f), "🌐 GIS ELEVATION & ATMOSPHERIC CLIMATE PHYSICS");
    ImGui::Separator();

    ImGui::Text("Grid Size: %dx%d Tiles | Elevation Model: Topographic Lapse Rate (-6.5 deg C / 1000m)", gis.map_width, gis.map_height);
    ImGui::Separator();

    if (ImGui::BeginTable("GISTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Tile (X, Y)");
        ImGui::TableSetupColumn("Terrain Type");
        ImGui::TableSetupColumn("Elevation (m)");
        ImGui::TableSetupColumn("Temperature (deg C)");
        ImGui::TableSetupColumn("Rainfall (mm/yr)");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < std::min((size_t)16, gis.grid.size()); ++i) {
            const auto& cell = gis.grid[i];
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("(%d, %d)", cell.x, cell.y);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", cell.terrain_type.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::Text("%.1fm", cell.elevation_m);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.1f deg C", cell.temperature_c);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%.1f mm", cell.rainfall_mm);
        }
        ImGui::EndTable();
    }
}

// ─── 📈 Dynamic Supply & Demand Physics Tab ─────────────────────────────────
void AeonGUI::draw_supply_demand_tab(AeonEngine& engine) {
    auto& sd = engine.supply_demand_engine;
    ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "📈 DYNAMIC SUPPLY & DEMAND MARKET ELASTICITY");
    ImGui::Separator();

    if (ImGui::BeginTable("SDTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Commodity Name");
        ImGui::TableSetupColumn("Base Price");
        ImGui::TableSetupColumn("Current Price");
        ImGui::TableSetupColumn("Global Supply / Demand");
        ImGui::TableSetupColumn("24h Price Delta");
        ImGui::TableHeadersRow();

        for (const auto& item : sd.commodities) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%s", item.name.c_str());
            ImGui::TableSetColumnIndex(1); ImGui::Text("$%.2f", item.base_price);
            ImGui::TableSetColumnIndex(2); ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "$%.2f", item.current_price);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.0f / %.0f", item.global_supply, item.global_demand);
            ImGui::TableSetColumnIndex(4); 
            if (item.price_change_pct >= 0.0f) {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "+%.2f%%", item.price_change_pct);
            } else {
                ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "%.2f%%", item.price_change_pct);
            }
        }
        ImGui::EndTable();
    }
}

// ─── 👥 Demographics & Population Pyramids Tab ──────────────────────────────
void AeonGUI::draw_demographics_tab(AeonEngine& engine) {
    auto& demo = engine.demographics_engine;
    ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.2f, 1.0f), "👥 DEMOGRAPHIC POPULATION PYRAMIDS, REFUGEES & SEIR PANDEMIC");
    ImGui::Separator();

    for (const auto& p : demo.pyramids) {
        ImGui::Text("Empire: %s", p.civ_name.c_str());
        ImGui::Text("  Youth (0-18): %lld  |  Working (19-64): %lld  |  Seniors (65+): %lld",
            p.youth_count, p.working_count, p.senior_count);
        ImGui::Text("  Dependency Ratio: %.2f  |  Birth Rate: %.1f/k  |  Death Rate: %.1f/k",
            p.dependency_ratio, p.birth_rate_per_1000, p.death_rate_per_1000);
        ImGui::Separator();
    }

    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "🌊 ACTIVE REFUGEE DISPLACEMENT WAVES:");
    if (demo.refugees.empty()) {
        ImGui::Text("  No active border refugee movements.");
    } else {
        for (const auto& rw : demo.refugees) {
            ImGui::Text("  [%s] Civ %d -> Civ %d: %lld refugees displaced (%s)",
                rw.reason.c_str(), rw.origin_civ_id, rw.target_civ_id, rw.displacement_count, rw.reason.c_str());
        }
    }
    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "☣️ GLOBAL SEIR PANDEMIC ENGINE:");
    if (!demo.global_pandemic.active) {
        ImGui::Text("  Status: No active global pathogen.");
        if (ImGui::Button("Trigger Outbreak (Ebola-X Variant)")) {
            demo.trigger_outbreak("Ebola-X Variant", 12000);
        }
    } else {
        const auto& pan = demo.global_pandemic;
        ImGui::Text("  Pathogen: %s  |  R0: %.2f  |  Mortality: %.1f%%",
            pan.pathogen_name.c_str(), pan.R0, pan.mortality_rate * 100.0f);
        ImGui::Text("  Susceptible: %lld  |  Exposed: %lld  |  Infectious: %lld  |  Recovered: %lld",
            pan.susceptible, pan.exposed, pan.infectious, pan.recovered);
        ImGui::ProgressBar(pan.vaccine_progress, ImVec2(0.0f, 0.0f), "Vaccine Development");
    }

    ImGui::Separator();
    // Population Genetic Adaptation
    if (ImGui::CollapsingHeader("🧬 Population Genetics & Bio-Engineering", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& genetics = engine.genetics_engine;
        for (const auto& prof : genetics.profiles) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "🧬 Empire: %s", prof.civ_name.c_str());
            ImGui::Text("  Bio-Tech Level: %.2fx | Lifespan: %.1f Yrs | Cognition Mult: %.2fx | Soldier Power: %.2fx",
                prof.bio_tech_level, prof.lifespan_years, prof.cognitive_research_mult, prof.soldier_gene_power_mult);
            ImGui::Text("  Unlocked Traits:");
            if (prof.active_traits.empty()) {
                ImGui::Text("   (None unlocked yet)");
            } else {
                for (auto t : prof.active_traits) {
                    ImGui::BulletText("%s", genetic_trait_name(t));
                }
            }
            if (ImGui::Button(("🧬 Bio-Engineer Cognitive Gene##" + std::to_string(prof.civ_id)).c_str())) {
                genetics.unlock_trait(prof.civ_id, GeneticTrait::COGNITIVE_AMPLIFICATION, engine);
            }
            ImGui::SameLine();
            if (ImGui::Button(("🦾 Bio-Engineer Cybernetics##" + std::to_string(prof.civ_id)).c_str())) {
                genetics.unlock_trait(prof.civ_id, GeneticTrait::CYBERNETIC_SYNTHESIS, engine);
            }
            ImGui::Separator();
        }
    }


    ImGui::Separator();
    ImGui::Text("TRANSIT DELAY CALCULATOR:");
    float transit_hrs = demo.calculate_transit_delay_hours(450.0f, 1.5f);
    ImGui::Text("  Standard 450km Cargo Transit across Hills: %.1f hours (%.2f days)", transit_hrs, transit_hrs / 24.0f);
}

// ─── 🪖 Military Logistics, Divisions & Frontlines Tab ─────────────────────
void AeonGUI::draw_military_logistics_tab(AeonEngine& engine) {
    auto& mil = engine.military_engine;
    ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "🪖 MILITARY DIVISIONS, SUPPLY LOGISTICS & FRONTLINES");
    ImGui::Separator();

    ImGui::Text("ACTIVE FRONTLINE WAR ZONES: %zu", mil.frontlines.size());
    for (const auto& fz : mil.frontlines) {
        ImGui::Text("  🔥 Combat Zone: Civ %d vs Civ %d at Center (%d, %d) [Intensity: %.0f%%]",
            fz.civ1_id, fz.civ2_id, fz.center_x, fz.center_y, fz.intensity * 100.0f);
    }
    ImGui::Separator();

    ImGui::Text("MILITARY DIVISION DEPLOYMENTS:");
    if (ImGui::BeginTable("MilitaryUnitsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Unit Name");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Position");
        ImGui::TableSetupColumn("Personnel");
        ImGui::TableSetupColumn("Supply Line");
        ImGui::TableSetupColumn("Ammo/Fuel");
        ImGui::TableHeadersRow();

        for (const auto& div : mil.divisions) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%s", div.name.c_str());
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", mil.get_unit_type_name(div.type));
            ImGui::TableSetColumnIndex(2); ImGui::Text("(%d, %d)", div.x, div.y);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.0f / %.0f", div.personnel, div.max_personnel);
            ImGui::TableSetColumnIndex(4); ImGui::TextColored(div.supply_level > 0.6f ? ImVec4(0.2f,0.9f,0.2f,1) : ImVec4(0.9f,0.2f,0.2f,1), "%.0f%%", div.supply_level * 100.0f);
            ImGui::TableSetColumnIndex(5); ImGui::TextColored(div.fuel_ammo > 0.6f ? ImVec4(0.2f,0.9f,0.2f,1) : ImVec4(0.9f,0.2f,0.2f,1), "%.0f%%", div.fuel_ammo * 100.0f);
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    // Real-Time Tactical Battlefield Command Mode
    if (ImGui::CollapsingHeader("⚔️ 2D Tactical Battlefield Command Mode", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& tac = engine.tactical_battle_engine;
        if (!tac.active_battle) {
            if (ImGui::Button("⚔️ Launch Tactical Command Battle (Nordra vs Eldoria)")) {
                tac.start_battle(0, 1, "Highland Pass", engine);
            }
        } else {
            ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "TACTICAL BATTLE IN PROGRESS at %s (Turn %d)", tac.battle_location.c_str(), tac.turn_number);
            if (ImGui::Button("▶ End Turn / Execute Tactical Phase")) {
                tac.resolve_turn(engine);
            }

            // Draw 20x20 Tactical Grid Canvas
            ImVec2 t_pos = ImGui::GetCursorScreenPos();
            float cell_sz = 14.0f;
            ImDrawList* dl = ImGui::GetWindowDrawList();

            dl->AddRectFilled(t_pos, ImVec2(t_pos.x + 20 * cell_sz, t_pos.y + 20 * cell_sz), IM_COL32(20, 25, 35, 255));
            for (int y = 0; y < 20; ++y) {
                for (int x = 0; x < 20; ++x) {
                    dl->AddRect(ImVec2(t_pos.x + x * cell_sz, t_pos.y + y * cell_sz),
                                ImVec2(t_pos.x + (x + 1) * cell_sz, t_pos.y + (y + 1) * cell_sz), IM_COL32(40, 50, 60, 100));
                }
            }

            for (const auto& u : tac.units) {
                if (u.hp <= 0.0f) continue;
                float ux = t_pos.x + u.x * cell_sz + cell_sz * 0.5f;
                float uy = t_pos.y + u.y * cell_sz + cell_sz * 0.5f;
                ImU32 u_col = (u.civ_id == tac.attacker_civ_id) ? IM_COL32(255, 80, 80, 255) : IM_COL32(80, 140, 255, 255);
                dl->AddCircleFilled(ImVec2(ux, uy), 4.5f, u_col);
            }

            ImGui::Dummy(ImVec2(280.0f, 290.0f));

            ImGui::Text("UNIT REGIMENT LIST:");
            for (const auto& u : tac.units) {
                if (u.hp <= 0.0f) continue;
                ImGui::Text("  %s | Pos:[%d, %d] | HP:%.0f/%.0f | ATK:%.0f | DEF:%.0f",
                    u.name.c_str(), u.x, u.y, u.hp, u.max_hp, u.attack_power, u.defense);
            }
        }
    }
}


// ─── 💱 National Currencies & Central Banking Tab ───────────────────────────
void AeonGUI::draw_central_banking_tab(AeonEngine& engine) {
    auto& bank = engine.central_bank_engine;
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.4f, 1.0f), "💱 NATIONAL CURRENCIES, CENTRAL BANKING & WAR INFLATION");
    ImGui::Separator();

    if (ImGui::BeginTable("CentralBankTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Empire");
        ImGui::TableSetupColumn("Currency");
        ImGui::TableSetupColumn("Money Supply (M)");
        ImGui::TableSetupColumn("Inflation Rate");
        ImGui::TableSetupColumn("Interest Rate");
        ImGui::TableSetupColumn("Fx vs SOL");
        ImGui::TableHeadersRow();

        for (const auto& cur : bank.currencies) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%s", cur.civ_name.c_str());
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s (%s)", cur.currency_name.c_str(), cur.symbol.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::Text("$%.1f B", cur.money_supply_millions / 1000.0);
            ImGui::TableSetColumnIndex(3); ImGui::TextColored(cur.inflation_rate > 5.0f ? ImVec4(0.9f,0.2f,0.2f,1) : ImVec4(0.2f,0.9f,0.2f,1), "%.1f%%", cur.inflation_rate);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%.2f%%", cur.interest_rate);
            ImGui::TableSetColumnIndex(5); ImGui::Text("1 %s = %.2f SOL", cur.symbol.c_str(), cur.exchange_rate_vs_reserve);
        }
        ImGui::EndTable();
    }
}

// ─── 🤖 Autonomous Agent & Governor Controls Tab ──────────────────────────────
void AeonGUI::draw_agent_control_tab(AeonEngine& engine) {
    auto& ag_eng = engine.agent_engine;
    auto& agent  = ag_eng.agent;

    ImGui::TextColored(ImVec4(0.0f, 0.9f, 1.0f, 1.0f), "🤖 AUTONOMOUS AI AGENT & IMPERIAL GOVERNOR CONTROLS");
    ImGui::Separator();

    ImGui::Text("AGENT AVATAR: %s", agent.name.c_str());
    ImGui::Text("Position: Grid (%d, %d)  |  Gold: %d  |  Energy: %.0f%%  |  Health: %.0f%%",
        agent.x, agent.y, agent.gold, agent.energy, agent.health);
    ImGui::Separator();

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "CONTROL MODE SELECTION:");
    if (agent.mode == ControlMode::MANUAL) {
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.2f, 1.0f), "Current Mode: 🎮 MANUAL PLAYER CONTROL");
        if (ImGui::Button("Switch to 🤖 Autonomous AI Auto-Pilot")) {
            ag_eng.set_control_mode(ControlMode::AUTONOMOUS_AI);
        }
    } else {
        ImGui::TextColored(ImVec4(0.0f, 0.8f, 1.0f, 1.0f), "Current Mode: 🤖 AUTONOMOUS AI AUTO-PILOT");
        if (ImGui::Button("Switch to 🎮 Manual Player Control")) {
            ag_eng.set_control_mode(ControlMode::MANUAL);
        }
    }
    ImGui::Separator();

    if (agent.mode == ControlMode::MANUAL) {
        ImGui::Text("🎮 MANUAL MOVEMENT D-PAD:");
        if (ImGui::Button("  ⬆️ UP  "))    ag_eng.move_manual( 0, -1, engine);
        ImGui::SameLine();
        if (ImGui::Button("  ⬇️ DOWN  "))  ag_eng.move_manual( 0,  1, engine);
        ImGui::SameLine();
        if (ImGui::Button("  ⬅️ LEFT  "))  ag_eng.move_manual(-1,  0, engine);
        ImGui::SameLine();
        if (ImGui::Button("  ➡️ RIGHT  ")) ag_eng.move_manual( 1,  0, engine);
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "🤖 AI AGENT REASONING & DECISION LOG:");
    ImGui::TextWrapped("%s", agent.last_ai_reasoning.c_str());
    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.9f, 1.0f), "👑 IMPERIAL GOVERNOR AUTO-PILOT TAKEOVER:");
    if (ag_eng.global_ai_governor_active) {
        ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "  Status: AI Governor in full command of imperial decree execution.");
        if (ImGui::Button("Disable Imperial AI Governor Takeover")) {
            ag_eng.global_ai_governor_active = false;
        }
    } else {
        ImGui::Text("  Status: Human Sovereign in command of imperial laws.");
        if (ImGui::Button("Enable Imperial AI Governor Takeover")) {
            ag_eng.global_ai_governor_active = true;
        }
    }
}



// ─── 🛰️ Space Race & Off-World Mining Tab ────────────────────────────────────
void AeonGUI::draw_space_race_tab(AeonEngine& engine) {
    auto& space = engine.space_race_engine;
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "🛰️ SPACE RACE & OFF-WORLD MINING COLONIES");
    ImGui::Separator();

    if (ImGui::BeginTable("SpaceTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Empire");
        ImGui::TableSetupColumn("Space Milestone");
        ImGui::TableSetupColumn("Budget");
        ImGui::TableSetupColumn("Lunar Pop");
        ImGui::TableSetupColumn("Mars Pop");
        ImGui::TableSetupColumn("Helium-3 Mined");
        ImGui::TableHeadersRow();

        for (const auto& sp : space.programs) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%s", sp.civ_name.c_str());
            ImGui::TableSetColumnIndex(1); ImGui::TextColored(ImVec4(0.4f,0.9f,1.0f,1), "%s", space.get_milestone_name(sp.highest_milestone));
            ImGui::TableSetColumnIndex(2); ImGui::Text("$%.0f M", sp.space_budget_millions);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%d", sp.lunar_colonists);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%d", sp.mars_colonists);
            ImGui::TableSetColumnIndex(5); ImGui::Text("%.1f Tons", sp.helium3_mined_tons);
        }
        ImGui::EndTable();
    }
}

// ─── ⚔️ Breakaway Rebel Factions & Civil Wars Tab ──────────────────────────────
void AeonGUI::draw_rebellion_tab(AeonEngine& engine) {
    auto& reb = engine.rebellion_engine;
    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "⚔️ BREAKAWAY REBEL FACTIONS & CIVIL WARS");
    ImGui::Separator();

    ImGui::Text("ACTIVE REBELLIONS: %zu", reb.rebellions.size());
    if (reb.rebellions.empty()) {
        ImGui::Text("  No active secessions or civil war insurgencies.");
    } else {
        for (const auto& r : reb.rebellions) {
            ImGui::Text("  🔥 %s (Civ %d Secession) | Ideology: %s | Strength: %.0f",
                r.name.c_str(), r.parent_civ_id, r.ideology.c_str(), r.strength);
        }
    }
    ImGui::Separator();

    ImGui::Text("CIVILIAN STABILITY MONITOR:");
    for (const auto& civ : engine.civs) {
        if (civ.is_commons) continue;
        ImGui::Text("  %s: Stability %.1f%% %s", civ.name.c_str(), civ.stability,
            civ.stability < 35.0f ? "[⚠️ CRITICAL UNREST - REBELLION IMMINENT]" : "[STABLE]");
        if (civ.stability < 35.0f) {
            ImGui::SameLine();
            std::string btn_label = "Trigger Secession (" + civ.name + ")";
            if (ImGui::Button(btn_label.c_str())) {
                reb.trigger_secession(civ.id, engine);
            }
        }
    }
}

// ─── ☢️ Nuclear Triad, MAD & Space Elevator Kinetic Bombardment Tab ──────────
void AeonGUI::draw_nuclear_tab(AeonEngine& engine) {

    auto& nuc = engine.nuclear_engine;
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "☢️ STRATEGIC NUCLEAR TRIAD, MAD & KINETIC BOMBARDMENT");
    ImGui::Separator();

    ImGui::Text("GLOBAL NUCLEAR STATUS: %s", nuc.global_nuclear_winter ? "☢️ DEFCON 1 - NUCLEAR WINTER" : "🟢 DEFCON 5 - STABLE PEACE");
    ImGui::Separator();

    for (const auto& ars : nuc.nuclear_arsenals) {
        std::string cname = (ars.civ_id >= 0 && ars.civ_id < (int)engine.civs.size()) ? engine.civs[ars.civ_id].name : "Unknown";
        ImGui::Text("Empire: %s | ICBMs: %d | SSBN Submarines: %d | Strategic Bombers: %d | Total Warheads: %d",
            cname.c_str(), ars.icbm_silos, ars.ssbn_submarines, ars.stealth_bombers, ars.warheads_total);

    }

    ImGui::Separator();
    // Space Elevators & Kinetic Bombardment
    if (ImGui::CollapsingHeader("🛰️ Space Elevators & Kinetic Bombardment (Rods from God)", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& kinetic = engine.kinetic_strike_engine;
        ImGui::Text("EQUATORIAL SPACE ELEVATOR SPIRES:");
        for (const auto& el : kinetic.elevators) {
            ImGui::Text("  🛰️ %s (Pos: [%d, %d]) — Status: %s",
                el.location_name.c_str(), el.x, el.y, el.operational ? "OPERATIONAL ✅" : "Under Construction (45%)");
        }
        ImGui::Separator();

        ImGui::Text("ORBITAL KINETIC ROD PLATFORMS:");
        for (const auto& sat : kinetic.kinetic_satellites) {
            std::string cname = (sat.civ_id >= 0 && sat.civ_id < (int)engine.civs.size()) ? engine.civs[sat.civ_id].name : "Unknown";
            ImGui::Text("  ⚡ Sat #%d (%s) | Tungsten Rods: %d/6 | Orbit: %.0f km",
                sat.id, cname.c_str(), sat.tungsten_rods, sat.orbit_alt_km);
        }

        static int k_target_x = 184, k_target_y = 40;
        ImGui::InputInt("Kinetic Target X", &k_target_x);
        ImGui::InputInt("Kinetic Target Y", &k_target_y);
        if (ImGui::Button("⚡ FIRE KINETIC TUNGSTEN ROD (Rods from God Mach 10 Strike)")) {
            kinetic.launch_kinetic_strike(0, k_target_x, k_target_y, engine);
        }
    }
}

// ─── 👑 Royal Dynasties & Lineages Tab ─────────────────────────────────────────
void AeonGUI::draw_dynasty_tab(AeonEngine& engine) {

    auto& dyn = engine.dynasty_engine;
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "👑 ROYAL DYNASTIES, MARRIAGES & SUCCESSION CRISES");
    ImGui::Separator();

    ImGui::Text("ROYAL LINEAGE REGISTER:");
    for (const auto& r : dyn.royals) {
        std::string cname = (r.civ_id >= 0 && r.civ_id < (int)engine.civs.size()) ? engine.civs[r.civ_id].name : "Unknown Realm";
        ImGui::Text("  👑 %s (%s) | Age: %d | Title: %s | Status: %s",
            r.name.c_str(), cname.c_str(), r.age, r.title.c_str(),
            r.married ? "Married" : "Single");
    }
    ImGui::Separator();

    ImGui::Text("DYNASTIC MARRIAGE ALLIANCES:");
    if (dyn.dynastic_marriages.empty()) {
        ImGui::Text("  No active royal marriages.");
    } else {
        for (const auto& da : dyn.dynastic_marriages) {
            ImGui::Text("  💍 %s (Trust Bonus: +%.0f)", da.marriage_description.c_str(), da.trust_bonus);
        }
    }
    ImGui::Separator();

    if (ImGui::Button("💍 Seal Royal Marriage (Nordra & Valoria)")) {
        dyn.arrange_royal_marriage(0, 1, engine);
    }
}



// ─── 🏗️ Mega-Engineering World Wonders Tab ────────────────────────────────────
void AeonGUI::draw_megawonders_tab(AeonEngine& engine) {

    auto& mega = engine.megawonder_engine;
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "🏗️ MEGA-ENGINEERING WORLD WONDERS");
    ImGui::Separator();

    for (const auto& mw : mega.mega_wonders) {
        std::string owner_name = (mw.owner_civ_id >= 0 && mw.owner_civ_id < (int)engine.civs.size()) ? engine.civs[mw.owner_civ_id].name : "None";
        ImGui::Text("  🏛️ %s [%s] | Owner: %s | Tile: (%d, %d)",
            mw.name.c_str(), mw.type.c_str(),
            owner_name.c_str(),
            mw.tile_x, mw.tile_y);
        ImGui::ProgressBar(mw.construction_progress / 100.0f, ImVec2(-1.0f, 0.0f),
            mw.completed ? "COMPLETED (+30% GDP)" : "Under Construction");
    }
    ImGui::Separator();

    if (ImGui::Button("🏗️ Begin Construction: Orbital Defense Ring (Solaria)")) {
        mega.start_construction(4, "Orbital Defense Ring Grid", engine);
    }
}

// ─── ⛵ Maritime Shipping Lanes, Blockades & Piracy Tab ──────────────────────
void AeonGUI::draw_maritime_tab(AeonEngine& engine) {
    auto& mari = engine.maritime_engine;
    ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "⛵ MARITIME SHIPPING LANES, BLOCKADES & PIRACY");
    ImGui::Separator();

    ImGui::Text("ACTIVE CARGO VESSELS: %zu", mari.cargo_ships.size());
    for (const auto& cs : mari.cargo_ships) {
        ImGui::Text("  🚢 %s -> %s | Position: (%d, %d) | Value: $%.0f Gold %s",
            cs.origin_port.c_str(), cs.dest_port.c_str(), cs.x, cs.y, cs.cargo_value_gold,
            cs.intercepted_by_pirates ? "[⚠️ RAIDED BY PIRATES]" : "[SAFE]");
    }
    ImGui::Separator();

    ImGui::Text("NAVAL BLOCKADES:");
    if (mari.active_blockades.empty()) {
        ImGui::Text("  No active naval blockades.");
    } else {
        for (const auto& nb : mari.active_blockades) {
            std::string att_name = (nb.attacker_civ_id >= 0 && nb.attacker_civ_id < (int)engine.civs.size()) ? engine.civs[nb.attacker_civ_id].name : "Unknown Attacker";
            ImGui::Text("  ⚓ %s Blockaded by %s Warships!",
                nb.port_name.c_str(), att_name.c_str());
        }
    }
    ImGui::Separator();

    if (ImGui::Button("⚓ Enact Naval Blockade (Nordra -> Valoria)")) {
        mari.enact_naval_blockade(0, 1, engine);
    }
}

// ─── 👤 Individual Citizens & Characters Tab ─────────────────────────────────
void AeonGUI::draw_citizens_tab(AeonEngine& engine) {
    auto& cit = engine.citizen_engine;
    ImGui::TextColored(ImVec4(0.4f, 0.9f, 1.0f, 1.0f), "👤 INDIVIDUAL CITIZENS & CHARACTERS DIRECTORY");
    ImGui::Separator();

    if (ImGui::BeginTable("CitizensTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Realm");
        ImGui::TableSetupColumn("Role / Profession");
        ImGui::TableSetupColumn("Age");
        ImGui::TableSetupColumn("Health");
        ImGui::TableSetupColumn("Wealth");
        ImGui::TableSetupColumn("Loyalty");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();

        for (auto& c : cit.citizens) {
            if (!c.is_alive) continue;
            std::string realm_name = (c.civ_id >= 0 && c.civ_id < (int)engine.civs.size()) ? engine.civs[c.civ_id].name : "The Commons";
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("👤 %s", c.name.c_str());
            ImGui::TableSetColumnIndex(1); ImGui::Text("%s", realm_name.c_str());
            ImGui::TableSetColumnIndex(2); ImGui::TextColored(ImVec4(0.4f,0.8f,1.0f,1), "%s", c.get_role_string().c_str());
            ImGui::TableSetColumnIndex(3); ImGui::Text("%d yrs", c.age);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%.0f%%", c.health);
            ImGui::TableSetColumnIndex(5); ImGui::Text("$%.0f", c.wealth_gold);
            ImGui::TableSetColumnIndex(6); ImGui::TextColored(c.loyalty < 50.0f ? ImVec4(1,0.3f,0.3f,1) : ImVec4(0.3f,1,0.3f,1), "%.0f%%", c.loyalty);

            ImGui::TableSetColumnIndex(7);
            std::string p_id = "Promote##" + std::to_string(c.id);
            if (ImGui::Button(p_id.c_str())) {
                cit.promote_citizen(c.id, CitizenRole::GOVERNOR, engine);
            }
            ImGui::SameLine();
            std::string e_id = "Exile##" + std::to_string(c.id);
            if (ImGui::Button(e_id.c_str())) {
                cit.exile_citizen(c.id, engine);
            }
            ImGui::SameLine();
            std::string a_id = "Assassinate##" + std::to_string(c.id);
            if (ImGui::Button(a_id.c_str())) {
                cit.assassinate_citizen(c.id, engine);
            }
        }
        ImGui::EndTable();
    }
}

// ─── ⚡ God Mode Control Console Tab ──────────────────────────────────────────
void AeonGUI::draw_god_mode_tab(AeonEngine& engine) {
    ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "⚡ GOD MODE -- DIVINE REALM CONTROL CONSOLE");
    ImGui::Text("You hold omnipotent control over all civilizations, economics, weather, and destiny.");
    ImGui::Separator();

    // 1. Economic & Divine Interventions
    if (ImGui::CollapsingHeader("💰 Divine Intervention & Treasury Boosts", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("💰 Infuse +$100,000 GDP to All Empires")) {
            for (auto& civ : engine.civs) civ.economy.gdp += 100000.0f;
            engine.history.record(engine.year, engine.month, "GOD_MODE", "Divine GDP Surge", "All empire economies boosted by $100k.", 0);
        }
        ImGui::SameLine();
        if (ImGui::Button("🕊️ Restore World Stability (Set Stability = 100%)")) {
            for (auto& civ : engine.civs) civ.stability = 100.0f;
            engine.history.record(engine.year, engine.month, "GOD_MODE", "Divine Pax Aeona", "All civil unrest erased.", 0);
        }

        if (ImGui::Button("⚛️ Grant +25% Tech Progress to All")) {
            for (auto& civ : engine.civs) civ.tech.progress = std::min(100.0f, civ.tech.progress + 25.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("☢️ Arm All Empires (+5 Strategic Warheads)")) {
            for (auto& n : engine.nuclear_engine.nuclear_arsenals) n.warheads_total += 5;
        }
    }

    // 2. Disaster Creation Suite
    if (ImGui::CollapsingHeader("🌋 Spawn Cosmic & Environmental Disasters", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("☄️ Trigger Cataclysmic Meteor Strike")) {
            engine.disaster_engine.trigger_disaster(engine, DisasterType::METEOR_STRIKE);
        }
        ImGui::SameLine();
        if (ImGui::Button("🌋 Trigger Volcanic Eruption")) {
            engine.disaster_engine.trigger_disaster(engine, DisasterType::VOLCANIC_ERUPTION);
        }

        if (ImGui::Button("☣️ Unleash Worldwide Bio Plague")) {
            engine.disaster_engine.trigger_disaster(engine, DisasterType::BIO_PLAGUE);
        }
        ImGui::SameLine();
        if (ImGui::Button("☀️ Trigger Solar Flare Electromagnetic Storm")) {
            engine.disaster_engine.trigger_disaster(engine, DisasterType::SOLAR_FLARE);
        }
    }

    // 3. World War & Peace Edicts
    if (ImGui::CollapsingHeader("⚔️ World War & Global Peace Directives", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("🔥 Force World War (Declare War Nordra vs Valoria)")) {
            if (engine.civs.size() >= 2) {
                engine.civs[0].at_war = true;
                engine.civs[0].war_with_civ = 1;
                engine.civs[1].at_war = true;
                engine.civs[1].war_with_civ = 0;
                engine.history.record(engine.year, engine.month, "WAR", "WORLD WAR FORCED BY DIVINE DECREE", "Nordra & Valoria forced into total war.", 0);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("🕊️ Force Global Ceasefire & Peace Pacts")) {
            for (auto& civ : engine.civs) {
                civ.at_war = false;
                civ.war_with_civ = -1;
            }
            engine.history.record(engine.year, engine.month, "PEACE", "GLOBAL PEACE DECREED", "All active wars terminated by divine order.", 0);
        }
    }

    // 4. Ollama World God Direct Decrees
    if (ImGui::CollapsingHeader("🤖 Ollama LLM Divine Decree Prompt Injector", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Enter a custom prompt to command the Ollama LLM World God:");
        ImGui::InputText("Custom Decree", god_custom_decree_buf_, IM_ARRAYSIZE(god_custom_decree_buf_));
        if (ImGui::Button("⚡ Submit Divine Decree to Ollama LLM")) {
            std::string ctx = "Year " + std::to_string(engine.year) + ", Civs count: " + std::to_string(engine.civs.size());
            std::string decree = AeonOllama::generate_custom_decree(god_custom_decree_buf_, ctx);
            engine.history.record(engine.year, engine.month, "OLLAMA_GOD", "Custom LLM Decree Issued", decree, 0);
        }
    }

    // 5. Scenario Creator & Custom World Map Painter
    if (ImGui::CollapsingHeader("🕹️ Scenario Creator & Custom Map Painter Engine", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& editor = engine.scenario_editor_engine;
        ImGui::Text("Brush Tool:");
        if (ImGui::RadioButton("🖌️ Biomes", editor.current_brush == EditorBrushMode::PAINT_BIOME)) { editor.current_brush = EditorBrushMode::PAINT_BIOME; } ImGui::SameLine();
        if (ImGui::RadioButton("🚩 Territory Borders", editor.current_brush == EditorBrushMode::PAINT_SOVEREIGNTY)) { editor.current_brush = EditorBrushMode::PAINT_SOVEREIGNTY; } ImGui::SameLine();
        if (ImGui::RadioButton("💎 Resources", editor.current_brush == EditorBrushMode::PLACE_RESOURCE)) { editor.current_brush = EditorBrushMode::PLACE_RESOURCE; } ImGui::SameLine();
        if (ImGui::RadioButton("🗿 Wonders", editor.current_brush == EditorBrushMode::PLACE_LANDMARK)) editor.current_brush = EditorBrushMode::PLACE_LANDMARK;

        ImGui::SliderInt("Brush Radius", &editor.brush_radius, 1, 5);


        static char scenario_save_buf[64] = "custom_earth_scenario";
        ImGui::InputText("Scenario Name", scenario_save_buf, sizeof(scenario_save_buf));
        if (ImGui::Button("💾 Save Scenario File (.json)")) {
            editor.save_scenario(engine, scenario_save_buf);
        }
        ImGui::SameLine();
        if (ImGui::Button("📂 Load Scenario File (.json)")) {
            editor.load_scenario(engine, scenario_save_buf);
        }
    }

}

// ─── ⚡ Interactive Crisis Choice Popup Modal ───────────────────────────────
void AeonGUI::draw_interactive_event_modal(AeonEngine& engine) {
    if (!engine.show_event_modal) return;

    ImGui::OpenPopup("CRISIS EVENT MODAL");
    if (ImGui::BeginPopupModal("CRISIS EVENT MODAL", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "🚨 CRITICAL STATE CRISIS DETECTED!");
        ImGui::Separator();
        ImGui::TextWrapped("Crisis Details: %s", engine.active_modal_event.description.c_str());
        ImGui::Separator();

        ImGui::Text("Select Imperial Directive:");

        if (ImGui::Button("Option 1: Deploy Emergency State Relief Aid")) {
            engine.history.record(engine.year, engine.month, "EVENT", "Crisis Aid Deployed", "State funds deployed for relief.", 0);
            engine.show_event_modal = false;
        }
        if (ImGui::Button("Option 2: Mobilize National Guard & Suppress Unrest")) {
            engine.history.record(engine.year, engine.month, "EVENT", "Military Mobilization", "Troops deployed to restore order.", 0);
            engine.show_event_modal = false;
        }
        if (ImGui::Button("Option 3: Request International Council Intervention")) {
            engine.history.record(engine.year, engine.month, "EVENT", "UN Assistance Requested", "Appealed to world powers for aid.", 0);
            engine.show_event_modal = false;
        }

        ImGui::EndPopup();
    }
}

} // namespace Aeon
