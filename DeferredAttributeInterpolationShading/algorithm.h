#ifndef DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHM
#define DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHM

#include "tools.h"
#include <algorithm>
#include <functional>
#include <optional>
#include <string>
#include <utility>

#include <fmt/format.h>

#include <glbinding/gl/gl.h>
using namespace gl;

using OptionsMap = std::unordered_map<std::string, bool>;

struct Algorithm;

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
    void run(Algorithm& algorithm, bool forceSync = false);

    void reset() { timer.reset(); }

    bool compileShaders(const OptionsMap& options);

    bool isEnabled() const { return !controller || (controller[0]); }
}; // end of struct RenderPass

struct Algorithm
{
    std::string name = "Algorithm"; // Algorithm name
    bool initialized = false;       // Flag for lazy initialization of shaders
    bool showDebug = false;         // Flag if debug method will be called
    Tools::GPUTimer timer;          // GPU timer for the complete algorithm
    std::vector<RenderPass> renderPasses; // Render passes of the algorithm
    OptionsMap options;                   // Algorithm options

    // Constructor
    explicit Algorithm(const std::string_view name_) : name(name_) {}

    // Initializes the algorithm
    virtual void setup() {
        // Add options
        options["WireModel"] = true;

        // Add rendering passes
        renderPasses.emplace_back(
          "lighting pass",
          [&]() -> void {
              glClear(GL_COLOR_BUFFER_BIT);
              if (options["WireModel"])
                  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
              glDrawArrays(GL_TRIANGLES, 0, 3);
          },
          "lighting");
        renderPasses.emplace_back(
          "read pass",
          [&]() -> void {
              GLubyte pixels[100 * 100 * 4] = {};
              glReadPixels(0, 0, 100, 100, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
          },
          &options["ReadColors"]);
    };

    // Displays algorithm debug informations. This method is called at the end
    // of Algorithm::run().
    virtual void debug() {}

    // Window resize callback
    virtual void windowResized(const glm::ivec2& resolution){};

    // Executes the algorithm
    virtual void run(bool forceSync = false);

    // Resets algorithm - resets all performance timers/counters and restarts
    // all renderpasses (and optionally rebuilds shaders of all renderpasses).
    virtual bool reset(bool resetShaders);

    // Rebuilds algorithm shaders (rebuild shaders of all renderpasses)
    virtual bool compile();

    // Displays window with GUI for the algorithm
    virtual void gui(const glm::ivec2& position, int width);

}; // end of struct Algorithm

#endif /* DEFERREDATTRIBUTEINTERPOLATIONSHADING_ALGORITHM */
