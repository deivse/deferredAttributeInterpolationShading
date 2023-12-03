//-----------------------------------------------------------------------------
//  [PGR2] Common function definitions
//  27/02/2008
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#ifndef _COMMON_INCLUDE_COMMON_H_
#define _COMMON_INCLUDE_COMMON_H_
#include <cassert>
#include <chrono>

#include <GL/glew.h>
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <GL/wglew.h>
#else
#include <GL/glxew.h>
#endif
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <implot_internal.h>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_precision.hpp>

#include "defines.h"
#include "globals.h"
#include "tools.h"

// INTERNAL USER CALLBACK FUNCTION POINTERS____________________________________
namespace Callbacks {
namespace User {
    TReleaseOpenGLCallback OpenGLRelease = nullptr;
    TDisplayCallback Display = nullptr;
    TShowGUICallback ShowGUI = nullptr;
    TWindowSizeChangedCallback WindowSizeChanged = nullptr;
    TMouseButtonChangedCallback MouseButtonChanged = nullptr;
    TMousePositionChangedCallback MousePositionChanged = nullptr;
    TKeyboardChangedCallback KeyboardChanged = nullptr;
} // end of namespace User

namespace GUI {
    //-----------------------------------------------------------------------------
    // Name: Set()
    // Desc: Default GUI "set" callback
    //-----------------------------------------------------------------------------
    template<typename T>
    void Set(const void* value, void* clientData) {
        *static_cast<T*>(clientData) = *static_cast<const T*>(value);
    }
    //-----------------------------------------------------------------------------
    // Name: Set()
    // Desc: Default GUI "set" callback
    //-----------------------------------------------------------------------------
    template<typename T>
    void SetCompile(const void* value, void* clientData) {
        *static_cast<T*>(clientData) = *static_cast<const T*>(value);
        compileShaders();
    }
    //-----------------------------------------------------------------------------
    // Name: Get()
    // Desc: Default GUI "get" callback
    //-----------------------------------------------------------------------------
    template<typename T>
    void Get(void* value, void* clientData) {
        *static_cast<T*>(value) = *static_cast<T*>(clientData);
    }

    //-----------------------------------------------------------------------------
    // Name: Show()
    // Desc:
    //-----------------------------------------------------------------------------
    void Show(void* user) {
        const int oneInt = 1;
        const float oneFloat = 0.01f;

        if (Variables::Debug) {
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
                                  nullptr, GL_FALSE);
            glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_ERROR,
                                  GL_DONT_CARE, 0, nullptr, GL_TRUE);
            assert(glGetError() == GL_NO_ERROR);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::GetStyle().Alpha = 0.80f;
        ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram]
          = ImVec4(0.19f, 0.42f, 0.67f, 1.0f);

        // Append example controls
        int heightOffset = 10;
        if (User::ShowGUI) {
            ImGui::SetNextWindowPos(glm::vec2(10, heightOffset)
                                      * IMGUI_RESIZE_FACTOR,
                                    ImGuiCond_Always);
            heightOffset += User::ShowGUI() + 5;
        }

        ImGui::SetNextWindowPos(
          glm::vec2(10, heightOffset) * IMGUI_RESIZE_FACTOR, ImGuiCond_Once);
        ImGui::SetNextWindowSize(glm::vec2(220, 100) * IMGUI_RESIZE_FACTOR,
                                 ImGuiCond_Always);
        ImGui::Begin("Shader Control");
        if (ImGui::Checkbox("USER_TEST", &Variables::Shader::UserTest))
            compileShaders();
#ifdef PGR2_SHOW_TOOL_TIPS
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Add define 'USER_TEST' into all shaders.");
#endif
        ImGui::SameLine(100.0f * IMGUI_RESIZE_FACTOR, 0.0f);
        if (ImGui::Button("compile shaders")) compileShaders();
#ifdef PGR2_SHOW_TOOL_TIPS
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Recompiles all shaders.");
#endif
        ImGui::InputScalar("int", ImGuiDataType_S32, &Variables::Shader::Int,
                           &oneInt);
#ifdef PGR2_SHOW_TOOL_TIPS
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("[GLSL] uniform int u_VariableInt");
#endif
        ImGui::SliderFloat("float", &Variables::Shader::Float, 0.0f, 1.0f,
                           "%.3f");
#ifdef PGR2_SHOW_TOOL_TIPS
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("[GLSL] uniform float u_VariableFloat");
#endif
        ImGui::End();

