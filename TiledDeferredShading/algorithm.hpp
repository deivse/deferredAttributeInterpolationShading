#pragma once

#include "common.h"
#include <map>

#define RENDERPASS_CALLBACK(callback) std::bind(&callback, this, std::placeholders::_1)

struct Algorithm;

using OptionsMap = std::map<std::string, bool>;
using VSInvocationCounter = Tools::GPUVSInvocationQuery;
using FSInvocationCounter = Tools::GPUFSInvocationQuery;

struct RenderPass {
    using renderPassFn = std::function<void(void)>; // Render function declaration

    std::string          name;                  // Renderpass name
    const bool*          controller = nullptr;  // Renderpass controller (optional)
    std::string          shaderFile;            // Shader file name (without file extensions 'vert', 'frag' or 'geom')
    GLuint               program = 0;           // Shader program ID
    renderPassFn         render;                // Render function
    Tools::GPUTimer      timer;                 // GPU timer
    VSInvocationCounter  vertexShaderCounter;   // GPU vertex shader execution counter
    FSInvocationCounter  fragmentShaderCounter; // GPU fragment shader execution counter

    // Constructors
    RenderPass(const std::string& _name, const char* _shaderFile, renderPassFn _render)
        : name(_name), controller(nullptr), shaderFile(_shaderFile ? _shaderFile : ""), render(_render) {}
    RenderPass(const std::string& _name, renderPassFn _render)
        : name(_name), controller(nullptr), shaderFile(""), render(_render) {}
    RenderPass(const std::string& _name, const char* _shaderFile, const bool& _controller, renderPassFn _render)
        : name(_name), controller(&_controller), shaderFile(_shaderFile ? _shaderFile : ""), render(_render) {}
    RenderPass(const std::string& _name, const bool& _controller, renderPassFn _render)
        : name(_name), controller(&_controller), shaderFile(""), render(_render) {}

    // Starts render pass including lazy initialization and performance measurements
    void run(Algorithm& algorithm, bool forceSync = false) {
        if (controller && (controller[0] == false)) return;

        vertexShaderCounter.start();
        fragmentShaderCounter.start();
        timer.start(forceSync);
            glUseProgram(program);
            render();
        timer.stop();
        fragmentShaderCounter.stop();
        vertexShaderCounter.stop();
    }

    void reset() {
        timer.reset();
    }

    bool compile(const OptionsMap& options) {
        reset();
        if (shaderFile.empty())
            return true;

        // Create list of GLSL defines for enabled options
        std::string preprocessor;
        for (auto& option : options) {
            if (option.second) {
                std::string defineName = option.first;
                std::replace(defineName.begin(), defineName.end(), ' ', '_');
                preprocessor += "#define " + defineName + "\n";
            }
        }

        // Try to compile VS+GS+FS at first...
        bool result = Tools::Shader::CreateShaderProgramFromFile(program, (shaderFile + ".vert").c_str(), nullptr, nullptr, (shaderFile + ".geom").c_str(), (shaderFile + ".frag").c_str(), preprocessor.empty() ? nullptr : preprocessor.c_str());
        // If compilation failed, try VS+FS only.
        if (!result)
            result = Tools::Shader::CreateShaderProgramFromFile(program, (shaderFile + ".vert").c_str(), nullptr, nullptr, nullptr, (shaderFile + ".frag").c_str(), preprocessor.empty() ? nullptr : preprocessor.c_str());
    
        // Explicit sync. for better time measurement
        glFinish();
        return result;
    }

    bool isEnabled() const {
        return !controller || (controller[0]);
    }
}; // end of struct RenderPass

struct Algorithm {
    std::string             name = "Algorithm";  // Algorithm name
    bool                    initialized = false; // Flag for lazy initialization of shaders
    bool                    showDebug = false;   // Flag if debug method will be called
    Tools::GPUTimer         timer;               // GPU timer for the complete algorithm
    std::vector<RenderPass> renderPasses;        // Render passes of the algorithm
    OptionsMap              options;             // Algorithm options

    // Constructor
    Algorithm(const std::string& _name) : name(_name) {}

