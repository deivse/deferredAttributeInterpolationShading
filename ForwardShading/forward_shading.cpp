//-----------------------------------------------------------------------------
//  [PGR2] Forward Shading
//  12/01/2023
//-----------------------------------------------------------------------------
//  Controls:
//    [i/I]   ... inc/dec value of user integer variable
//    [f/F]   ... inc/dec value of user floating-point variable
//    [z/Z]   ... move scene along z-axis
//    [a]     ... enable/disable animation of lights
//    [v]     ... show/hide lights
//    [k]     ... show/hide lights' ranges
//    [e]     ... make explicit synchronization before each performance
//    measurement [s/S]   ... inc/dec number of spheres per row [d/D]   ...
//    inc/dec number of sphare slices [l/L]   ... inc/dec number of lights [r]
//    ... reset all algorithms [c]     ... compile shaders [mouse] ... scene
//    rotation (left button)
//-----------------------------------------------------------------------------

#include <common.h>
#include <tools.h>
#include <algorithm>
#include <map>
#include "glbinding-aux/Meta.h"
#include <spdlog/spdlog.h>

#define RENDERPASS_CALLBACK(callback) \
    std::bind(&callback, this, std::placeholders::_1)

struct Algorithm;

using OptionsMap = std::map<std::string, bool>;
using VSInvocationCounter = Tools::GPUVSInvocationQuery;
using FSInvocationCounter = Tools::GPUFSInvocationQuery;

struct RenderPass
{
    using renderPassFn
      = std::function<void(void)>; // Render function declaration

    std::string name;               // Renderpass name
    const bool* enabled = nullptr;  // Renderpass controller (optional)
    std::string shaderFilenameBase; // Shader file name (without file extensions
                                    // 'vert', 'frag' or 'geom')
    GLuint program = 0;             // Shader program ID
    renderPassFn render;            // Render function
    Tools::GPUTimer timer;          // GPU timer
    // VSInvocationCounter
    //   vertexShaderCounter; // GPU vertex shader execution counter
    // FSInvocationCounter
    //   fragmentShaderCounter; // GPU fragment shader execution counter

    // Constructors
    RenderPass(const std::string& _name, const char* _shaderFile,
               renderPassFn _render)
      : name(_name), enabled(nullptr),
        shaderFilenameBase(_shaderFile ? _shaderFile : ""), render(_render) {}
    RenderPass(const std::string& _name, renderPassFn _render)
      : name(_name), enabled(nullptr), shaderFilenameBase(""), render(_render) {
    }
    RenderPass(const std::string& _name, const char* _shaderFile,
               const bool& _controller, renderPassFn _render)
      : name(_name), enabled(&_controller),
        shaderFilenameBase(_shaderFile ? _shaderFile : ""), render(_render) {}
    RenderPass(const std::string& _name, const bool& _controller,
               renderPassFn _render)
      : name(_name), enabled(&_controller), shaderFilenameBase(""),
        render(_render) {}

    // Starts render pass including lazy initialization and performance
    // measurements
    void run(Algorithm& algorithm, bool forceSync = false) {
        if (enabled && (enabled[0] == false)) return;

        // vertexShaderCounter.start();
        // fragmentShaderCounter.start();
        timer.start(forceSync);
        glUseProgram(program);
        render();
        timer.stop();
        // fragmentShaderCounter.stop();
        // vertexShaderCounter.stop();
    }

    void resetTimer() { timer.reset(); }

    bool compile(const OptionsMap& options) {
        resetTimer();
        if (shaderFilenameBase.empty()) return true;

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
        bool result = Tools::Shader::CreateShaderProgramFromFile(
          program, (shaderFilenameBase + ".vert").c_str(), nullptr, nullptr,
          (shaderFilenameBase + ".geom").c_str(),
          (shaderFilenameBase + ".frag").c_str(),
          preprocessor.empty() ? nullptr : preprocessor.c_str());
        // If compilation failed, try VS+FS only.
        if (!result)
            result = Tools::Shader::CreateShaderProgramFromFile(
              program, (shaderFilenameBase + ".vert").c_str(), nullptr, nullptr,
              nullptr, (shaderFilenameBase + ".frag").c_str(),
              preprocessor.empty() ? nullptr : preprocessor.c_str());

        // Explicit sync. for better time measurement
        glFinish();
        return result;
    }

