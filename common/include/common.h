//-----------------------------------------------------------------------------
//  [PGR2] Common function definitions
//  27/02/2008
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#ifndef COMMON_INCLUDE_COMMON
#define COMMON_INCLUDE_COMMON
#include <cassert>
#include <chrono>

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>
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

#include <stb_image.h>
#include <stb_image_write.h>

#include <spdlog/spdlog.h>
#include <fmt/color.h>

using namespace gl;

// INTERNAL USER CALLBACK FUNCTION POINTERS____________________________________
namespace Callbacks {
namespace User {
    extern TReleaseOpenGLCallback OpenGLRelease;
    extern TDisplayCallback Display;
    extern TShowGUICallback ShowGUI;
    extern TWindowSizeChangedCallback WindowSizeChanged;
    extern TMouseButtonChangedCallback MouseButtonChanged;
    extern TMousePositionChangedCallback MousePositionChanged;
    extern TKeyboardChangedCallback KeyboardChanged;
} // end of namespace User

namespace GUI {
    //-----------------------------------------------------------------------------
    // Name: Set()
    // Desc: Default GUI "set" callback
    //-----------------------------------------------------------------------------
    template<typename T>
    inline void Set(const void* value, void* clientData) {
        *static_cast<T*>(clientData) = *static_cast<const T*>(value);
    }
    //-----------------------------------------------------------------------------
    // Name: Set()
    // Desc: Default GUI "set" callback
    //-----------------------------------------------------------------------------
    template<typename T>
    inline void SetCompile(const void* value, void* clientData) {
        *static_cast<T*>(clientData) = *static_cast<const T*>(value);
        compileShaders();
    }
    //-----------------------------------------------------------------------------
    // Name: Get()
    // Desc: Default GUI "get" callback
    //-----------------------------------------------------------------------------
    template<typename T>
    inline void Get(void* value, void* clientData) {
        *static_cast<T*>(value) = *static_cast<T*>(clientData);
    }

    //-----------------------------------------------------------------------------
    // Name: Show()
    // Desc:
    //-----------------------------------------------------------------------------
    void Show(void* user);
} // namespace GUI

//-----------------------------------------------------------------------------
// Name: WindowSizeChanged()
// Desc: internal
//-----------------------------------------------------------------------------
inline void WindowSizeChanged(GLFWwindow* window, int width, int height) {
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
inline void WindowClosed(GLFWwindow* window) {
    if (Callbacks::User::OpenGLRelease) {
        Callbacks::User::OpenGLRelease();
    }
}

//-----------------------------------------------------------------------------
// Name: KeyboardChanged()
// Desc: internal
//-----------------------------------------------------------------------------
void KeyboardChanged(GLFWwindow* window, int key, int scancode, int action,
                     int mods);
//-----------------------------------------------------------------------------
// Name: MouseWheelChanged()
// Desc: internal
//-----------------------------------------------------------------------------
inline void MouseWheelChanged(GLFWwindow* window, double dx, double dy) {
    Variables::Transform.Camera.zoom(static_cast<float>(dy));
    Variables::Transform.update();
}

//-----------------------------------------------------------------------------
// Name: MouseButtonChanged()
// Desc: internal
//-----------------------------------------------------------------------------
inline void MouseButtonChanged(GLFWwindow* window, int button, int action,
                               int mods) {
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
inline void MousePositionChanged(GLFWwindow* window, double x, double y) {
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
inline void
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

    constexpr auto getMsgSourceString = [](GLenum source) {
        switch (source) {
            case GL_DEBUG_SOURCE_API:
                return "API";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                return "window system";
            case GL_DEBUG_SOURCE_SHADER_COMPILER:
                return "shader compiler";
            case GL_DEBUG_SOURCE_THIRD_PARTY:
                return "third party";
            case GL_DEBUG_SOURCE_APPLICATION:
                return "application";
            case GL_DEBUG_SOURCE_OTHER:
                return "other";
            default:
                return "unknown";
        }
    };

    constexpr auto getMsgTypeString = [](GLenum type) {
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:
                return "error";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                return "deprecated behaviour";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                return "undefined behaviour";
            case GL_DEBUG_TYPE_PORTABILITY:
                return "portability issue";
            case GL_DEBUG_TYPE_PERFORMANCE:
                return "performance issue";
            case GL_DEBUG_TYPE_OTHER:
                return "other";
            default:
                return "unknown";
        }
    };

    constexpr auto getMsgSeverityString = [](GLenum severity) {
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                return "high";
            case GL_DEBUG_SEVERITY_MEDIUM:
                return "medium";
            case GL_DEBUG_SEVERITY_LOW:
                return "low";
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                return "notification";
            default:
                return "unknown";
        }
    };

    const auto logLevel
      = (type == GL_DEBUG_TYPE_ERROR || severity == GL_DEBUG_SEVERITY_HIGH)
          ? spdlog::level::err
          : spdlog::level::warn;

    static auto oglMsgLabel = fmt::format(
      fmt::fg(fmt::terminal_color::bright_black) | fmt::emphasis::bold, "OpenGL");
    spdlog::log(logLevel, "[{}] {}", oglMsgLabel, message);
    spdlog::log(logLevel,
                "[{}] Type: {} | Source: {} | Severity: {} | Id: 0x{:x}",
                oglMsgLabel, getMsgTypeString(type), getMsgSourceString(source),
                getMsgSeverityString(severity), id);
}

//-----------------------------------------------------------------------------
// Name: glfwError()
// Desc:
//-----------------------------------------------------------------------------
inline void glfwError(int error, const char* description) {
    spdlog::error("GLFW error: {} (0x{:x})", description, error);
}
}; // end of namespace Callbacks

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
                TMousePositionChangedCallback cbUserMousePositionChanged);

//-----------------------------------------------------------------------------
// Name: common_main()
// Desc:
//-----------------------------------------------------------------------------
inline int
  common_main(int window_width, int window_height, const char* window_title,
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

#endif /* COMMON_INCLUDE_COMMON */
