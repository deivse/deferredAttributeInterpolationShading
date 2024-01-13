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
#include "algorithm.hpp"
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
        renderPasses.emplace_back(
          "Forward Shading", "forward_shading", []() -> void {
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
    int tileSize = 32;                    // Tile size in pixels
    glm::ivec2 tileFramebufferSize;       // Tile framebuffer resolution
    GLuint tileFramebuffer = 0,
           tileDepthBuffer = 0; // Tile framebuffer and z-buffer
    GLuint lightIDBuffer = 0,
           lightCounterBuffer = 0; // Buffers for tile-based shading

    DeferredShadingAlgorithm() : Algorithm("DeferredShading") {}

    void setup() override {
        showDebug = true;
        options["Restore Depth"] = true;
        options["Tiled Shading"] = true;

        // Setup resources
        windowResized(Variables::WindowSize);

        renderPasses.emplace_back(
          "Clear Resources", nullptr, options["Tiled Shading"], [&]() -> void {
              GLuint zero = 0;
              glClearNamedBufferData(lightCounterBuffer, GL_R32UI,
                                     GL_RED_INTEGER, GL_UNSIGNED_INT,
                                     &glm::u32vec1(0)[0]);
              // TODO: Clear fbo
          });

        renderPasses.emplace_back(
          "Tiled PreDepth", "tiled_pre_depth", options["Tiled Shading"],
          [&]() -> void {
              glBindFramebuffer(GL_FRAMEBUFFER, tileFramebuffer);
              glViewport(0, 0, tileFramebufferSize.x, tileFramebufferSize.y);
              glClear(GL_DEPTH_BUFFER_BIT);
              renderScene();
              glViewport(0, 0, Variables::WindowSize.x,
                         Variables::WindowSize.y);
              glBindFramebuffer(GL_FRAMEBUFFER, 0);
          });

        renderPasses.emplace_back(
          "Light Count", "light_count", options["Tiled Shading"],
          [&]() -> void {
              glBindFramebuffer(GL_FRAMEBUFFER, tileFramebuffer);
              glViewport(0, 0, tileFramebufferSize.x, tileFramebufferSize.y);
              glDepthMask(GL_FALSE);
              glUniform1ui(0, tileFramebufferSize.x);
              renderLightRanges(false);
              glDepthMask(GL_TRUE);
              glViewport(0, 0, Variables::WindowSize.x,
                         Variables::WindowSize.y);
              glBindFramebuffer(GL_FRAMEBUFFER, 0);
          });

        renderPasses.emplace_back("G-Buffer", "gbuffer", [&]() -> void {
            glBindFramebuffer(GL_FRAMEBUFFER, gbuffer);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderScene();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        });

        renderPasses.emplace_back(
          "Deferred Shading", "deferred_shading", [&]() -> void {
              glBindFramebuffer(GL_FRAMEBUFFER, 0);
              glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
              glUniform3fv(1, 1,
                           &g_CameraPosition.x); // Set camera world's position
              glUniform1ui(2, g_NumLights);      // Set number of lights
              glDisable(GL_DEPTH_TEST);
              if (options["Tiled Shading"]) {
                  glUniform1ui(4, tileFramebufferSize.x);
                  glUniform1ui(5, tileSize);
              }
              // glBindVertexArray(emptyVAO);
              glBindTextures(1, 3, gbufferTexture.data());
              glDrawArrays(GL_TRIANGLES, 0, 3);
              glEnable(GL_DEPTH_TEST);
          });

        renderPasses.emplace_back(
          "Restore Z-Buffer", nullptr, options["Restore Depth"], [&]() -> void {
              glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer);
              glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
              glBlitFramebuffer(
                0, 0, Variables::WindowSize.x, Variables::WindowSize.y, 0, 0,
                Variables::WindowSize.x, Variables::WindowSize.y,
                GL_DEPTH_BUFFER_BIT, GL_NEAREST);
              glBindFramebuffer(GL_FRAMEBUFFER, 0);
          });
    }

    void windowResized(const glm::ivec2& resolution) override {
        // Create G-Buffer
        glDeleteFramebuffers(1, &gbuffer);
        glDeleteTextures(gbufferTexture.size(), gbufferTexture.data());
        Tools::Texture::Create2D(gbufferTexture[0], GL_RGBA8,
                                 resolution); // Color
        Tools::Texture::Create2D(gbufferTexture[1], GL_RGBA32F,
                                 resolution); // Normal
        Tools::Texture::Create2D(gbufferTexture[2], GL_RGBA32F,
                                 resolution); // Vertex
        Tools::Texture::Create2D(gbufferTexture[3], GL_DEPTH_COMPONENT32,
                                 resolution);
        glCreateFramebuffers(1, &gbuffer);
        glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT0,
                                  gbufferTexture[0], 0);
        glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT1,
                                  gbufferTexture[1], 0);
        glNamedFramebufferTexture(gbuffer, GL_COLOR_ATTACHMENT2,
                                  gbufferTexture[2], 0);
        glNamedFramebufferTexture(gbuffer, GL_DEPTH_ATTACHMENT,
                                  gbufferTexture[3], 0);

        GLenum buffers[3]
          = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
        glNamedFramebufferDrawBuffers(gbuffer, sizeof(buffers) / sizeof(GLenum),
                                      buffers);

        // Create tiled framebuffer
        tileFramebufferSize
          = glm::ivec2(resolution.x / tileSize, resolution.y / tileSize);
        Tools::Texture::Create2D(tileDepthBuffer, GL_DEPTH_COMPONENT24,
                                 tileFramebufferSize);
        glDeleteFramebuffers(1, &tileFramebuffer);
        glCreateFramebuffers(1, &tileFramebuffer);
        glNamedFramebufferTexture(tileFramebuffer, GL_DEPTH_ATTACHMENT,
                                  tileDepthBuffer, 0);

        // Create buffers for tiled-based shading
        int numTiles = tileFramebufferSize.x * tileFramebufferSize.y;
        Tools::Buffer::Create(lightCounterBuffer, numTiles * sizeof(GLuint),
                              GL_NONE, GL_SHADER_STORAGE_BUFFER, 0);
        Tools::Buffer::Create(lightIDBuffer,
                              numTiles * sizeof(GLuint) * MaxLights, GL_NONE,
                              GL_SHADER_STORAGE_BUFFER, 1);
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
        Tools::Texture::Show2D(tileDepthBuffer, Variables::WindowSize.x - width,
                               Variables::WindowSize.y - height * (i + 1),
                               width, height);
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
#include "controls.hpp"