        ImGui::SetNextWindowPos(glm::vec2(10, heightOffset + 105)
                                  * IMGUI_RESIZE_FACTOR,
                                ImGuiCond_Once);
        ImGui::SetNextWindowSize(glm::vec2(220, 125) * IMGUI_RESIZE_FACTOR,
                                 ImGuiCond_Always);
        ImGui::Begin("Application Control");
        ImGui::InputScalar("int0", ImGuiDataType_S32, &Variables::Menu::Int0,
                           &oneInt);
#ifdef PGR2_SHOW_TOOL_TIPS
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("[App] Variables::Menu::Int0");
#endif
        ImGui::InputScalar("int1", ImGuiDataType_S32, &Variables::Menu::Int1,
                           &oneInt);
#ifdef PGR2_SHOW_TOOL_TIPS
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("[App] Variables::Menu::Int1");
#endif
        ImGui::InputScalar("float0", ImGuiDataType_Float,
                           &Variables::Menu::Float0, &oneFloat);
#ifdef PGR2_SHOW_TOOL_TIPS
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("[App] Variables::Menu::Float0");
#endif
        ImGui::InputScalar("float1", ImGuiDataType_Float,
                           &Variables::Menu::Float1, &oneFloat);
#ifdef PGR2_SHOW_TOOL_TIPS
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("[App] Variables::Menu::Float1");
#endif
        ImGui::End();

        // Create statistic window
        ImGui::SetNextWindowPos(glm::vec2(10, heightOffset + 235)
                                  * IMGUI_RESIZE_FACTOR,
                                ImGuiCond_Once);
        ImGui::SetNextWindowSize(glm::vec2(220, 220) * IMGUI_RESIZE_FACTOR,
                                 ImGuiCond_Always);
        ImGui::Begin("Statistic");
        ImGui::Text("Frame %d, FPS %.3f", Statistic::Frame::ID, Statistic::FPS);

        // GPU time
        static float gpu_times[50] = {};
        static int gpu_times_offset = 0;
        static int gpu_max_time = 0;
        static int gpu_max_time_200 = 0;
        if (gpu_max_time < Statistic::Frame::GPUTime)
            gpu_max_time = Statistic::Frame::GPUTime;
        if (gpu_max_time_200 < Statistic::Frame::GPUTime)
            gpu_max_time_200 = Statistic::Frame::GPUTime;
        if (Statistic::Frame::ID % 200 == 0) {
            gpu_max_time = gpu_max_time_200;
            gpu_max_time_200 = 0;
        }
        gpu_times[gpu_times_offset] = Statistic::Frame::GPUTime;
        gpu_times_offset = (gpu_times_offset + 1) % IM_ARRAYSIZE(gpu_times);
        char overlay[32];
        sprintf(overlay, "GPU time: %d", Statistic::Frame::GPUTime);
        ImGui::PlotLines("", gpu_times, IM_ARRAYSIZE(gpu_times),
                         gpu_times_offset, overlay, 0.0f, gpu_max_time,
                         glm::vec2(205, 50) * IMGUI_RESIZE_FACTOR);

        // CPU time
        static float cpu_times[50] = {};
        static int cpu_times_offset = 0;
        static int cpu_max_time = 0;
        static int cpu_max_time_200 = 0;
        if (cpu_max_time < Statistic::Frame::CPUTime)
            cpu_max_time = Statistic::Frame::CPUTime;
        if (cpu_max_time_200 < Statistic::Frame::CPUTime)
            cpu_max_time_200 = Statistic::Frame::CPUTime;
        if (Statistic::Frame::ID % 200 == 0) {
            cpu_max_time = cpu_max_time_200;
            cpu_max_time_200 = 0;
        }
        cpu_times[gpu_times_offset] = Statistic::Frame::CPUTime;
        cpu_times_offset = (cpu_times_offset + 1) % IM_ARRAYSIZE(cpu_times);
        sprintf(overlay, "CPU time: %d", Statistic::Frame::CPUTime);
        ImGui::PlotLines("", cpu_times, IM_ARRAYSIZE(cpu_times),
                         cpu_times_offset, overlay, 0.0f, cpu_max_time,
                         glm::vec2(205, 50) * IMGUI_RESIZE_FACTOR);