    bool isEnabled() const { return !enabled || (enabled[0]); }
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
    Algorithm(const std::string& _name) : name(_name) {}

    // Initializes the algorithm
    virtual void setup(){
      /*
      // Add options
          options["WireModel"] = true;

      // Add rendering passes
          renderPasses.emplace_back("lighting pass", "lighting", [&]() -> void
      { glClear(GL_COLOR_BUFFER_BIT); if (options["WireModel"])
                  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
              glDrawArrays(GL_TRIANGLES, 0, 3);
          });
          renderPasses.emplace_back("read pass", nullptr,
      options["ReadColors"], [&]() -> void { GLubyte pixels[100 * 100 * 4] =
      {}; glReadPixels(0, 0, 100, 100, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
          });
      */
    };

    // Displays algorithm debug informations. This method is called at the end
    // of Algorithm::run().
    virtual void debug() {}

    // Window resize callback
    virtual void windowResized(const glm::ivec2& resolution){};

    // Executes the algorithm
    virtual void run(bool forceSync = false) {
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
    virtual bool reset(bool resetShaders) {
        timer.reset();
        bool result = true;
        if (resetShaders) {
            return compile();
        }
        for (auto& renderPass : renderPasses) renderPass.resetTimer();
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

        const int rowHeight = 9 * IMGUI_RESIZE_FACTOR;
        const size_t height
          = 160 + rowHeight * (3 * numRenderPasses + options.size());
        ImGui::Begin(name.c_str());
        ImGui::SetWindowPos(glm::vec2(position.x, position.y)
                              * IMGUI_RESIZE_FACTOR,
                            ImGuiCond_Once);
        ImGui::SetWindowSize(glm::vec2(width, height) * IMGUI_RESIZE_FACTOR,
                             ImGuiCond_Always);
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
        // // Invocation counters
        // ImGui::Separator();
        // ImGui::Text("VERTEX SHADERS");
        // for (auto& renderPass : renderPasses) {
        //     if (renderPass.isEnabled())
        //         ImGui::Text(" * %s: %u", renderPass.name.c_str(),
        //                     renderPass.vertexShaderCounter.get());
        // }
        // ImGui::Separator();
        // ImGui::Text("FRAGMENT SHADERS");
        // for (auto& renderPass : renderPasses) {
        //     if (renderPass.isEnabled())
        //         ImGui::Text(" * %s: %u", renderPass.name.c_str(),
        //                     renderPass.fragmentShaderCounter.get());
        // }
        ImGui::End();
    }

}; // end of struct Algorithm

#include <array>

// GLOBAL CONSTANTS____________________________________________________________
enum eAlgorithm
{
    ForwardShading = 0,
    DeferredShading,
    NumAlgorithms
};
const int MaxLights = 2048; // Maximum number of lights supported

struct Light
{
    glm::vec4 position; // (x, y, z, radius)
    glm::vec4 color;    // (diffuse.rgb, specular)
};

// GLOBAL VARIABLES____________________________________________________________
int g_Algorithm = DeferredShading; // Current shading algorithm
bool g_RotateLights = true;        // Rotate lights
bool g_ShowLightCenters = true;    // Render lights
bool g_ShowLightRange = false;     // Render lights' ranges
bool g_ExplicitTimerSync = false;  // Explicit synchronization will be made
                                   // before any performance measurement
// Scene
glm::vec4 g_CameraPosition; // Camera (view) position in world space
int g_NumSpheresPerRow = 5; // Number of spheres per row/column
int g_NumSphereSlices = 20; // Number of sphere slices
// Lights
std::vector<Light> g_Lights; // Lights
GLuint g_LightBuffer = 0;    // Uniform buffer with lights
GLuint g_LightsVertexArray
  = 0;                  // Vertex array with light sphere (attenuation) geometry
int g_NumLights = 1000; // Number of lights currently used in the scene
float g_LightSpeed = 0.01;                // Speed of light moving (rotation)
glm::vec2 g_LightRangeLimits(0.2f, 2.0f); // Light range limits

// FORWARD DECLARATIONS________________________________________________________
void createLights();
void updateLights();
void renderLights();
void renderLightRanges(bool bindShader = true);
void renderScene();

// IMPLEMENTATION______________________________________________________________

// Forward-Shading algorithm implementation
struct ForwardShadingAlgorithm : Algorithm
{
    ForwardShadingAlgorithm() : Algorithm("ForwardShading") {}

