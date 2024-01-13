//-----------------------------------------------------------------------------
//  [PGR2] Forward Shading
//  12/01/2023
//-----------------------------------------------------------------------------
const char* help_message =
"[PGR2] Forward Shading\n\
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
        float const     radius = glm::compRand1(1.0f, g_NumSpheresPerRow);
        glm::vec3 const position = glm::normalize(glm::compRand3(glm::vec3(-1.0f), glm::vec3(1.0f))) * radius;
        float const     range = glm::compRand1(g_LightRangeLimits.x, g_LightRangeLimits.y);

        g_Lights[i].position = glm::vec4(position, range);
        g_Lights[i].color    = glm::vec4(glm::compRand3(glm::vec3(0.0f), glm::vec3(0.5f)), 0.10f);
    }

    // Create buffer with lights and bind it as GL_UNIFORM_BUFFER to 0 binding point
    glDeleteBuffers(1, &g_LightBuffer);
    glCreateBuffers(1, &g_LightBuffer);
    glNamedBufferStorage(g_LightBuffer, sizeof(Light) * g_Lights.size(), g_Lights.data(), GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, g_LightBuffer);

    // Create vertex array to display all lights
    glDeleteVertexArrays(1, &g_LightsVertexArray);
    glCreateVertexArrays(1, &g_LightsVertexArray);
    glVertexArrayVertexBuffer(g_LightsVertexArray, 0, g_LightBuffer, 0, 2 * sizeof(glm::vec4));
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
        Tools::Shader::CreateShaderProgramFromFile(program, "light_center.vert", nullptr, nullptr, nullptr, "light.frag");
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
    for (int i = 1; i < vertices.size(); i += 3) std::swap(vertices[i], vertices[i + 1]); // TODO: fix CW order bug
    return vertices;
}