        if (Variables::ShowMemStat) {
            ImGui::Text("Total GPU memory %d KB",
                        Statistic::GPUMemory::TotalMemory);
            // ImGui::ProgressBar(Statistic::GPUMemory::DedicatedMemory /
            // static_cast<float>(Statistic::GPUMemory::TotalMemory),
            // glm::vec2(140.0f, 0.0f)); ImGui::SameLine(0.0f,
            // ImGui::GetStyle().ItemInnerSpacing.x); ImGui::Text("dedicated");
            ImGui::ProgressBar(
              Statistic::GPUMemory::AllocatedMemory
                / static_cast<float>(Statistic::GPUMemory::TotalMemory),
              glm::vec2(80.0f, 0.0f) * IMGUI_RESIZE_FACTOR);
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::Text("%u MB allocated",
                        static_cast<unsigned int>(
                          Statistic::GPUMemory::AllocatedMemory / 1024));
            ImGui::ProgressBar(
              Statistic::GPUMemory::FreeMemory
                / static_cast<float>(Statistic::GPUMemory::TotalMemory),
              glm::vec2(80.0f, 0.0f) * IMGUI_RESIZE_FACTOR);
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::Text("%u MB free",
                        static_cast<unsigned int>(
                          Statistic::GPUMemory::FreeMemory / 1024));
            // ImGui::SetNextItemWidth(80);
            // ImGui::ProgressBar(Statistic::GPUMemory::EvictedMemory /
            // static_cast<float>(Statistic::GPUMemory::TotalMemory),
            // glm::vec2(140.0f, 0.0f)); ImGui::SameLine(0.0f,
            // ImGui::GetStyle().ItemInnerSpacing.x); ImGui::Text("%u MB
            // evicted", static_cast<unsigned
            // int>(Statistic::GPUMemory::EvictedMemory / 1024));
        }
        ImGui::End();

        // Create zoom window
        ImGui::SetNextWindowPos(glm::vec2(10, heightOffset + 460)
                                  * IMGUI_RESIZE_FACTOR,
                                ImGuiCond_Once);
        ImGui::SetNextWindowSize(glm::vec2(220, 125) * IMGUI_RESIZE_FACTOR,
                                 ImGuiCond_Always);
        ImGui::Begin("Inspector");
        if (ImGui::Button("take screenshot",
                          glm::vec2(204, 0) * IMGUI_RESIZE_FACTOR))
            Tools::SaveFramebuffer();

        const ImVec4 color = ImColor(Variables::Inspector::pixel.r,
                                     Variables::Inspector::pixel.g,
                                     Variables::Inspector::pixel.b, 255);
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
        ImGui::Button(" ", glm::vec2(19.0f, 0.0f) * IMGUI_RESIZE_FACTOR);
        ImGui::PopStyleColor(3);
        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::Text("R: %d G: %d B: %d A: %d", Variables::Inspector::pixel.r,
                    Variables::Inspector::pixel.g,
                    Variables::Inspector::pixel.b,
                    Variables::Inspector::pixel.a);

        if (ImGui::CollapsingHeader("Magnifier",
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("enabled", &Variables::Inspector::Zoom::Enabled);
            ImGui::SameLine(90.0f * IMGUI_RESIZE_FACTOR, 0.0f);
            ImGui::SetNextItemWidth(80.0f * IMGUI_RESIZE_FACTOR);
            ImGui::SliderInt("zoom", &Variables::Inspector::Zoom::Factor, 2, 10,
                             "%d");
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (Variables::Debug) {
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
                                  nullptr, GL_TRUE);
        }
    }
} // namespace GUI

//-----------------------------------------------------------------------------
// Name: WindowSizeChanged()
// Desc: internal
//-----------------------------------------------------------------------------
void WindowSizeChanged(GLFWwindow* window, int width, int height) {
    height = glm::max(height, 1);

    glViewport(0, 0, width, height);
    glm::ivec2 newWindowSize = glm::ivec2(width, height);
    ImGui::GetIO().DisplaySize = glm::vec2(Variables::WindowSize);

    if (User::WindowSizeChanged) User::WindowSizeChanged(newWindowSize);

    Variables::WindowSize = newWindowSize;
    Variables::Transform.update();
}

//-----------------------------------------------------------------------------
// Name: WindowClosed()
// Desc: internal
//-----------------------------------------------------------------------------
void WindowClosed(GLFWwindow* window) {
    if (Callbacks::User::OpenGLRelease) {
        Callbacks::User::OpenGLRelease();
    }
}