    void setup() override {
        renderPasses.emplace_back("shading", "forward_shading", []() -> void {
            glUniform3fv(1, 1,
                         &g_CameraPosition.x); // Set camera world's position
            glUniform1ui(2, g_NumLights);      // Set number of lights
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderScene();
        });
    }
};
ForwardShadingAlgorithm g_ForwardShading;

// Deferred Shading algorithm implementation
struct DeferredShadingAlgorithm : Algorithm
{
    GLuint gbuffer; // Main framebuffer with same size as current window
    std::array<GLuint, 4> gbufferTexture; // Auxiliary textures for g-buffer

    DeferredShadingAlgorithm() : Algorithm("DeferredShading") {}

    void setup() override {
        showDebug = true;
        renderPasses.emplace_back("shading", "forward_shading", [&]() -> void {
            glUniform3fv(1, 1,
                         &g_CameraPosition.x); // Set camera world's position
            glUniform1ui(2, g_NumLights);      // Set number of lights
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderScene();
        });

        windowResized(Variables::WindowSize);
    }

    void windowResized(const glm::ivec2& resolution) override {
        // TODO: Create G-Buffer
        glDeleteFramebuffers(1, &gbuffer);
        glDeleteTextures(gbufferTexture.size(), gbufferTexture.data());
    }

    void debug() override {
        const int width = 400;
        const float height
          = width / float(Variables::WindowSize.x) * Variables::WindowSize.y;
        int i = 0;
        for (; i < gbufferTexture.size(); i++)
            Tools::Texture::Show2D(
              gbufferTexture[i], Variables::WindowSize.x - width,
              Variables::WindowSize.y - height * (i + 1), width, height);
    }
};
DeferredShadingAlgorithm g_DeferredShading;

//-----------------------------------------------------------------------------
// Name: display()
// Desc:
//-----------------------------------------------------------------------------
void display() {
    // Update lights (rotate)
    updateLights();
    // Update camera world's position
    g_CameraPosition = Variables::Transform.ModelViewInverse
                       * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (g_Algorithm == ForwardShading)
        g_ForwardShading.run(g_ExplicitTimerSync);
    else
        g_DeferredShading.run(g_ExplicitTimerSync);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (g_ShowLightCenters) renderLights();
    if (g_ShowLightRange) renderLightRanges();
}

//-----------------------------------------------------------------------------
// Name: init()
// Desc:
//-----------------------------------------------------------------------------
void init() {
    GLint n = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    for (GLint i = 0; i < n; i++) {
        spdlog::info(
          "Supported extension {}",
          reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i)));
    }

    // spdlog::info("Pipeline stat query status: {}", glbinding::aux::Meta::;

    // Default scene distance
    Variables::Transform.SceneZOffset = 8.0f;

    // Set OpenGL state variables
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DITHER);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glPointSize(10.0f);
    glEnable(GL_POINT_SMOOTH);

    // Setup algorithms
    g_ForwardShading.setup();
    g_DeferredShading.setup();

    // Create random lights
    createLights();
}