//-----------------------------------------------------------------------------
// Name: renderLightRanges()
// Desc: 
//-----------------------------------------------------------------------------
void renderLightRanges(bool bindShader) {
    static GLuint  program = 0;
    static GLuint  vertexArray = 0;
    static GLsizei numVertices = 0;
    if (program == 0) {
        Tools::Shader::CreateShaderProgramFromFile(program, "light_range.vert", nullptr, nullptr, nullptr, "light.frag");
        // Create buffer with sphere geometry
        std::vector<glm::vec3> vertices = createSphereGeometry(0.5f, 20);
        numVertices = static_cast<GLsizei>(vertices.size());
        GLuint vertexBuffer = 0;
        glCreateBuffers(1, &vertexBuffer);
        glNamedBufferStorage(vertexBuffer, vertices.size() * sizeof(glm::vec3), &vertices[0].x, GL_NONE);

        // Create vertex array for lights' spheres
        glCreateVertexArrays(1, &vertexArray);
        glVertexArrayVertexBuffer(vertexArray, 0, vertexBuffer, 0, sizeof(glm::vec3));
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
    }
    else {
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

    float const cosAngle = glm::cos(g_LightSpeed);
    float const sinAngle = glm::sin(g_LightSpeed);
    for (auto& light : g_Lights) {
        light.position.x = light.position.x * cosAngle + light.position.z * sinAngle;
        light.position.z = -light.position.x * sinAngle + light.position.z * cosAngle;
    }
    // Update light buffer
    glNamedBufferSubData(g_LightBuffer, 0, sizeof(Light) * g_Lights.size(), g_Lights.data());
}


//-----------------------------------------------------------------------------
// Name: updateLightRadius()
// Desc: 
//-----------------------------------------------------------------------------
void updateLightRadius() {
    for (auto& light : g_Lights) {
        light.position.w = glm::compRand1(g_LightRangeLimits.x, g_LightRangeLimits.y);
    }
    // Update light buffer
    glNamedBufferSubData(g_LightBuffer, 0, sizeof(Light) * g_Lights.size(), g_Lights.data());
}


//-----------------------------------------------------------------------------
// Name: renderScene()
// Desc: 
//-----------------------------------------------------------------------------
void renderScene() {
    static GLuint  vertexArray  = 0;
    static GLuint  vertexBuffer = 0;
    static GLuint  attribBuffer = 0;
    static GLsizei numSlices    = 0;
    static std::vector<glm::vec3> spherePositions;
    static std::vector<glm::vec3> sphereVertices;

    // Create texture for scene
    static GLuint  texture = Tools::Texture::CreateFromFile("../shared/textures/metal01.jpg");

    // Calculate sphere positions
    size_t const numSpheres = g_NumSpheresPerRow * g_NumSpheresPerRow * g_NumSpheresPerRow;
    if (numSpheres != spherePositions.size()) {
        spherePositions.clear();
        for (int i = 0; i < numSpheres; i++) {
            int const x = i % g_NumSpheresPerRow;
            int const y = i / g_NumSpheresPerRow % g_NumSpheresPerRow;
            int const z = i / (g_NumSpheresPerRow * g_NumSpheresPerRow) % g_NumSpheresPerRow;
            spherePositions.push_back(glm::vec3(x, y, z) - glm::vec3((g_NumSpheresPerRow - 1) * 0.5f));
        }

        glDeleteBuffers(1, &attribBuffer);
        glCreateBuffers(1, &attribBuffer);
        glNamedBufferStorage(attribBuffer, spherePositions.size() * sizeof(glm::vec3), &spherePositions[0].x, GL_NONE);

        numSlices = 0;
    }

    if ((vertexArray == 0) || (numSlices != g_NumSphereSlices)) {
        numSlices = g_NumSphereSlices;
        glDeleteVertexArrays(1, &vertexArray);
        glDeleteBuffers(1, &vertexBuffer);

        // Create vertex buffer
        sphereVertices = createSphereGeometry(0.5f, g_NumSphereSlices);
        glCreateBuffers(1, &vertexBuffer);
        glNamedBufferStorage(vertexBuffer, sphereVertices.size() * sizeof(glm::vec3), &sphereVertices[0].x, GL_NONE);
        // Create vertex array
        glCreateVertexArrays(1, &vertexArray);
        glVertexArrayVertexBuffer(vertexArray, 0, vertexBuffer, 0, sizeof(glm::vec3));
        glVertexArrayAttribBinding(vertexArray, 0, 0);
        glVertexArrayAttribFormat(vertexArray, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(vertexArray, 0);
        glVertexArrayVertexBuffer(vertexArray, 1, attribBuffer, 0, sizeof(glm::vec3));
        glVertexArrayAttribBinding(vertexArray, 1, 1);
        glVertexArrayAttribFormat(vertexArray, 1, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(vertexArray, 1);
        glVertexArrayBindingDivisor(vertexArray, 1, 1);
    }

    glBindTextureUnit(0, texture);
    glBindVertexArray(vertexArray);
    glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>(sphereVertices.size()), spherePositions.size());
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
void compileShaders(void *clientData) {
    resetAlgorithms(true);
}


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
    int menuHeight      = 305;
    int const oneInt    = 1;
    int const itemWidth = 140 * IMGUI_RESIZE_FACTOR;

    ImGui::Begin("Render");
        ImGui::SetWindowSize(ImVec2(220, menuHeight) * IMGUI_RESIZE_FACTOR, ImGuiCond_Always);
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::Combo("shading", &g_Algorithm, "Forward\0Deferred\0"))
            resetAlgorithms();
        ImGui::Checkbox("rotate lights", &g_RotateLights);
        ImGui::Checkbox("show lights", &g_ShowLights);
        ImGui::Checkbox("show ranges", &g_ShowLightRange);
        if (ImGui::Checkbox("synchronize timers", &g_ExplicitTimerSync)) {
            resetAlgorithms();
        }
        ImGui::SameLine(170.0f * IMGUI_RESIZE_FACTOR, -10.0f * IMGUI_RESIZE_FACTOR);
        if (ImGui::Button("reset"))
            resetAlgorithms();

        ImGui::Separator();
        ImGui::Text("SPHERES");
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::InputScalar("per row", ImGuiDataType_S32, &g_NumSpheresPerRow, &oneInt)) {
            g_NumSpheresPerRow = glm::clamp(g_NumSpheresPerRow, 1, 100);
            createLights();
            resetAlgorithms();
        }
        ImGui::SetNextItemWidth(itemWidth);
        if (ImGui::InputScalar("slices", ImGuiDataType_S32, &g_NumSphereSlices, &oneInt)) {
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
        if (ImGui::SliderFloat("##x", &g_LightRangeLimits.x, 0.01f, g_LightRangeLimits.y, "%.2f")) {
            updateLightRadius();
            resetAlgorithms();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(56 * IMGUI_RESIZE_FACTOR);
        if (ImGui::SliderFloat("##y", &g_LightRangeLimits.y, g_LightRangeLimits.x, 8.0f, "%.2f")) {
            updateLightRadius();
            resetAlgorithms();
        }
        ImGui::SameLine();
        ImGui::Text("range");
    ImGui::End();

    if (g_Algorithm == ForwardShading)
        g_ForwardShading.gui(glm::ivec2(235, 10), 260);
    else
        g_DeferredShading.gui(glm::ivec2(235, 10), 260);

    return menuHeight;
}


//-----------------------------------------------------------------------------
// Name: keyboardChanged()
// Desc:
//-----------------------------------------------------------------------------
void keyboardChanged(int key, int action, int mods) {
    switch (key) {
    case GLFW_KEY_R: resetAlgorithms(); break;
    case GLFW_KEY_A: g_RotateLights = !g_RotateLights; break;
    case GLFW_KEY_V: g_ShowLights = !g_ShowLights; break;
    case GLFW_KEY_K: g_ShowLightRange = !g_ShowLightRange; break;
    case GLFW_KEY_E: g_ExplicitTimerSync = !g_ExplicitTimerSync; break;
    case GLFW_KEY_S: g_NumSpheresPerRow += glm::clamp(g_NumSpheresPerRow + ((mods == GLFW_MOD_SHIFT) ? -1 : 1), 1, 100); break;
    case GLFW_KEY_D: g_NumSphereSlices += glm::clamp(g_NumSphereSlices + ((mods == GLFW_MOD_SHIFT) ? -1 : 1), 5, 100); break;
    case GLFW_KEY_L: g_NumLights += glm::clamp(g_NumLights + ((mods == GLFW_MOD_SHIFT) ? -1 : 1), 1, MaxLights); break;
    // g_LightSpeed
    // g_LightRangeLimits
    }
}


//-----------------------------------------------------------------------------
// Name: main()
// Desc: 
//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    int OGL_CONFIGURATION[] = {
        GLFW_CONTEXT_VERSION_MAJOR,  4,
        GLFW_CONTEXT_VERSION_MINOR,  5,
        GLFW_OPENGL_FORWARD_COMPAT,  GL_FALSE,
        GLFW_OPENGL_DEBUG_CONTEXT,   GL_TRUE,
        GLFW_OPENGL_PROFILE,         GLFW_OPENGL_COMPAT_PROFILE, // GLFW_OPENGL_CORE_PROFILE
        PGR2_SHOW_MEMORY_STATISTICS, GL_TRUE, 
        0
    };

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