//-----------------------------------------------------------------------------
// Name: KeyboardChanged()
// Desc: internal
//-----------------------------------------------------------------------------
void KeyboardChanged(GLFWwindow* window, int key, int scancode, int action,
                     int mods) {
    static const float CameraSensitivity = 0.05f;

    if ((action != GLFW_PRESS) && (action != GLFW_REPEAT)) return;

    switch (key) {
        case GLFW_KEY_RIGHT:
            if (mods == GLFW_MOD_CONTROL) {
                Variables::Inspector::Zoom::Center.x += 1;
            } else {
                Variables::Transform.Camera.moveSide(CameraSensitivity);
                Variables::Transform.update();
            }
            break;
        case GLFW_KEY_LEFT:
            if (mods == GLFW_MOD_CONTROL) {
                Variables::Inspector::Zoom::Center.x -= 1;
            } else {
                Variables::Transform.Camera.moveSide(-CameraSensitivity);
                Variables::Transform.update();
            }
            break;
        case GLFW_KEY_DOWN:
            if (mods == GLFW_MOD_CONTROL) {
                Variables::Inspector::Zoom::Center.y -= 1;
            } else {
                Variables::Transform.Camera.moveForward(-CameraSensitivity);
                Variables::Transform.update();
            }
            break;
        case GLFW_KEY_UP:
            if (mods == GLFW_MOD_CONTROL) {
                Variables::Inspector::Zoom::Center.y += 1;
            } else {
                Variables::Transform.Camera.moveForward(CameraSensitivity);
                Variables::Transform.update();
            }
            break;
        case GLFW_KEY_Z:
            if (Variables::ModelTransformEnabled) {
                Variables::Transform.SceneZOffset
                  += (mods == GLFW_MOD_SHIFT) ? -0.5f : 0.5f;
                Variables::Transform.update();
            }
            return;
        case GLFW_KEY_I:
            Variables::Shader::Int += (mods == GLFW_MOD_SHIFT) ? -1 : 1;
            return;
        case GLFW_KEY_F:
            Variables::Shader::Float
              += (mods == GLFW_MOD_SHIFT) ? -0.01f : 0.01f;
            return;
        case GLFW_KEY_U:
            Variables::Shader::UserTest = !Variables::Shader::UserTest;
            return;
        case GLFW_KEY_C:
            fprintf(stderr, "\n");
            compileShaders();
            return;
    }

    if ((action == GLFW_PRESS) || (action == GLFW_REPEAT)) {
        if (key == GLFW_KEY_ESCAPE) Variables::AppClose = true;

        if (User::KeyboardChanged) User::KeyboardChanged(key, action, mods);
    }
}

//-----------------------------------------------------------------------------
// Name: MouseWheelChanged()
// Desc: internal
//-----------------------------------------------------------------------------
void MouseWheelChanged(GLFWwindow* window, double dx, double dy) {
    Variables::Transform.Camera.zoom(static_cast<float>(dy));
    Variables::Transform.update();
}

//-----------------------------------------------------------------------------
// Name: MouseButtonChanged()
// Desc: internal
//-----------------------------------------------------------------------------
void MouseButtonChanged(GLFWwindow* window, int button, int action, int mods) {
    static std::chrono::system_clock::time_point lastClickTime[3];
    const bool clicked = (action == GLFW_PRESS);
    bool doubleClicked = false;

    if (clicked) {
        const auto now = std::chrono::system_clock::now();

        switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                Variables::MouseLeftPressed = true;
                lastClickTime[GLFW_MOUSE_BUTTON_LEFT] = now;
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                Variables::MouseRightPressed = true;

                // Move zoom window on double-click
                {
                    const double diff_ms
                      = std::chrono::duration<double, std::milli>(
                          now - lastClickTime[GLFW_MOUSE_BUTTON_RIGHT])
                          .count();
                    if (Variables::Inspector::Zoom::Enabled && (diff_ms < 200))
                        Variables::Inspector::Zoom::Center
                          = glm::round(Variables::MousePosition);
                }

                lastClickTime[GLFW_MOUSE_BUTTON_RIGHT] = now;
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                Variables::MouseMiddlePressed = true;
                lastClickTime[GLFW_MOUSE_BUTTON_MIDDLE] = now;
                glReadPixels(
                  Variables::MousePosition.x,
                  Variables::WindowSize.y - Variables::MousePosition.y, 1, 1,
                  GL_RGBA, GL_UNSIGNED_BYTE, &Variables::Inspector::pixel);
                break;
        }
    } else {
        Variables::MouseLeftPressed = Variables::MouseRightPressed
          = Variables::MouseMiddlePressed = false;
    }

    if (User::MouseButtonChanged) User::MouseButtonChanged(button, action);
}

//-----------------------------------------------------------------------------
// Name: MousePositionChanged()
// Desc: internal
//-----------------------------------------------------------------------------
void MousePositionChanged(GLFWwindow* window, double x, double y) {
    constexpr float SceneMovementSensitivity = 0.02f;

    const bool leftMouse
      = Variables::MouseLeftPressed && !ImGui::GetIO().MouseDownOwned[0];
    const bool rightMouse
      = Variables::MouseRightPressed && !ImGui::GetIO().MouseDownOwned[1];
    const bool middleMouse
      = Variables::MouseMiddlePressed && !ImGui::GetIO().MouseDownOwned[2];
    const glm::vec2 lastMousePos = Variables::MousePosition;
    Variables::MousePosition = glm::vec2(x, y);
    const glm::vec2 change = Variables::MousePosition - lastMousePos;
    if (leftMouse) {
        if (!Variables::ModelTransformEnabled) {
            Variables::Transform.Camera.rotateAroundCenter(change.x, change.y);
        } else {
            Variables::Transform.SceneRotation.x
              += SceneMovementSensitivity * change.y;
            Variables::Transform.SceneRotation.y
              += SceneMovementSensitivity * change.x;
        }
        Variables::Transform.update();
    }
    if (rightMouse) {
        Variables::Transform.Camera.rotateAroundPosition(-change.x, change.y);
        Variables::Transform.update();
    }
    if (User::MousePositionChanged) User::MousePositionChanged(x, y);
    if (middleMouse) {
        glReadPixels(Variables::MousePosition.x, Variables::MousePosition.y, 1,
                     1, GL_RGBA, GL_UNSIGNED_BYTE,
                     &Variables::Inspector::pixel);
    }
}

