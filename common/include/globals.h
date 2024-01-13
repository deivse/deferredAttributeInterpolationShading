#ifndef COMMON_INCLUDE_GLOBALS
#define COMMON_INCLUDE_GLOBALS

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
extern GLFWwindow* Window;
extern glm::ivec2 WindowSize;
extern glm::ivec2 LastWindowSize;
extern glm::vec2 MousePosition;
extern bool MouseLeftPressed;
extern bool MouseRightPressed;
extern bool MouseMiddlePressed;
extern bool ModelTransformEnabled;
extern bool Debug;
extern bool ShaderBinaryOutput;
extern bool ShowMemStat;
extern bool AppClose;

namespace Inspector {
    extern glm::u8vec4 pixel;
    namespace Zoom {
        extern glm::ivec2 Center;
        extern bool Enabled;
        extern int Factor;
    }; // namespace Zoom
};     // namespace Inspector

namespace Shader { // Shader default variables
    extern int Int;       // Value will be automatically passed to 'u_VariableInt'
                   // uniform in all shaders
    extern float Float;   // Value will be automatically passed to 'u_FloatInt'
                   // uniform in all shaders
    extern bool UserTest; // USER_TEST will be define in every shader if true
};                 // namespace Shader

namespace Menu {
    extern int
      Int0; // Value will be automatically shown in the sub-menu 'Statistic'
    extern float
      Float0; // Value will be automatically shown in the sub-menu 'Statistic'
    extern int
      Int1; // Value will be automatically shown in the sub-menu 'Statistic'
    extern float
      Float1; // Value will be automatically shown in the sub-menu 'Statistic'
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
        Projection
          = glm::perspective(glm::radians(60.0f),
                             float(WindowSize.x) / WindowSize.y, 0.1f, 1000.0f);
        ModelViewProjection = Projection * ModelView;
        Normal = glm::inverseTranspose(glm::mat3(ModelView));

        glGetFloatv(GL_VIEWPORT, &Viewport.x);
    }
}; // end of namespace Transformation

extern Transformation Transform;
}; // end of namespace Variables

#endif /* COMMON_INCLUDE_GLOBALS */
