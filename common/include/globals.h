#ifndef _COMMON_INCLUDE_GLOBALS_H_
#define _COMMON_INCLUDE_GLOBALS_H_

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "camera.h"

#include <glbinding/gl/gl.h>

using namespace gl;

// INTERNAL VARIABLES DEFINITIONS______________________________________________
namespace Variables {
GLFWwindow* Window = nullptr;
glm::ivec2 WindowSize = glm::ivec2(0);
glm::ivec2 LastWindowSize = glm::ivec2(0);
glm::vec2 MousePosition = glm::vec2(-1.0f, -1.0f);
bool MouseLeftPressed = false;
bool MouseRightPressed = false;
bool MouseMiddlePressed = false;
bool ModelTransformEnabled = true;
bool Debug = false;
bool ShaderBinaryOutput = false;
bool ShowMemStat = true;
bool AppClose = false;

namespace Inspector {
    glm::u8vec4 pixel;
    namespace Zoom {
        glm::ivec2 Center = glm::ivec2(0);
        bool Enabled = false;
        int Factor = 2;
    }; // namespace Zoom
};     // namespace Inspector

namespace Shader {      // Shader default variables
    int Int = 0;        // Value will be automatically passed to 'u_VariableInt'
                        // uniform in all shaders
    float Float = 0.0f; // Value will be automatically passed to 'u_FloatInt'
                        // uniform in all shaders
    bool UserTest = false; // USER_TEST will be define in every shader if true
};                         // namespace Shader

namespace Menu {
    int Int0
      = 0; // Value will be automatically shown in the sub-menu 'Statistic'
    float Float0
      = 0.0f; // Value will be automatically shown in the sub-menu 'Statistic'
    int Int1
      = 0; // Value will be automatically shown in the sub-menu 'Statistic'
    float Float1
      = 0.0f; // Value will be automatically shown in the sub-menu 'Statistic'
};            // namespace Menu

struct Transformation
{ // Scene transformation matrixes ( readonly variables - calculated
  // automatically every frame)
    glm::vec4 SceneRotation
      = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Scene orientation
    GLfloat SceneZOffset = 0.0f;           // Scene translation along z-axis

    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 ModelView;
    glm::mat4 ModelViewInverse;
    glm::mat4 Projection;
    glm::mat4 ModelViewProjection;
    glm::mat3 Normal;
    glm::vec4 Viewport;
    Tools::Camera Camera;

    void update() {
        Model = glm::rotate(
          glm::rotate(glm::translate(glm::mat4(1.0f),
                                     glm::vec3(0.0f, 0.0f, -SceneZOffset)),
                      SceneRotation.x, glm::vec3(1.0f, 0.0f, 0.0f)),
          SceneRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        View = Camera.GetViewMatrix();
        ModelView = View * Model;
        ModelViewInverse = glm::inverse(ModelView);
        Projection = glm::perspective(glm::radians(60.0f), float(WindowSize.x) / WindowSize.y,
                                      0.1f, 1000.0f);
        ModelViewProjection = Projection * ModelView;
        Normal = glm::inverseTranspose(glm::mat3(ModelView));

        glGetFloatv(GL_VIEWPORT, &Viewport.x);
    }
}; // end of namespace Transformation

Transformation Transform;
}; // end of namespace Variables

#endif // _COMMON_INCLUDE_GLOBALS_H_