//-----------------------------------------------------------------------------
// Name: PrintOGLDebugLog()
// Desc:
//-----------------------------------------------------------------------------
void
#ifdef _WIN32
  __stdcall
#endif
  PrintOGLDebugLog(GLenum source, GLenum type, GLuint id, GLenum severity,
                   GLsizei length, const GLchar* message,
                   const GLvoid* userParam) {
    // Disable buffer info message on nVidia drivers
    if ((id == 0x20070) && (severity == GL_DEBUG_SEVERITY_LOW)) return;
    if ((id == 0x20071) && (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        && (type == GL_DEBUG_TYPE_OTHER)) // daam4408
        return;

    switch (source) {
        case GL_DEBUG_SOURCE_API:
            fprintf(stderr, "Source  : API\n");
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            fprintf(stderr, "Source  : window system\n");
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            fprintf(stderr, "Source  : shader compiler\n");
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            fprintf(stderr, "Source  : third party\n");
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            fprintf(stderr, "Source  : application\n");
            break;
        case GL_DEBUG_SOURCE_OTHER:
            fprintf(stderr, "Source  : other\n");
            break;
        default:
            fprintf(stderr, "Source  : unknown\n");
            break;
    }

    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            fprintf(stderr, "Type    : error\n");
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            fprintf(stderr, "Type    : deprecated behaviour\n");
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            fprintf(stderr, "Type    : undefined behaviour\n");
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            fprintf(stderr, "Type    : portability issue\n");
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            fprintf(stderr, "Type    : performance issue\n");
            break;
        case GL_DEBUG_TYPE_OTHER:
            fprintf(stderr, "Type    : other\n");
            break;
        default:
            fprintf(stderr, "Type    : unknown\n");
            break;
    }

    fprintf(stderr, "ID      : 0x%x\n", id);

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            fprintf(stderr, "Severity: high\n");
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            fprintf(stderr, "Severity: medium\n");
            break;
        case GL_DEBUG_SEVERITY_LOW:
            fprintf(stderr, "Severity: low\n");
            break;
        default:
            fprintf(stderr, "Severity: unknown\n");
            break;
    }

    fprintf(stderr, "Message : %s\n", message);
    fprintf(stderr, "----------------------------------------------------------"
                    "---------------------\n");
}

//-----------------------------------------------------------------------------
// Name: glfwError()
// Desc:
//-----------------------------------------------------------------------------
void glfwError(int error, const char* description) {
    fprintf(stderr, "GLFW error: %s\n", description);
}
}; // end of namespace Callbacks