    // Initializes the algorithm
    virtual void setup() {
        /*
        // Add options
            options["WireModel"] = true;

        // Add rendering passes
            renderPasses.emplace_back("lighting pass", "lighting", [&]() -> void { 
                glClear(GL_COLOR_BUFFER_BIT);
                if (options["WireModel"])
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDrawArrays(GL_TRIANGLES, 0, 3);
            });
            renderPasses.emplace_back("read pass", nullptr, options["ReadColors"], [&]() -> void {
                GLubyte pixels[100 * 100 * 4] = {};
                glReadPixels(0, 0, 100, 100, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            });
        */
    };
    
    // Displays algorithm debug informations. This method is called at the end of Algorithm::run(). 
    virtual void debug() {}

    // Window resize callback
    virtual void windowResized(const glm::ivec2& resolution) {};

    // Executes the algorithm
    virtual void run(bool forceSync = false) {
        // Lazy initialization of the algorithm
        if (!initialized) initialized = reset(true);

        timer.start(forceSync);
        for (auto& renderPass : renderPasses)
            renderPass.run(*this, forceSync);
        timer.stop();

        // Render/print debug information
        if (showDebug) debug();
    }

    // Resets algorithm - resets all performance timers/counters and restarts all renderpasses (and optionally rebuilds shaders of all renderpasses).
    virtual bool reset(bool resetShaders) {
        timer.reset();
        bool result = true;
        if (resetShaders) {
            return compile();
        } 
        for (auto& renderPass : renderPasses)
            renderPass.reset();
        return true;
    }

    // Rebuilds algorithm shaders (rebuild shaders of all renderpasses)
    virtual bool compile() {
        for (auto& renderPass : renderPasses)
            if (!renderPass.compile(options)) return false;
        return true;
    }

    // Displays window with GUI for the algorithm
    virtual void gui(const glm::ivec2& position, int width) {
        // Count enabled renderpasses
        int numRenderPasses = 0;
        for (auto& renderPass : renderPasses) {
            if (renderPass.isEnabled()) numRenderPasses++;
        }

        int numControls = 3 * numRenderPasses + options.size();
        int const rowHeight = 10;
        int const rowSpace = 8;
        size_t const height = (170 + rowHeight * numControls + rowSpace * (numControls - 1));
        ImGui::Begin(name.c_str());
        ImGui::SetWindowPos(ImVec2(position.x, position.y) * IMGUI_RESIZE_FACTOR, ImGuiCond_Once);
        ImGui::SetWindowSize(ImVec2(width, height) * IMGUI_RESIZE_FACTOR, ImGuiCond_Always);
        // Add checkboxes for algorithm options
        ImGui::Text("OPTIONS");
        ImGui::Checkbox("show debug", &showDebug);
        for (auto& option : options) {
            if (ImGui::Checkbox(option.first.c_str(), &option.second))
                reset(true);
        }
        ImGui::Separator();
        // Timers
        ImGui::Text("TIMERS");
        auto const totalTime = timer.getAverage();
        ImGui::Text("Frame time: %u (%u)", totalTime, timer.getCounter());
        for (auto& renderPass : renderPasses) {
            if (renderPass.isEnabled()) {
                auto const renderPassTime = renderPass.timer.getAverage();
                ImGui::Text(" * %s: %u (%.2f%%)", renderPass.name.c_str(), renderPassTime, float(renderPassTime) / totalTime * 100.0f);
            }
        }
        // Invocation counters
        ImGui::Separator();
        ImGui::Text("VERTEX SHADERS");
        for (auto& renderPass : renderPasses) {
            if (renderPass.isEnabled())
                ImGui::Text(" * %s: %u", renderPass.name.c_str(), renderPass.vertexShaderCounter.get());
        }
        ImGui::Separator();
        ImGui::Text("FRAGMENT SHADERS");
        for (auto& renderPass : renderPasses) {
            if (renderPass.isEnabled())
                ImGui::Text(" * %s: %u", renderPass.name.c_str(), renderPass.fragmentShaderCounter.get());
        }
        ImGui::End();
    }

}; // end of struct Algorithm


