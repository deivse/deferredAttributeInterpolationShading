
#include <globals.h>

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

Transformation Transform{};
}; // end of namespace Variables
