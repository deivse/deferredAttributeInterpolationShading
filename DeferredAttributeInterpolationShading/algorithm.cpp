#include "algorithm.h"

#include <filesystem>

void RenderPass::run(Algorithm& algorithm, bool forceSync) {
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

bool RenderPass::compileShaders(const OptionsMap& options) {
    reset();
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
    if (std::filesystem::exists(makeFilename(".geom"))) {
        compilationResult = Tools::Shader::CreateShaderProgramFromFile(
          program, makeFilename(".vert").c_str(), nullptr, nullptr,
          makeFilename(".geom").c_str(), makeFilename(".frag").c_str(),
          getDefinesOrNullptr());
    } else {
        compilationResult = Tools::Shader::CreateShaderProgramFromFile(
          program, makeFilename(".vert").c_str(), nullptr, nullptr, nullptr,
          makeFilename(".frag").c_str(), getDefinesOrNullptr());
    }

    // Explicit sync. for better time measurement
    glFinish();
    return compilationResult;
}

// Executes the algorithm
void Algorithm::run(bool forceSync) {
    // Lazy initialization of the algorithm
    if (!initialized) initialized = reset(true);

    timer.start(forceSync);
    for (auto& renderPass : renderPasses) renderPass.run(*this, forceSync);
    timer.stop();

    // Render/print debug information
    if (showDebug) debug();
}

// Resets algorithm - resets all performance timers/counters and restarts
// all renderpasses (and optionally rebuilds shaders of all renderpasses).
bool Algorithm::reset(bool resetShaders) {
    timer.reset();
    bool result = true;
    if (resetShaders) {
        return compile();
    }
    for (auto& renderPass : renderPasses) renderPass.reset();
    return true;
}

// Rebuilds algorithm shaders (rebuild shaders of all renderpasses)
bool Algorithm::compile() {
    for (auto& renderPass : renderPasses)
        if (!renderPass.compileShaders(options)) return false;
    return true;
}

// Displays window with GUI for the algorithm
void Algorithm::gui(const glm::ivec2& position, int width) {
    // Count enabled renderpasses
    int numRenderPasses = 0;
    for (auto& renderPass : renderPasses) {
        if (renderPass.isEnabled()) numRenderPasses++;
    }

    const int rowHeight = 9 * IMGUI_RESIZE_FACTOR;
    const size_t height
      = 160 + rowHeight * (3 * numRenderPasses + options.size());
    ImGui::Begin(name.c_str());
    ImGui::SetWindowPos(glm::vec2(position.x, position.y) * IMGUI_RESIZE_FACTOR,
                        ImGuiCond_Once);
    ImGui::SetWindowSize(glm::vec2(width, height) * IMGUI_RESIZE_FACTOR,
                         ImGuiCond_Always);
    // Add checkboxes for algorithm options
    ImGui::Text("OPTIONS");
    ImGui::Checkbox("show debug", &showDebug);
    for (auto& option : options) {
        if (ImGui::Checkbox(option.first.c_str(), &option.second)) reset(true);
    }
    ImGui::Separator();
    // Timers
    ImGui::Text("TIMERS");
    const auto totalTime = timer.getAverage();
    ImGui::Text("Frame time: %u (%u)", totalTime, timer.getCounter());
    for (auto& renderPass : renderPasses) {
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
    for (auto& renderPass : renderPasses) {
        if (renderPass.isEnabled())
            ImGui::Text(" * %s: %u", renderPass.name.c_str(),
                        renderPass.vertexShaderCounter.get());
    }
    ImGui::Separator();
    ImGui::Text("FRAGMENT SHADERS");
    for (auto& renderPass : renderPasses) {
        if (renderPass.isEnabled())
            ImGui::Text(" * %s: %u", renderPass.name.c_str(),
                        renderPass.fragmentShaderCounter.get());
    }
    ImGui::End();
}
