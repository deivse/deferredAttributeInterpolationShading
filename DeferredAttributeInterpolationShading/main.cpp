#include "common.h"
#include "models/cornell_box.h"

// GLOBAL VARIABLES____________________________________________________________
bool g_WireMode = false; // Wire mode enabled/disabled
GLuint g_Program = 0;    // Shader program ID
GLuint g_GBufferFBO = 0;
std::array<GLuint, 4> colorTextures{};

GLuint createGBuffer() {
    std::array bufferAttachments{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                 GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};

    // TODO: make this shit sane, add a class, remove global state

    for (auto& texture : colorTextures) {
        Tools::Texture::Create2D(texture, gl::GLenum::GL_RGBA8,
                                 Variables::WindowSize);
    }

    GLuint fbo{};
    // Create a framebuffer object ...
    glCreateFramebuffers(1, &fbo);
    glNamedFramebufferDrawBuffers(fbo, bufferAttachments.size(),
                                  bufferAttachments.data());

    // and attach color and depth textures
    for (size_t i = 0; i < bufferAttachments.size(); i++) {
        glNamedFramebufferTexture(fbo, bufferAttachments[i], colorTextures[i],
                                  0);
    }

    assert(glGetError() == GL_NO_ERROR);
    assert(glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    return fbo;
}

void display() {
    Tools::GPUVSInvocationQuery query{};
    glUseProgram(g_Program);

    glBindFramebuffer(GL_FRAMEBUFFER, g_GBufferFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    query.start();
    glPolygonMode(GL_FRONT_AND_BACK, g_WireMode ? GL_LINE : GL_FILL);
    Tools::DrawCornellBox();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    query.get();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Tools::Texture::Show2D(colorTextures[0], Variables::WindowSize.x / 2, 0, Variables::WindowSize.x / 2, Variables::WindowSize.y / 2);
    Tools::Texture::Show2D(colorTextures[1], Variables::WindowSize.x / 2, Variables::WindowSize.y / 2, Variables::WindowSize.x / 2, Variables::WindowSize.y / 2);
}

void init() {
    
    // Default scene distance
    Variables::Transform.SceneZOffset = 3.0f;

    // Set OpenGL state variables
    glEnable(GL_DEPTH_TEST);
    glLineWidth(2.0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    g_GBufferFBO = createGBuffer();

    // Load shader program
    compileShaders();
}

constexpr const char* help_message = "no help here";

void compileShaders(void* clientData) {
    // Create shader program object
    Tools::Shader::CreateShaderProgramFromFile(
      g_Program, "shader.vert", nullptr, nullptr, nullptr, "shader.frag");
}

int showGUI() {
    const int menuHeight = 55;

    ImGui::Begin("Render");
    ImGui::SetWindowSize(glm::vec2(220, menuHeight) * IMGUI_RESIZE_FACTOR,
                         ImGuiCond_Always);
    ImGui::Checkbox("wire mode", &g_WireMode);
    ImGui::End();
    return menuHeight;
}

//-----------------------------------------------------------------------------
// Name: keyboardChanged()
// Desc:
//-----------------------------------------------------------------------------
void keyboardChanged(int key, int action, int mods) {
    switch (key) {
        case GLFW_KEY_W:
            g_WireMode = !g_WireMode;
            break;
        default:
            break;
    }
}

//-----------------------------------------------------------------------------
// Name: main()
// Desc:
//-----------------------------------------------------------------------------
int main() {
    constexpr int OGL_CONFIGURATION[]
      = {GLFW_CONTEXT_VERSION_MAJOR,
         4,
         GLFW_CONTEXT_VERSION_MINOR,
         5,
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

    return common_main(
      1200, 900, "[PGR2] Cornell Box",
      static_cast<const int*>(OGL_CONFIGURATION), // OGL configuration hints
      init,                                       // Init GL callback function
      nullptr,         // Release GL callback function
      showGUI,         // Show GUI callback function
      display,         // Display callback function
      nullptr,         // Window resize callback function
      keyboardChanged, // Keyboard callback function
      nullptr,         // Mouse button callback function
      nullptr);        // Mouse motion callback function
}
