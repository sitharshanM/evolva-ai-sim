cmake_minimum_required(VERSION 3.20)
project(DigitalLife VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ── GLFW ──────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ── GLM ───────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

# ── nlohmann/json (single-header, URL download – no git) ─────────────────────
FetchContent_Declare(
    json
    URL      https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(json)

# ── ImGui (URL tarball – no git) ──────────────────────────────────────────────
FetchContent_Declare(
    imgui
    URL      https://github.com/ocornut/imgui/archive/refs/tags/v1.90.8.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# ── GLAD (URL tarball – no git) ────────────────────────────────────────────────
FetchContent_Declare(
    glad
    URL      https://github.com/Dav1dde/glad/archive/refs/tags/v0.1.36.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_GetProperties(glad)
if(NOT glad_POPULATED)
    FetchContent_Populate(glad)
    set(GLAD_PROFILE   "core" CACHE STRING "" FORCE)
    set(GLAD_API       "gl=3.3" CACHE STRING "" FORCE)
    set(GLAD_GENERATOR "c"    CACHE STRING "" FORCE)
    set(GLAD_EXTENSIONS ""    CACHE STRING "" FORCE)
    file(READ ${glad_SOURCE_DIR}/CMakeLists.txt GLAD_CMAKE)
    string(REPLACE "VERSION 3.0" "VERSION 3.5" GLAD_CMAKE_MOD "${GLAD_CMAKE}")
    file(WRITE ${glad_SOURCE_DIR}/CMakeLists.txt "${GLAD_CMAKE_MOD}")
    add_subdirectory(${glad_SOURCE_DIR} ${glad_BINARY_DIR})
endif()

# ── Project Sources (AEON Engine) ─────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/sound_engine.cpp
    src/aeon_ascii_map.cpp
    src/aeon_character.cpp
    src/aeon_civilization.cpp
    src/aeon_government.cpp
    src/aeon_test_government.cpp
    src/aeon_ruler_ai.cpp
    src/aeon_ollama.cpp
    src/aeon_economy.cpp
    src/aeon_religion.cpp
    src/aeon_persistence.cpp
    src/aeon_chronicler.cpp
    src/aeon_history.cpp
    src/aeon_engine.cpp
    src/aeon_president.cpp
    src/aeon_space_espionage.cpp
    src/aeon_coalition.cpp
    src/aeon_tech_tree.cpp
    src/aeon_trade_caravan.cpp
    src/aeon_disasters.cpp
    src/aeon_wonders.cpp
    src/aeon_economy_market.cpp
    src/aeon_naval.cpp
    src/aeon_parliament.cpp
    src/aeon_un_council.cpp
    src/aeon_alliances.cpp
    src/aeon_gis_climate.cpp
    src/aeon_supply_demand.cpp
    src/aeon_demographics.cpp
    src/aeon_military.cpp
    src/aeon_central_bank.cpp
    src/aeon_autonomous_agent.cpp
    src/aeon_rebellion.cpp
    src/aeon_nuclear.cpp
    src/aeon_space_race.cpp
    src/aeon_dynasty.cpp
    src/aeon_megawonders.cpp
    src/aeon_maritime.cpp
    src/aeon_individual_citizens.cpp

    src/aeon_analytics.cpp
    src/aeon_diplomatic_summit.cpp
    src/aeon_genetics.cpp
    src/aeon_tactical_battle.cpp
    src/aeon_scenario_editor.cpp
    src/aeon_ruler_psyche.cpp
    src/aeon_deep_state.cpp
    src/aeon_kinetic_strike.cpp
    src/aeon_gemini.cpp
    src/aeon_nemotron.cpp
    src/aeon_openrouter.cpp
    src/aeon_cli.cpp


    src/aeon_gui.cpp
    ${IMGUI_SOURCES}
)


add_executable(DigitalLife ${SOURCES})

target_include_directories(DigitalLife PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${glad_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(DigitalLife PRIVATE
    glad
    glfw
    glm::glm
    nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(DigitalLife PRIVATE opengl32 winhttp ws2_32 gdiplus wininet)
    # Avoid annoying Windows min/max macro conflicts
    target_compile_definitions(DigitalLife PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
endif()


if(MSVC)
    target_compile_options(DigitalLife PRIVATE /W3 /MP /openmp)
else()
    target_compile_options(DigitalLife PRIVATE -Wall -Wextra -O2 -fopenmp)
    target_link_libraries(DigitalLife PRIVATE -fopenmp)
endif()

# ── Copy shaders and assets to binary dir ──────────────────────────────────────
add_custom_command(TARGET DigitalLife POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:DigitalLife>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:DigitalLife>/assets
    COMMENT "Copying shaders and OpenStreetMap picture assets..."
)

