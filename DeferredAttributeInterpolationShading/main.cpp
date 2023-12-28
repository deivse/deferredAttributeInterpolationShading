#include "common.h"
#include "models/cornell_box.h"

// GLOBAL VARIABLES____________________________________________________________
bool g_WireMode = false; // Wire mode enabled/disabled
GLuint g_Program = 0;    // Shader program ID
GLuint g_GBufferFBO = 0;
std::array<GLuint, 4> colorTextures{};

GLuint createGBuffer() {
    std::array colorAttachments{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};

    GLuint depthStencilTex{};
    Tools::Texture::Create2D(depthStencilTex, gl::GLenum::GL_DEPTH24_STENCIL8,
                             Variables::WindowSize);

    // TODO: make this shit sane, add a class, remove global state

    for (auto& texture : colorTextures) {
        Tools::Texture::Create2D(texture, gl::GLenum::GL_RGBA8,
                                 Variables::WindowSize);
    }

    GLuint fbo{};
    // Create a framebuffer object ...
    glCreateFramebuffers(1, &fbo);
    glNamedFramebufferDrawBuffers(fbo, colorAttachments.size(),
                                  colorAttachments.data());

    // and attach color and depth textures
    for (size_t i = 0; i < colorAttachments.size(); i++) {
        glNamedFramebufferTexture(fbo, colorAttachments[i], colorTextures[i],
                                  0);
    }
    glNamedFramebufferTexture(fbo, gl::GLenum::GL_DEPTH_STENCIL_ATTACHMENT,
                              depthStencilTex, 0);

    assert(glGetError() == GL_NO_ERROR);
    assert(glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER)
           == GL_FRAMEBUFFER_COMPLETE);
    return fbo;
}

std::vector<glm::vec3> createSphereGeometry(float radius, int slices) {
    std::vector<glm::vec3> vertices;
    Tools::Mesh::CreateSphereVertexMesh(vertices, radius, slices, slices);
    for (int i = 1; i < vertices.size(); i += 3)
        std::swap(vertices[i], vertices[i + 1]); // TODO: fix CW order bug
    return vertices;
}

void renderScene() {
    static GLuint vertexArray = 0;
    static GLuint vertexBuffer = 0;
    static GLuint attribBuffer = 0;
    static GLsizei numSlices = 0;
    static std::vector<glm::vec3> spherePositions;
    static std::vector<glm::vec3> sphereVertices;
    int g_NumSpheresPerRow = 20;
    int g_NumSphereSlices = 10;

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
    // glUniform1i(0, g_NumSpheresPerRow);
    glBindVertexArray(vertexArray);
    // TODO: Use instance rendering
    for (auto& position : spherePositions) {
        // glUniform3fv(3, 1, &position.x);
        glDrawArrays(GL_TRIANGLES, 0,
                     static_cast<GLsizei>(sphereVertices.size()));
    }
    glBindVertexArray(0);
}

void display() {
    Tools::GPUVSInvocationQuery
      vertexShaderCounter; // GPU vertex shader execution counter
    Tools::GPUFSInvocationQuery
      fragmentShaderCounter; // GPU fragment shader execution counter
    glUseProgram(g_Program);

    vertexShaderCounter.start();
    fragmentShaderCounter.start();

    glBindFramebuffer(GL_FRAMEBUFFER, g_GBufferFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode(GL_FRONT_AND_BACK, g_WireMode ? GL_LINE : GL_FILL);
    Tools::DrawCornellBox();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Tools::Texture::Show2D(colorTextures[0], Variables::WindowSize.x / 2, 0,
                           Variables::WindowSize.x / 2,
                           Variables::WindowSize.y / 2);
    Tools::Texture::Show2D(colorTextures[1], Variables::WindowSize.x / 2,
                           Variables::WindowSize.y / 2,
                           Variables::WindowSize.x / 2,
                           Variables::WindowSize.y / 2);
    vertexShaderCounter.stop();
    vertexShaderCounter.get();
    fragmentShaderCounter.stop();
    fragmentShaderCounter.get();
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
    constexpr int OGL_CONFIGURATION[] = {GLFW_CONTEXT_VERSION_MAJOR,
                                         4,
                                         GLFW_CONTEXT_VERSION_MINOR,
                                         6,
                                         GLFW_OPENGL_FORWARD_COMPAT,
                                         GL_FALSE.m_value,
                                         GLFW_OPENGL_DEBUG_CONTEXT,
                                         GL_TRUE.m_value,
                                         GLFW_OPENGL_PROFILE,
                                         GLFW_OPENGL_CORE_PROFILE,
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
