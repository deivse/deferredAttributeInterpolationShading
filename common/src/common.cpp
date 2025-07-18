#include <common.h>
#include <tools.h>

namespace Callbacks {
namespace User {
    TReleaseOpenGLCallback OpenGLRelease = nullptr;
    TDisplayCallback Display = nullptr;
    TShowGUICallback ShowGUI = nullptr;
    TWindowSizeChangedCallback WindowSizeChanged = nullptr;
    TMouseButtonChangedCallback MouseButtonChanged = nullptr;
    TMousePositionChangedCallback MousePositionChanged = nullptr;
    TKeyboardChangedCallback KeyboardChanged = nullptr;
} // namespace User

namespace GUI {
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
            compileShaders();
            return;
    }

    if ((action == GLFW_PRESS) || (action == GLFW_REPEAT)) {
        if (key == GLFW_KEY_ESCAPE) Variables::AppClose = true;

        if (User::KeyboardChanged) User::KeyboardChanged(key, action, mods);
    }
}

} // namespace Callbacks

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
                Variables::ShowMemStat = (value == 1);
                break;
            case PGR2_DISABLE_VSYNC:
                bDisableVSync = (value == 1);
                break;
            case PGR2_DISABLE_BUFFER_SWAP:
                bAutoSwapDisabled = (value == 1);
                break;
            default:
                glfwWindowHint(hint, value);
                if (hint == GLFW_OPENGL_DEBUG_CONTEXT)
                    Variables::Debug = (value == 1);
                if ((hint == GLFW_CONTEXT_VERSION_MAJOR) && (value < 4)) {
                    spdlog::critical("Error: OpenGL version must be 4 or "
                                     "higher");
                    return 2;
                }
        }
    }

    // Create a window
    Variables::Window
      = glfwCreateWindow(Variables::WindowSize.x, Variables::WindowSize.y,
                         window_title, nullptr, nullptr);
    if (!Variables::Window) {
        spdlog::critical("Error: unable to create window");
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

    glbinding::initialize(glfwGetProcAddress);

    // Print debug info
    constexpr auto getGlStringView = [](gl::GLenum name) {
        return std::string_view{
          reinterpret_cast<const char*>(glGetString(name))};
    };
    spdlog::info("VENDOR  : {}", getGlStringView(GL_VENDOR));
    spdlog::info("VERSION : {}", getGlStringView(GL_VERSION));
    spdlog::info("RENDERER: {}", getGlStringView(GL_RENDERER));
    spdlog::info("GLSL    : {}", getGlStringView(GL_SHADING_LANGUAGE_VERSION));

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
        spdlog::info("VSync is disabled.");
    } else {
        spdlog::info("VSync is enabled.");
    }

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
        spdlog::info("OpenGL initialized...");
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