// Include GUI and control stuff
//-----------------------------------------------------------------------------
//  [PGR2] Forward Shading
//  12/01/2023
//-----------------------------------------------------------------------------
const char* help_message = "[PGR2] Forward Shading\n\
-------------------------------------------------------------------------------\n\
CONTROLS:\n\
   [i/I]   ... inc/dec value of user integer variable\n\
   [f/F]   ... inc/dec value of user floating-point variable\n\
   [z/Z]   ... move scene along z-axis\n\
   [a]     ... enable/disable animation of lights\n\
   [v]     ... show/hide lights\n\
   [k]     ... show/hide lights' ranges\n\
   [e]     ... make explicit synchronization before each performance measurement\n\
   [s/S]   ... inc/dec number of spheres per row\n\
   [d/D]   ... inc/dec number of sphare slices\n\
   [l/L]   ... inc/dec number of lights\n\
   [r]     ... reset all algorithms\n\
   [c]     ... compile shaders\n\
   [mouse] ... scene rotation (left button)\n\
-------------------------------------------------------------------------------";

// IMPLEMENTATION______________________________________________________________

//-----------------------------------------------------------------------------
// Name: createLights()
// Desc:
//-----------------------------------------------------------------------------
void createLights() {
    // Create random light
    g_Lights.resize(MaxLights);
    for (int i = 0; i < MaxLights; i++) {
        const float radius
          = glm::linearRand(1.0f, static_cast<float>(g_NumSpheresPerRow));
        const glm::vec3 position
          = glm::normalize(glm::linearRand(glm::vec3(-1.0f), glm::vec3(1.0f)))
            * radius;
        const float range
          = glm::linearRand(g_LightRangeLimits.x, g_LightRangeLimits.y);

        g_Lights[i].position = glm::vec4(position, range);
        g_Lights[i].color
          = glm::vec4(glm::linearRand(glm::vec3(0.0f), glm::vec3(0.5f)), 0.10f);
    }

    // Create buffer with lights and bind it as GL_UNIFORM_BUFFER to 0 binding
    // point
    glDeleteBuffers(1, &g_LightBuffer);
    glCreateBuffers(1, &g_LightBuffer);
    glNamedBufferStorage(g_LightBuffer, sizeof(Light) * g_Lights.size(),
                         g_Lights.data(), GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_LightBuffer);

    // Create vertex array to display all lights
    glDeleteVertexArrays(1, &g_LightsVertexArray);
    glCreateVertexArrays(1, &g_LightsVertexArray);
    glVertexArrayVertexBuffer(g_LightsVertexArray, 0, g_LightBuffer, 0,
                              2 * sizeof(glm::vec4));
    glVertexArrayAttribBinding(g_LightsVertexArray, 0, 0);
    glVertexArrayAttribFormat(g_LightsVertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib(g_LightsVertexArray, 0);
}

//-----------------------------------------------------------------------------
// Name: renderLights()
// Desc:
//-----------------------------------------------------------------------------
void renderLights() {
    static GLuint program = 0;
    static GLuint vertexArray = 0;
    if (program == 0) {
        Tools::Shader::CreateShaderProgramFromFile(program, "light_center.vert",
                                                   nullptr, nullptr, nullptr,
                                                   "light.frag");
        glCreateVertexArrays(1, &vertexArray);
    }
    glUseProgram(program);
    glBindVertexArray(g_LightsVertexArray);
    glDrawArrays(GL_POINTS, 0, g_NumLights);
}

//-----------------------------------------------------------------------------
// Name: createSphereGeometry()
// Desc:
//-----------------------------------------------------------------------------
std::vector<glm::vec3> createSphereGeometry(float radius, int slices) {
    std::vector<glm::vec3> vertices;
    Tools::Mesh::CreateSphereVertexMesh(vertices, radius, slices, slices);
    for (int i = 1; i < vertices.size(); i += 3)
        std::swap(vertices[i], vertices[i + 1]); // TODO: fix CW order bug
    return vertices;
}

//-----------------------------------------------------------------------------
// Name: renderLightRanges()
// Desc:
//-----------------------------------------------------------------------------
void renderLightRanges(bool bindShader) {
    static GLuint program = 0;
    static GLuint vertexArray = 0;
    static GLsizei numVertices = 0;
    if (program == 0) {
        Tools::Shader::CreateShaderProgramFromFile(
          program, "light_range.vert", nullptr, nullptr, nullptr, "light.frag");
        // Create buffer with sphere geometry
        std::vector<glm::vec3> vertices = createSphereGeometry(0.5f, 20);
        numVertices = static_cast<GLsizei>(vertices.size());
        GLuint vertexBuffer = 0;
        glCreateBuffers(1, &vertexBuffer);
        glNamedBufferStorage(vertexBuffer, vertices.size() * sizeof(glm::vec3),
                             &vertices[0].x,
                             gl::BufferStorageMask::GL_NONE_BIT);

        // Create vertex array for lights' spheres
        glCreateVertexArrays(1, &vertexArray);
        glVertexArrayVertexBuffer(vertexArray, 0, vertexBuffer, 0,
                                  sizeof(glm::vec3));
        glVertexArrayAttribBinding(vertexArray, 0, 0);
        glVertexArrayAttribFormat(vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(vertexArray, 0);
    }

    if (bindShader) {
        glUseProgram(program);
        glBindVertexArray(vertexArray);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArraysInstanced(GL_TRIANGLES, 0, numVertices, g_NumLights);
        glDisable(GL_BLEND);
    } else {
        glBindVertexArray(vertexArray);
        glDrawArraysInstanced(GL_TRIANGLES, 0, numVertices, g_NumLights);
    }
}

//-----------------------------------------------------------------------------
// Name: updateLights()
// Desc:
//-----------------------------------------------------------------------------
void updateLights() {
    if (!g_RotateLights) return;

    const float cosAngle = glm::cos(g_LightSpeed);
    const float sinAngle = glm::sin(g_LightSpeed);
    for (auto& light : g_Lights) {
        light.position.x
          = light.position.x * cosAngle + light.position.z * sinAngle;
        light.position.z
          = -light.position.x * sinAngle + light.position.z * cosAngle;
    }
    // Update light buffer
    glNamedBufferSubData(g_LightBuffer, 0, sizeof(Light) * g_Lights.size(),
                         g_Lights.data());
}

//-----------------------------------------------------------------------------
// Name: updateLightRadius()
// Desc:
//-----------------------------------------------------------------------------
void updateLightRadius() {
    for (auto& light : g_Lights) {
        light.position.w
          = glm::linearRand(g_LightRangeLimits.x, g_LightRangeLimits.y);
    }
    // Update light buffer
    glNamedBufferSubData(g_LightBuffer, 0, sizeof(Light) * g_Lights.size(),
                         g_Lights.data());
}

//-----------------------------------------------------------------------------
// Name: renderScene()
// Desc:
//-----------------------------------------------------------------------------
void renderScene() {
    static GLuint vertexArray = 0;
    static GLuint vertexBuffer = 0;
    static GLuint attribBuffer = 0;
    static GLsizei numSlices = 0;
    static std::vector<glm::vec3> spherePositions;
    static std::vector<glm::vec3> sphereVertices;

    // Create texture for scene
    static GLuint texture
      = Tools::Texture::CreateFromFile("../common/textures/metal01.jpg");

    // Calculate sphere positions
    const size_t numSpheres
      = g_NumSpheresPerRow * g_NumSpheresPerRow * g_NumSpheresPerRow;
    if (numSpheres != spherePositions.size()) {
        spherePositions.clear();
        for (int i = 0; i < numSpheres; i++) {
            const int x = i % g_NumSpheresPerRow;
            const int y = i / g_NumSpheresPerRow % g_NumSpheresPerRow;
            const int z = i / (g_NumSpheresPerRow * g_NumSpheresPerRow)
                          % g_NumSpheresPerRow;
            spherePositions.push_back(
              glm::vec3(x, y, z) - glm::vec3((g_NumSpheresPerRow - 1) * 0.5f));
        }

        // glDeleteBuffers(1, &attribBuffer);
        // glCreateBuffers(1, &attribBuffer);
        // glNamedBufferStorage(attribBuffer, spherePositions.size() *
        // sizeof(glm::vec3), &spherePositions[0].x, GL_NONE);
    }

    if ((vertexArray == 0) || (numSlices != g_NumSphereSlices)) {
        numSlices = g_NumSphereSlices;
        glDeleteVertexArrays(1, &vertexArray);
        glDeleteBuffers(1, &vertexBuffer);

        // Create vertex buffer
        sphereVertices = createSphereGeometry(0.5f, g_NumSphereSlices);
        glCreateBuffers(1, &vertexBuffer);
        glNamedBufferStorage(
          vertexBuffer, sphereVertices.size() * sizeof(glm::vec3),
          &sphereVertices[0].x, gl::BufferStorageMask::GL_NONE_BIT);
        // Create vertex array
        glCreateVertexArrays(1, &vertexArray);
        glVertexArrayVertexBuffer(vertexArray, 0, vertexBuffer, 0,
                                  sizeof(glm::vec3));
        glVertexArrayAttribBinding(vertexArray, 0, 0);
        glVertexArrayAttribFormat(vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(vertexArray, 0);
    }

    glBindTextureUnit(0, texture);
    glUniform1i(0, g_NumSpheresPerRow);
    glBindVertexArray(vertexArray);
    // TODO: Use instance rendering
    for (auto& position : spherePositions) {
        glUniform3fv(3, 1, &position.x);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<GLsizei>(sphereVertices.size()));
    }
    glBindVertexArray(0);
}

//-----------------------------------------------------------------------------
// Name: resetAlgorithms()
// Desc:
//-----------------------------------------------------------------------------
void resetAlgorithms(bool resetShaders = false) {
    g_ForwardShading.reset(resetShaders);
    g_DeferredShading.reset(resetShaders);
}

//-----------------------------------------------------------------------------
// Name: compileShaders()
// Desc:
//-----------------------------------------------------------------------------
void compileShaders(void* clientData) { resetAlgorithms(true); }

//-----------------------------------------------------------------------------
// Name: windowResized()
// Desc: Called anytime the window size has changed
//-----------------------------------------------------------------------------
void windowResized(const glm::ivec2& resolution) {
    g_ForwardShading.windowResized(resolution);
    g_DeferredShading.windowResized(resolution);
}

//-----------------------------------------------------------------------------
// Name: showGUI()
// Desc:
//-----------------------------------------------------------------------------
int showGUI() {
    int menuHeight = 305;
    const int oneInt = 1;
    const int itemWidth = 140 * IMGUI_RESIZE_FACTOR;

    ImGui::Begin("Render");
    ImGui::SetWindowSize(glm::vec2(220, menuHeight) * IMGUI_RESIZE_FACTOR,
                         ImGuiCond_Always);
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::Combo("shading", &g_Algorithm, "Forward\0Deferred\0"))
        resetAlgorithms();
    ImGui::Checkbox("rotate lights", &g_RotateLights);
    ImGui::Checkbox("show lights", &g_ShowLightCenters);
    ImGui::Checkbox("show light range", &g_ShowLightRange);
    if (ImGui::Checkbox("synchronize timers", &g_ExplicitTimerSync)) {
        resetAlgorithms();
    }
    ImGui::SameLine(170.0f * IMGUI_RESIZE_FACTOR, -10.0f * IMGUI_RESIZE_FACTOR);
    if (ImGui::Button("reset")) resetAlgorithms();

    ImGui::Separator();
    ImGui::Text("SPHERES");
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::InputScalar("per row", ImGuiDataType_S32, &g_NumSpheresPerRow,
                           &oneInt)) {
        g_NumSpheresPerRow = glm::clamp(g_NumSpheresPerRow, 1, 100);
        createLights();
        resetAlgorithms();
    }
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::InputScalar("slices", ImGuiDataType_S32, &g_NumSphereSlices,
                           &oneInt)) {
        g_NumSphereSlices = glm::clamp(g_NumSphereSlices, 5, 100);
        resetAlgorithms();
    }

    ImGui::Separator();
    ImGui::Text("LIGHTS");
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::InputScalar("count", ImGuiDataType_S32, &g_NumLights, &oneInt)) {
        g_NumLights = glm::clamp(g_NumLights, 1, MaxLights);
        resetAlgorithms();
    }
    ImGui::SetNextItemWidth(itemWidth);
    ImGui::SliderFloat("speed", &g_LightSpeed, 0.0f, 0.2f);
    ImGui::SetNextItemWidth(56 * IMGUI_RESIZE_FACTOR);
    if (ImGui::SliderFloat("##x", &g_LightRangeLimits.x, 0.01f,
                           g_LightRangeLimits.y, "%.2f")) {
        updateLightRadius();
        resetAlgorithms();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(56 * IMGUI_RESIZE_FACTOR);
    if (ImGui::SliderFloat("##y", &g_LightRangeLimits.y, g_LightRangeLimits.x,
                           8.0f, "%.2f")) {
        updateLightRadius();
        resetAlgorithms();
    }
    ImGui::SameLine();
    ImGui::Text("range");
    ImGui::End();

    if (g_Algorithm == ForwardShading)
        g_ForwardShading.gui(glm::ivec2(235, 10), 240);
    else
        g_DeferredShading.gui(glm::ivec2(235, 10), 240);

    return menuHeight;
}

