#include "argparse.h"
#include "common.h"
#include "algorithms/deferred_shading.h"
#include "scene.h"
#include <variant>

#include <spdlog/spdlog.h>

using AlgorithmVariant
  = std::variant<std::unique_ptr<Algorithms::DeferredShading>>;
AlgorithmVariant g_AlgorithmP{std::make_unique<Algorithms::DeferredShading>()};

bool g_ShowLightCenters = true;   // Render lights
bool g_ShowLightRange = false;    // Render lights' ranges
bool g_ExplicitTimerSync = false; // Explicit synchronization will be made
                                  // before any performance measurement

void display() {
    std::visit([](auto&& algo) { algo->run(); }, g_AlgorithmP);
    if (g_ShowLightCenters) Scene::get().lights.renderLightCenters();
    if (g_ShowLightRange) Scene::get().lights.renderLightRanges();
}

void init() {
    // Default scene distance
    Variables::Transform.SceneZOffset = 8.0f;

    std::visit([](auto&& algo) { algo->initialize(); }, g_AlgorithmP);

    // Load shader program
    compileShaders();
}

constexpr const char* help_message = "no help here";

void resetAlgorithm() {
    std::visit([](auto&& algo) { algo->reset(); }, g_AlgorithmP);
}

int showGUI() {
    int menuHeight = 305;
    const int oneInt = 1;
    const int itemWidth = 140 * IMGUI_RESIZE_FACTOR;

    ImGui::Begin("Render");
    ImGui::SetWindowSize(glm::vec2(220, menuHeight) * IMGUI_RESIZE_FACTOR,
                         ImGuiCond_Always);
    ImGui::SetNextItemWidth(itemWidth);
    // if (ImGui::Combo("shading", &g_Algorithm, "Forward\0Deferred\0"))
    //     resetAlgorithm();
    ImGui::Checkbox("rotate lights", &Scene::get().lights.rotate);
    ImGui::Checkbox("show lights", &g_ShowLightCenters);
    ImGui::Checkbox("show light range", &g_ShowLightRange);
    if (ImGui::Checkbox("synchronize timers", &g_ExplicitTimerSync)) {
        resetAlgorithm();
    }
    ImGui::SameLine(170.0f * IMGUI_RESIZE_FACTOR, -10.0f * IMGUI_RESIZE_FACTOR);
    if (ImGui::Button("reset")) resetAlgorithm();

    ImGui::Separator();
    ImGui::Text("SPHERES");
    ImGui::SetNextItemWidth(itemWidth);
    auto& numSpheresPerRow = Scene::get().spheres.numSpheresPerRow;
    if (ImGui::InputScalar("per row", ImGuiDataType_S32, &numSpheresPerRow,
                           &oneInt)) {
        numSpheresPerRow = glm::clamp(numSpheresPerRow, 1, 100);
        Scene::get().lights.create(static_cast<float>(numSpheresPerRow));
        resetAlgorithm();
    }
    ImGui::SetNextItemWidth(itemWidth);
    auto& numSphereSlices = Scene::get().spheres.numSphereSlices;
    if (ImGui::InputScalar("slices", ImGuiDataType_S32, &numSphereSlices,
                           &oneInt)) {
        numSphereSlices = glm::clamp(numSphereSlices, 5, 100);
        resetAlgorithm();
    }

    ImGui::Separator();
    ImGui::Text("LIGHTS");
    ImGui::SetNextItemWidth(itemWidth);
    auto& numLights = Scene::get().lights.numLights;
    if (ImGui::InputScalar("count", ImGuiDataType_S32, &numLights, &oneInt)) {
        numLights = glm::clamp(numLights, 1, Scene::MAX_LIGHTS);
        Scene::get().lights.create(static_cast<float>(numSpheresPerRow));
        resetAlgorithm();
    }
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::SliderFloat("speed", &Scene::get().lights.rotationSpeed, 0.0f, 0.2f);

    ImGui::SetNextItemWidth(56 * IMGUI_RESIZE_FACTOR);
    auto& lightRangeLimits = Scene::get().lights.rangeLimits;
    if (ImGui::SliderFloat("##x", &lightRangeLimits.x, 0.01f,
                           lightRangeLimits.y, "%.2f")) {
        Scene::get().lights.genRandomRadiuses();
        resetAlgorithm();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(56 * IMGUI_RESIZE_FACTOR);
    if (ImGui::SliderFloat("##y", &lightRangeLimits.y, lightRangeLimits.x, 8.0f,
                           "%.2f")) {
        Scene::get().lights.genRandomRadiuses();
        resetAlgorithm();
    }
    ImGui::SameLine();
    ImGui::Text("range");
    ImGui::End();

    std::visit([](auto&& algo) { algo->gui(glm::ivec2(235, 10), 240); },
               g_AlgorithmP);

    return menuHeight;
}

void compileShaders(void* clientData) {
    spdlog::debug("compileShaders top level function called, calling "
                  "algorithm.reset(true)");
    glUseProgram(0);
    std::visit([](auto&& algo) { algo->reset(true); }, g_AlgorithmP);
}

//-----------------------------------------------------------------------------
// Name: keyboardChanged()
// Desc:
//-----------------------------------------------------------------------------
void keyboardChanged(int key, int action, int mods) {
    // switch (key) {
    //     case GLFW_KEY_W:
    //         // g_WireMode = !g_WireMode;
    //         break;
    //     default:
    //         break;
    // }
}

void windowResized(const glm::ivec2& resolution) {
    spdlog::trace("Window resized");
    std::visit(
      [&resolution](auto&& algo) { algo->onWindowResized(resolution); },
      g_AlgorithmP);
}

void destroyAlgorithm() {
    spdlog::trace("Destroying algorithm");
    std::visit([](auto&& algo) { algo.reset(nullptr); }, g_AlgorithmP);
}

void setLogLevel(std::string_view levelStr) {
    if (levelStr == "debug") {
        spdlog::set_level(spdlog::level::debug);
    } else if (levelStr == "trace") {
        spdlog::set_level(spdlog::level::trace);
    } else if (levelStr == "info") {
        spdlog::set_level(spdlog::level::info);
    } else if (levelStr == "warn") {
        spdlog::set_level(spdlog::level::warn);
    } else if (levelStr == "err") {
        spdlog::set_level(spdlog::level::err);
    } else if (levelStr == "critical") {
        spdlog::set_level(spdlog::level::critical);
    } else if (levelStr == "off") {
        spdlog::set_level(spdlog::level::off);
    } else {
        spdlog::warn("Unknown log level {}, using default", levelStr);
    }
}

//-----------------------------------------------------------------------------
// Name: main()
// Desc:
//-----------------------------------------------------------------------------
int main(int argc, char** argv) {
    constexpr int OGL_CONFIGURATION[] = {GLFW_CONTEXT_VERSION_MAJOR,
                                         4,
                                         GLFW_CONTEXT_VERSION_MINOR,
                                         6,
                                         GLFW_OPENGL_FORWARD_COMPAT,
                                         GL_FALSE.m_value,
                                         GLFW_OPENGL_DEBUG_CONTEXT,
                                         GL_TRUE.m_value,
                                         GLFW_OPENGL_PROFILE,
                                         GLFW_OPENGL_CORE_PROFILE,
                                         PGR2_SHOW_MEMORY_STATISTICS,
                                         GL_TRUE.m_value,
                                         0};

    printf("%s\n", help_message);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");

    auto args = argparse(argc, argv);
    if (auto it = args.keyValueArgs.find("loglevel");
        it != args.keyValueArgs.end()) {
        setLogLevel(it->second);
    }

    return common_main(
      1200, 900, "[PGR2] Cornell Box",
      static_cast<const int*>(OGL_CONFIGURATION), // OGL configuration hints
      init,                                       // Init GL callback function
      destroyAlgorithm, // Release GL callback function
      showGUI,          // Show GUI callback function
      display,          // Display callback function
      windowResized,    // Window resize callback function
      keyboardChanged,  // Keyboard callback function
      nullptr,          // Mouse button callback function
      nullptr);         // Mouse motion callback function
}
