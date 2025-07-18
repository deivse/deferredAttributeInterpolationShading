#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHM
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHM

#include "tools.h"
#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <filesystem>

#include <fmt/format.h>

#include <glbinding/gl/gl.h>
#include <spdlog/spdlog.h>

using namespace gl;

using OptionsMap = std::unordered_map<std::string, bool>;

template<typename DerivedT>
class Algorithm;

struct RenderPass
{
    using RenderPassCb = std::function<void(void)>;

    std::string name;                    // Renderpass name
    const bool* controller = nullptr;    // Renderpass controller (optional)
    std::string_view shaderFilenameBase; // Shader file name (without file
                                         // extensions 'vert', 'frag' or 'geom')
    GLuint program = 0;                  // Shader program ID
    RenderPassCb render;                 // Render function
    Tools::GPUTimer timer;               // GPU timer
    Tools::GPUVSInvocationQuery
      vertexShaderCounter; // GPU vertex shader execution counter
    Tools::GPUFSInvocationQuery
      fragmentShaderCounter; // GPU fragment shader execution counter

    // Constructors
    RenderPass(std::string name_, RenderPassCb renderFunc_,

               std::optional<std::string_view> shaderFilenameBase_
               = std::nullopt,
               const bool* controller_ = nullptr)
      : name(std::move(name_)), controller(controller_),
        shaderFilenameBase(shaderFilenameBase_.value_or("")),
        render(std::move(renderFunc_)) {}

    RenderPass(std::string name_, RenderPassCb renderFunc_,
               const bool* controller_ = nullptr)
      : RenderPass(std::move(name_), std::move(renderFunc_), std::nullopt,
                   controller_) {}

    // Starts render pass including lazy initialization and performance
    // measurements
    void run(bool forceSync = false) {
        if (controller != nullptr && !*controller) return;

        vertexShaderCounter.start();
        fragmentShaderCounter.start();
        timer.start(forceSync);

        glUseProgram(program);
        render();

        timer.stop();
        fragmentShaderCounter.stop();
        vertexShaderCounter.stop();
    }

    void resetTimer() { timer.reset(); }

    bool compileShaders(const OptionsMap& options) {
        resetTimer();
        if (shaderFilenameBase.empty()) return true;

        // Create list of GLSL defines for enabled options
        std::string preprocessorDefines;
        for (auto&& [name, enabled] : options) {
            if (enabled) {
                std::string defineName = name;
                std::replace(defineName.begin(), defineName.end(), ' ', '_');
                preprocessorDefines += fmt::format("#define {}\n", defineName);
            }
        }

        const auto makeFilename = [&](std::string_view extension) {
            return fmt::format("{}.{}", shaderFilenameBase, extension);
        };
        const auto getDefinesOrNullptr = [&]() {
            return preprocessorDefines.empty() ? nullptr
                                               : preprocessorDefines.c_str();
        };

        bool compilationResult{};
        if (std::filesystem::exists(makeFilename("comp"))) {
            compilationResult
              = Tools::Shader::CreateComputeShaderProgramFromFile(
                program, makeFilename("comp").c_str(), getDefinesOrNullptr());
        } else if (std::filesystem::exists(makeFilename("geom"))) {
            compilationResult = Tools::Shader::CreateShaderProgramFromFile(
              program, makeFilename("vert").c_str(), nullptr, nullptr,
              makeFilename("geom").c_str(), makeFilename("frag").c_str(),
              getDefinesOrNullptr());
        } else {
            compilationResult = Tools::Shader::CreateShaderProgramFromFile(
              program, makeFilename("vert").c_str(), nullptr, nullptr, nullptr,
              makeFilename("frag").c_str(), getDefinesOrNullptr());
        }

        // Explicit sync. for better time measurement
        glFinish();
        return compilationResult;
    }

    bool isEnabled() const { return !controller || (controller[0]); }
}; // end of struct RenderPass

template<typename DerivedT>
class Algorithm
{
    bool initialized = false; // Flag for lazy initialization of shaders
    bool showDebug = false;   // Flag if debug method will be called
    Tools::GPUTimer timer;    // GPU timer for the complete algorithm