//-----------------------------------------------------------------------------
// Name: keyboardChanged()
// Desc:
//-----------------------------------------------------------------------------
void keyboardChanged(int key, int action, int mods) {
    switch (key) {
        case GLFW_KEY_R:
            resetAlgorithms();
            break;
        case GLFW_KEY_A:
            g_RotateLights = !g_RotateLights;
            break;
        case GLFW_KEY_V:
            g_ShowLightCenters = !g_ShowLightCenters;
            break;
        case GLFW_KEY_K:
            g_ShowLightRange = !g_ShowLightRange;
            break;
        case GLFW_KEY_E:
            g_ExplicitTimerSync = !g_ExplicitTimerSync;
            break;
        case GLFW_KEY_S:
            g_NumSpheresPerRow += glm::clamp(
              g_NumSpheresPerRow + ((mods == GLFW_MOD_SHIFT) ? -1 : 1), 1, 100);
            break;
        case GLFW_KEY_D:
            g_NumSphereSlices += glm::clamp(
              g_NumSphereSlices + ((mods == GLFW_MOD_SHIFT) ? -1 : 1), 5, 100);
            break;
        case GLFW_KEY_L:
            g_NumLights += glm::clamp(
              g_NumLights + ((mods == GLFW_MOD_SHIFT) ? -1 : 1), 1, MaxLights);
            break;
            // g_LightSpeed
            // g_LightRangeLimits
    }
}

//-----------------------------------------------------------------------------
// Name: main()
// Desc:
//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    int OGL_CONFIGURATION[]
      = {GLFW_CONTEXT_VERSION_MAJOR,
         4,
         GLFW_CONTEXT_VERSION_MINOR,
         6,
         GLFW_OPENGL_FORWARD_COMPAT,
         GL_FALSE.m_value,
         GLFW_OPENGL_DEBUG_CONTEXT,
         GL_TRUE.m_value,
         GLFW_OPENGL_PROFILE,
         GLFW_OPENGL_COMPAT_PROFILE, // GLFW_OPENGL_CORE_PROFILE
         PGR2_SHOW_MEMORY_STATISTICS,
         GL_TRUE.m_value,
         0};

    printf("%s\n", help_message);

    return common_main(1200, 900, "[PGR2] Forward Shading",
                       OGL_CONFIGURATION, // OGL configuration hints
                       init,              // Init GL callback function
                       nullptr,           // Release GL callback function
                       showGUI,           // Show GUI callback function
                       display,           // Display callback function
                       windowResized,     // Window resize callback function
                       keyboardChanged,   // Keyboard callback function
                       nullptr,           // Mouse button callback function
                       nullptr);          // Mouse motion callback function
}