//-----------------------------------------------------------------------------
// Name: ShowMagnifier()
// Desc:
//-----------------------------------------------------------------------------
void ShowMagnifier() {
    if (!Variables::Inspector::Zoom::Enabled) return;

    static GLuint s_MagTexture = 0;
    static const GLsizei s_MagWindowResolution = 200 * WINDOW_RESIZE_FACTOR;
    static const GLsizei s_MagWindowHalfResolution = s_MagWindowResolution >> 1;
    static GLsizei s_MagTextureResolution = 0;
    static std::vector<GLubyte> s_Pixels(s_MagWindowResolution
                                         * s_MagWindowResolution * 4);

    const GLsizei magTextureResolution
      = s_MagWindowResolution / Variables::Inspector::Zoom::Factor;
    const GLsizei magTextureHalfResolution = magTextureResolution >> 1;
    if (!s_MagTexture || (s_MagTextureResolution != magTextureResolution)) {
        glDeleteTextures(1, &s_MagTexture);
        s_MagTextureResolution = magTextureResolution;

        glCreateTextures(GL_TEXTURE_2D, 1, &s_MagTexture);
        glTextureParameteri(s_MagTexture, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_BORDER);
        glTextureParameteri(s_MagTexture, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_BORDER);
        glTextureParameteri(s_MagTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(s_MagTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameterfv(s_MagTexture, GL_TEXTURE_BORDER_COLOR,
                             &glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)[0]);
        glTextureStorage2D(s_MagTexture, 1, GL_RGBA8, s_MagTextureResolution,
                           s_MagTextureResolution);
    }

    const GLint invY
      = Variables::WindowSize.y - Variables::Inspector::Zoom::Center.y;
    const GLint x = glm::clamp(
      Variables::Inspector::Zoom::Center.x - magTextureHalfResolution, 0,
      Variables::WindowSize.x - magTextureResolution);
    const GLint y = glm::clamp(invY - magTextureHalfResolution, 0,
                               Variables::WindowSize.y - magTextureResolution);
    glReadPixels(x, y, s_MagTextureResolution, s_MagTextureResolution, GL_RGBA,
                 GL_UNSIGNED_BYTE, &s_Pixels[0]);
    glTextureSubImage2D(s_MagTexture, 0, 0, 0, s_MagTextureResolution,
                        s_MagTextureResolution, GL_RGBA, GL_UNSIGNED_BYTE,
                        &s_Pixels[0]);

    // Show magnificier texture
    const GLint x1 = glm::clamp(
      Variables::Inspector::Zoom::Center.x - s_MagWindowHalfResolution, 0,
      Variables::WindowSize.x - s_MagWindowResolution);
    const GLint y1
      = glm::clamp(invY - s_MagWindowHalfResolution, 0,
                   Variables::WindowSize.y - s_MagWindowResolution);
    Tools::Texture::Show2D(s_MagTexture, x1, y1, s_MagWindowResolution,
                           s_MagWindowResolution, 1.0f, 0, true);
}

//-----------------------------------------------------------------------------
// Name: common_main()
// Desc:
//-----------------------------------------------------------------------------
int common_main(int window_width, int window_height, const char* window_title,
                const int* opengl_config, TInitGLCallback cbUserInitGL,
                TReleaseOpenGLCallback cbUserReleaseGL, TShowGUICallback cbGUI,
                TDisplayCallback cbUserDisplay,
                TWindowSizeChangedCallback cbUserWindowSizeChanged,
                TKeyboardChangedCallback cbUserKeyboardChanged,
                TMouseButtonChangedCallback cbUserMouseButtonChanged,
                TMousePositionChangedCallback cbUserMousePositionChanged) {
    // Setup user callback functions
    assert(cbUserDisplay && cbUserInitGL);
    Callbacks::User::Display = cbUserDisplay;
    Callbacks::User::ShowGUI = cbGUI;
    Callbacks::User::OpenGLRelease = cbUserReleaseGL;
    Callbacks::User::WindowSizeChanged = cbUserWindowSizeChanged;
    Callbacks::User::KeyboardChanged = cbUserKeyboardChanged;
    Callbacks::User::MouseButtonChanged = cbUserMouseButtonChanged;
    Callbacks::User::MousePositionChanged = cbUserMousePositionChanged;

    // Setup internal variables
    Variables::WindowSize = glm::ivec2(window_width * WINDOW_RESIZE_FACTOR,
                                       window_height * WINDOW_RESIZE_FACTOR);

    // Setup temporary variables
    bool bDisableVSync = false;
    bool bShowRotation = false;
    bool bAutoSwapDisabled = false;

    // Intialize GLFW
    glfwSetErrorCallback(Callbacks::glfwError);
    if (!glfwInit()) return 1;

    const int* pConfig = opengl_config;
    while (*pConfig != 0) {
        const int hint = *pConfig++;
        const int value = *pConfig++;

        switch (hint) {
            case PGR2_SHOW_MEMORY_STATISTICS:
                Variables::ShowMemStat = (value == GL_TRUE);
                break;
            case PGR2_DISABLE_VSYNC:
                bDisableVSync = (value == GL_TRUE);
                break;
            case PGR2_DISABLE_BUFFER_SWAP:
                bAutoSwapDisabled = (value == GL_TRUE);
                break;
            default:
                glfwWindowHint(hint, value);
                if (hint == GLFW_OPENGL_DEBUG_CONTEXT)
                    Variables::Debug = (value == GL_TRUE);
                if ((hint == GLFW_CONTEXT_VERSION_MAJOR) && (value < 4)) {
                    fprintf(stderr,
                            "Error: OpenGL version must be 4 or higher\n");
                    return 2;
                }
        }
    }

    // Create a window
    Variables::Window
      = glfwCreateWindow(Variables::WindowSize.x, Variables::WindowSize.y,
                         window_title, nullptr, nullptr);
    if (!Variables::Window) {
        fprintf(stderr, "Error: unable to create window\n");
        return 3;
    }
    glfwSetWindowPos(Variables::Window, 100, 100);
    glfwMakeContextCurrent(Variables::Window);
    glfwSetInputMode(Variables::Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetScrollCallback(Variables::Window, Callbacks::MouseWheelChanged);
    // Set GLFW event callbacks
    glfwSetWindowSizeCallback(Variables::Window, Callbacks::WindowSizeChanged);
    glfwSetWindowCloseCallback(Variables::Window, Callbacks::WindowClosed);
    glfwSetMouseButtonCallback(Variables::Window,
                               Callbacks::MouseButtonChanged);
    glfwSetCursorPosCallback(Variables::Window,
                             Callbacks::MousePositionChanged);
    glfwSetKeyCallback(Variables::Window, Callbacks::KeyboardChanged);

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW error: %s\n", glewGetErrorString(err));
        return 4;
    }

    // Print debug info
    (void)fprintf(
      stderr, "VENDOR  : %s\nVERSION : %s\nRENDERER: %s\nGLSL    : %s\n",
      glGetString(GL_VENDOR), glGetString(GL_VERSION), glGetString(GL_RENDERER),
      glGetString(GL_SHADING_LANGUAGE_VERSION));
    (void)fprintf(stderr,
                  "----------------------------------------------------------"
                  "---------------------\n");

    // Init GUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    io.Fonts->Build();

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(Variables::Window, true);
    ImGui_ImplOpenGL3_Init("#version 450");
    // Scale them to the appropriate size
    ImGui::GetStyle().ScaleAllSizes(IMGUI_RESIZE_FACTOR);
    ImGui::GetIO().FontGlobalScale = IMGUI_RESIZE_FACTOR;

    // Init STB_IMAGE
    stbi_set_flip_vertically_on_load(1);
    stbi_flip_vertically_on_write(1);

    // Enable OGL debug
    if (Variables::Debug && glfwExtensionSupported("GL_ARB_debug_output")) {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(Callbacks::PrintOGLDebugLog, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
                              nullptr, GL_TRUE);

        GLint numBinaryFormats = 0;
        glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &numBinaryFormats);
        Variables::ShaderBinaryOutput = (numBinaryFormats > 0);
    } else
        Variables::Debug = false;

    // Disable VSync if required
    if (bDisableVSync) {
        glfwSwapInterval(0);
        fprintf(stderr, "VSync is disabled.\n");
    } else
        fprintf(stderr, "VSync is enabled!\n");

    // Check
    if (Variables::ShowMemStat) {
        Variables::ShowMemStat
          = glfwExtensionSupported("GL_NVX_gpu_memory_info") == GLFW_TRUE;
        if (Variables::ShowMemStat) {
            glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX,
                          &Statistic::GPUMemory::DedicatedMemory);
            glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX,
                          &Statistic::GPUMemory::TotalMemory);
        }
    }

    // Init OGL
    if (cbUserInitGL) {
        cbUserInitGL();
        (void)fprintf(stderr, "OpenGL initialized...\n");
        (void)fprintf(stderr,
                      "------------------------------------------------------"
                      "-------------------------\n");
    }

    glGenQueries(OpenGL::NumQueries, OpenGL::Query);

    // Main loop
    GLuint64 gpu_frame_start;
    GLuint64 gpu_frame_end;

    // Default transformations
    Variables::Transform.update();

    while (!glfwWindowShouldClose(Variables::Window) && !Variables::AppClose) {
        glfwPollEvents();

        // Increase frame counter
        Statistic::Frame::ID++;

        // Update transformations and default variables if used
        {
            for (size_t i = 0; i < OpenGL::programs.size(); i++) {
                const OpenGL::Program& program = OpenGL::programs[i];
                glUseProgram(program.id);
                if (program.MVPMatrix > -1)
                    glUniformMatrix4fv(
                      program.MVPMatrix, 1, GL_FALSE,
                      &Variables::Transform.ModelViewProjection[0][0]);
                if (program.ModelMatrix > -1)
                    glUniformMatrix4fv(program.ModelMatrix, 1, GL_FALSE,
                                       &Variables::Transform.Model[0][0]);
                if (program.ViewMatrix > -1)
                    glUniformMatrix4fv(program.ViewMatrix, 1, GL_FALSE,
                                       &Variables::Transform.View[0][0]);
                if (program.ModelViewMatrix > -1)
                    glUniformMatrix4fv(program.ModelViewMatrix, 1, GL_FALSE,
                                       &Variables::Transform.ModelView[0][0]);
                if (program.ModelViewMatrixInverse > -1)
                    glUniformMatrix4fv(
                      program.ModelViewMatrixInverse, 1, GL_FALSE,
                      &Variables::Transform.ModelViewInverse[0][0]);
                if (program.ProjectionMatrix > -1)
                    glUniformMatrix4fv(program.ProjectionMatrix, 1, GL_FALSE,
                                       &Variables::Transform.Projection[0][0]);
                if (program.NormalMatrix > -1)
                    glUniformMatrix3fv(program.NormalMatrix, 1, GL_FALSE,
                                       &Variables::Transform.Normal[0][0]);
                if (program.Viewport > -1)
                    glUniform4fv(program.Viewport, 1,
                                 &Variables::Transform.Viewport.x);
                if (program.ZOffset > -1)
                    glUniform1f(program.ZOffset,
                                Variables::Transform.SceneZOffset);
                if (program.VariableInt > -1)
                    glUniform1i(program.VariableInt, Variables::Shader::Int);
                if (program.VariableFloat > -1)
                    glUniform1f(program.VariableFloat,
                                Variables::Shader::Float);
                if (program.FrameCounter > -1)
                    glUniform1i(program.FrameCounter, Statistic::Frame::ID);
            }
        }
        glUseProgram(0);

        // Clean the pipeline
        // glFinish();

        const std::chrono::high_resolution_clock::time_point cpu_start
          = std::chrono::high_resolution_clock::now();
        glQueryCounter(OpenGL::Query[OpenGL::FrameStartQuery], GL_TIMESTAMP);
        if (Callbacks::User::Display) {
            Callbacks::User::Display();
            assert(glGetError() == GL_NO_ERROR);
            // Restore OGL states
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glUseProgram(0);
        }
        glQueryCounter(OpenGL::Query[OpenGL::FrameEndQuery], GL_TIMESTAMP);
        const std::chrono::high_resolution_clock::time_point cpu_end
          = std::chrono::high_resolution_clock::now();
        Statistic::Frame::CPUTime = static_cast<int>(
          std::chrono::duration_cast<std::chrono::microseconds>(cpu_end
                                                                - cpu_start)
            .count());

        // Get memory statistics if required
        if (Variables::ShowMemStat) {
            glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX,
                          &Statistic::GPUMemory::FreeMemory);
            glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX,
                          &Statistic::GPUMemory::EvictedMemory);
            Statistic::GPUMemory::AllocatedMemory
              = Statistic::GPUMemory::TotalMemory
                - Statistic::GPUMemory::FreeMemory;
        }

        {
            // Unbind query buffer if bound
            Tools::QueryBufferRelease queryBufferRelease;
            glGetQueryObjectui64v(OpenGL::Query[OpenGL::FrameStartQuery],
                                  GL_QUERY_RESULT, &gpu_frame_start);
            glGetQueryObjectui64v(OpenGL::Query[OpenGL::FrameEndQuery],
                                  GL_QUERY_RESULT, &gpu_frame_end);
            Statistic::Frame::GPUTime
              = static_cast<int>(gpu_frame_end - gpu_frame_start);
        }

        // Count FPS
        static unsigned long GPUTimeSum = 0;
        static unsigned int FPSFrameCount = 0;
        FPSFrameCount++;
        GPUTimeSum += Statistic::Frame::GPUTime;
        if (GPUTimeSum > 1000000) {
            Statistic::FPS
              = FPSFrameCount * 1000000000.0f / static_cast<float>(GPUTimeSum);
            GPUTimeSum = 0;
            FPSFrameCount = 0;
        }

        // Show Magnifier
        ShowMagnifier();

        // Render GUI
        Callbacks::GUI::Show(nullptr);

        // Present frame buffer
        if (!bAutoSwapDisabled) glfwSwapBuffers(Variables::Window);

        glfwPollEvents();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(Variables::Window);

    // Terminate GLFW
    glfwTerminate();

    return 0;
}

//-----------------------------------------------------------------------------
// Name: common_main()
// Desc:
//-----------------------------------------------------------------------------
int common_main(int window_width, int window_height, const char* window_title,
                TInitGLCallback cbUserInitGL,
                TReleaseOpenGLCallback cbUserReleaseGL, TShowGUICallback cbGUI,
                TDisplayCallback cbUserDisplay,
                TWindowSizeChangedCallback cbUserWindowSizeChanged,
                TKeyboardChangedCallback cbUserKeyboardChanged,
                TMouseButtonChangedCallback cbUserMouseButtonChanged,
                TMousePositionChangedCallback cbUserMousePositionChanged) {
    return common_main(window_width, window_height, window_title, nullptr,
                       cbUserInitGL, cbUserReleaseGL, cbGUI, cbUserDisplay,
                       cbUserWindowSizeChanged, cbUserKeyboardChanged,
                       cbUserMouseButtonChanged, cbUserMousePositionChanged);
}

#endif // _COMMON_INCLUDE_COMMON_H_