    DerivedT& getDerived() { return reinterpret_cast<DerivedT&>(*this); }
    const DerivedT& getDerived() const {
        return reinterpret_cast<const DerivedT&>(*this);
    }
    const std::string& getName() const { return getDerived().name; }
    std::vector<RenderPass>& getRenderPasses() {
        return getDerived().renderPasses;
    }
    OptionsMap& getOptions() { return getDerived().options; }

protected:
    using AlgorithmCRTPBaseT = Algorithm<DerivedT>;

    template<typename... Args>
    void log(spdlog::level::level_enum level,
             spdlog::format_string_t<Args...> fmt, Args&&... args) {
        spdlog::log(level, "{}: {}", getDerived().name,
                    fmt::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void logWarning(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        log(spdlog::level::warn, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void logError(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        log(spdlog::level::err, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void logInfo(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        log(spdlog::level::info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void logDebug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        log(spdlog::level::debug, fmt, std::forward<Args>(args)...);
    }

public:
    // Displays algorithm debug informations. This method is called at the end
    // of Algorithm::run().
    void debug() { getDerived().debug(); }

    // Window resize callback
    void onWindowResized(const glm::ivec2& resolution) {
        getDerived().windowResized(resolution);
    };

    // Executes the algorithm
    void run(bool forceSync = false) {
        // Lazy initialization of the algorithm
        if (!initialized) initialized = reset(true);

        timer.start(forceSync);
        for (auto& renderPass : getRenderPasses()) {
            log(spdlog::level::trace, "Starting {} render pass",
                renderPass.name);
            renderPass.run(forceSync);
        }
        timer.stop();

        // Render/print debug information
        if (showDebug) debug();
    }

    // Resets algorithm - resets all performance timers/counters and restarts
    // all renderpasses (and optionally rebuilds shaders of all renderpasses).
    bool reset(bool resetShaders = false) {
        timer.reset();
        bool result = true;
        if (resetShaders) {
            result = compile();
        }
        for (auto& renderPass : getRenderPasses()) renderPass.resetTimer();
        return result;
    }

    // Rebuilds algorithm shaders (rebuild shaders of all renderpasses)
    bool compile() {
        for (auto& renderPass : getRenderPasses())
            if (!renderPass.compileShaders(getOptions())) return false;
        return true;
    }

    // Displays window with GUI for the algorithm, returns window height
    size_t gui(const glm::ivec2& position, int width) {
        // Count enabled renderpasses
        int numRenderPasses = 0;
        for (auto& renderPass : getRenderPasses()) {
            if (renderPass.isEnabled()) numRenderPasses++;
        }

        const int rowHeight = 9 * IMGUI_RESIZE_FACTOR;
        size_t height
          = 160 + rowHeight * (3 * numRenderPasses + getOptions().size());


        ImGui::Begin(getName().c_str());
        ImGui::SetWindowPos(glm::vec2(position.x, position.y)
                              * IMGUI_RESIZE_FACTOR,
                            ImGuiCond_Once);

        height += getDerived().customGui();
        // Add checkboxes for algorithm options
        ImGui::Text("OPTIONS");
        ImGui::Checkbox("show debug", &showDebug);
        for (auto& option : getOptions()) {
            if (ImGui::Checkbox(option.first.c_str(), &option.second))
                reset(true);
        }
        ImGui::Separator();
        // Timers
        ImGui::Text("TIMERS");
        const auto totalTime = timer.getAverage();
        ImGui::Text("Frame time: %u (%u)", totalTime, timer.getCounter());
        for (auto& renderPass : getRenderPasses()) {
            if (renderPass.isEnabled()) {
                const auto renderPassTime = renderPass.timer.getAverage();
                ImGui::Text(" * %s: %u (%.2f%%)", renderPass.name.c_str(),
                            renderPassTime,
                            float(renderPassTime) / totalTime * 100.0f);
            }
        }
        // Invocation counters
        ImGui::Separator();
        ImGui::Text("VERTEX SHADERS");
        for (auto& renderPass : getRenderPasses()) {
            if (renderPass.isEnabled())
                ImGui::Text(" * %s: %u", renderPass.name.c_str(),
                            renderPass.vertexShaderCounter.get());
        }
        ImGui::Separator();
        ImGui::Text("FRAGMENT SHADERS");
        for (auto& renderPass : getRenderPasses()) {
            if (renderPass.isEnabled())
                ImGui::Text(" * %s: %u", renderPass.name.c_str(),
                            renderPass.fragmentShaderCounter.get());
        }
        ImGui::End();
        return height;
    }
}; // end of struct Algorithm=

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHM */
