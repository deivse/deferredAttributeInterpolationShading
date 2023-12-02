//-----------------------------------------------------------------------------
//  [PGR2] Cornell Box
//  25/04/2020
//-----------------------------------------------------------------------------
const char* help_message =
"[PGR2] Cornell Box\n\
-------------------------------------------------------------------------------\n\
CONTROLS:\n\
   [i/I]   ... inc/dec value of user integer variable\n\
   [f/F]   ... inc/dec value of user floating-point variable\n\
   [z/Z]   ... move scene along z-axis\n\
   [w]     ... toggle wire mode\n\
   [c]     ... compile shaders\n\
   [mouse] ... scene rotation (left button)\n\
-------------------------------------------------------------------------------"; 

// IMPLEMENTATION______________________________________________________________

//-----------------------------------------------------------------------------
// Name: compileShaders()
// Desc: 
//-----------------------------------------------------------------------------
void compileShaders(void *clientData) {
    // Create shader program object
    Tools::Shader::CreateShaderProgramFromFile(g_Program, "cornell_box.vert", nullptr, nullptr, nullptr, "cornell_box.frag");
}


//-----------------------------------------------------------------------------
// Name: showGUI()
// Desc: 
//-----------------------------------------------------------------------------
int showGUI() {
    int const menuHeight = 55;

    ImGui::Begin("Render");
    ImGui::SetWindowSize(ImVec2(220, menuHeight) * IMGUI_RESIZE_FACTOR, ImGuiCond_Always);
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
    case GLFW_KEY_W: g_WireMode = !g_WireMode; break;
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

    return common_main(1200, 900, "[PGR2] Cornell Box",
                       OGL_CONFIGURATION, // OGL configuration hints
                       init,              // Init GL callback function
                       nullptr,           // Release GL callback function
                       showGUI,           // Show GUI callback function
                       display,           // Display callback function
                       nullptr,           // Window resize callback function
                       keyboardChanged,   // Keyboard callback function
                       nullptr,           // Mouse button callback function
                       nullptr);          // Mouse motion callback function                   
}
