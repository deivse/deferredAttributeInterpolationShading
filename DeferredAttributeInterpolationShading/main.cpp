#include "argparse.h"
#include "common.h"
#include "algorithms/deferred_shading.h"
#include "algorithms/deferred_attribute_interpolation_shading.h"
#include "scene.h"
#include <variant>

#include <spdlog/spdlog.h>

namespace Algorithms {
using DAISUniquePtr = std::unique_ptr<DeferredAttributeInterpolationShading>;
using DSUniquePtr = std::unique_ptr<DeferredShading>;

using Variant = std::variant<DSUniquePtr, DAISUniquePtr>;
} // namespace Algorithms

enum class AlgorithmsEnum : int
{
    DS = 0,
    DAIS = 1,
};

Algorithms::Variant g_AlgorithmVariant{
  std::make_unique<Algorithms::DeferredShading>()};

bool g_ExplicitTimerSync = false; // Explicit synchronization will be made
                                  // before any performance measurement

void display() {
    std::visit([](auto&& algo) { algo->run(); }, g_AlgorithmVariant);
    Scene::get().lights.render(); // Render light centers/ranges if enabled
}

void init() {
    // Default scene distance
    Variables::Transform.SceneZOffset = 8.0f;

    std::visit([](auto&& algo) { algo->initialize(); }, g_AlgorithmVariant);

    // Load shader program
    compileShaders();
}

constexpr const char* help_message = "no help here";

void resetAlgorithm() {
    // this is NOT std::unique_ptr::reset!
    std::visit([](auto&& algo) { algo->reset(); }, g_AlgorithmVariant);
}

int showGUI() {
    int menuHeight = 305;
    const int oneInt = 1;
    const int itemWidth = 140 * IMGUI_RESIZE_FACTOR;

    static AlgorithmsEnum algorithm
      = std::holds_alternative<Algorithms::DSUniquePtr>(g_AlgorithmVariant)
          ? AlgorithmsEnum::DS
          : AlgorithmsEnum::DAIS;

    ImGui::Begin("Render");
    ImGui::SetWindowSize(glm::vec2(220, menuHeight) * IMGUI_RESIZE_FACTOR,
                         ImGuiCond_Always);
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::Combo(
          "Shading", reinterpret_cast<int*>(&algorithm),
          "DeferredShading\0Deferred Attribute Interpolation Shading\0")) {
        switch (algorithm) {
            case AlgorithmsEnum::DS:
                g_AlgorithmVariant
                  = std::make_unique<Algorithms::DeferredShading>();
                break;
            case AlgorithmsEnum::DAIS:
                g_AlgorithmVariant = std::make_unique<
                  Algorithms::DeferredAttributeInterpolationShading>();
                break;
        }
        std::visit([](auto&& algo) { algo->initialize(); }, g_AlgorithmVariant);
        resetAlgorithm();
    }

    static uint8_t MSAASamples;
    switch (std::visit([](auto&& algo) { return algo->getMSAASampleCount(); },
                       g_AlgorithmVariant)) {
        case 4:
            MSAASamples = 1;
            break;
        case 8:
            MSAASamples = 2;
            break;
        default:
            MSAASamples = 0;
    }

    if (ImGui::Combo(
          "MSAA/SSAA", reinterpret_cast<int*>(&MSAASamples),
          "Disabled\0" // NOLINT(bugprone-string-literal-with-embedded-nul)
          "x4\0"
          "x8")) {
        if (MSAASamples > 0) MSAASamples = 1 << (MSAASamples + 1);
        std::visit([](auto&& algo) { algo->setMSAASampleCount(MSAASamples); },
                   g_AlgorithmVariant);
    }

    ImGui::Checkbox("Rotate lights", &Scene::get().lights.rotate);
    ImGui::Checkbox("Show light centers",
                    &Scene::get().lights.showLightCenters);
    ImGui::Checkbox("Show light ranges", &Scene::get().lights.showLightRanges);
    if (ImGui::Checkbox("Synchronize timers", &g_ExplicitTimerSync)) {
        resetAlgorithm();
    }
    ImGui::SameLine(170.0f * IMGUI_RESIZE_FACTOR, -10.0f * IMGUI_RESIZE_FACTOR);
    if (ImGui::Button("Reset")) resetAlgorithm();

    ImGui::Separator();
    ImGui::Text("Spheres");
    ImGui::SetNextItemWidth(itemWidth);
    auto& numSpheresPerRow = Scene::get().spheres.numSpheresPerRow;
    if (ImGui::InputScalar("#(Spheres)/row", ImGuiDataType_S32,
                           &numSpheresPerRow, &oneInt)) {
        numSpheresPerRow = glm::clamp(numSpheresPerRow, 1, 100);
        Scene::get().lights.create(static_cast<float>(numSpheresPerRow));
        resetAlgorithm();
    }
    ImGui::SetNextItemWidth(itemWidth);
    auto& numSphereSlices = Scene::get().spheres.numSphereSlices;
    if (ImGui::InputScalar("#(Slices)", ImGuiDataType_S32, &numSphereSlices,
                           &oneInt)) {
        numSphereSlices = glm::clamp(numSphereSlices, 5, 100);
        resetAlgorithm();
    }
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::Text("#(Triangles): %zu",
                Scene::get().spheres.trianglesPerSphere
                  * static_cast<size_t>(std::pow(numSpheresPerRow, 3)));
    Scene::get().spheres.updateGeometry();

    ImGui::Separator();
    ImGui::Text("Lights");
    ImGui::SetNextItemWidth(itemWidth);
    auto& numLights = Scene::get().lights.numLights;
    if (ImGui::InputScalar("Count", ImGuiDataType_S32, &numLights, &oneInt)) {
        numLights = glm::clamp(numLights, 1, Scene::MAX_LIGHTS);
        Scene::get().lights.create(static_cast<float>(numSpheresPerRow));
        resetAlgorithm();
    }
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::SliderFloat("Speed", &Scene::get().lights.rotationSpeed, 0.0f, 0.2f);

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
    ImGui::Text("Range");
    ImGui::End();

    std::visit([](auto&& algo) { algo->gui(glm::ivec2(235, 10), 240); },
               g_AlgorithmVariant);

    return menuHeight;
}

void compileShaders(void* clientData) {
    spdlog::debug("compileShaders top level function called, calling "
                  "algorithm.reset(true)");
    glUseProgram(0);
    std::visit([](auto&& algo) { algo->reset(true); }, g_AlgorithmVariant);
}

//-----------------------------------------------------------------------------
// Name: keyboardChanged()
// Desc:
//-----------------------------------------------------------------------------
void keyboardChanged(int key, int action, int mods) {}

void windowResized(const glm::ivec2& resolution) {
    spdlog::trace("Window resized");
    std::visit(
      [&resolution](auto&& algo) { algo->onWindowResized(resolution); },
      g_AlgorithmVariant);
}

void destroyAlgorithm() {
    spdlog::trace("Destroying algorithm");
    std::visit([](auto&& algo) { algo.reset(nullptr); }, g_AlgorithmVariant);
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
