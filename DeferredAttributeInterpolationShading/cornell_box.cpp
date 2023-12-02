//-----------------------------------------------------------------------------
//  [PGR2] Cornell Box
//  25/04/2020
//-----------------------------------------------------------------------------
//  Controls:
//    [i/I]   ... inc/dec value of user integer variable
//    [f/F]   ... inc/dec value of user floating-point variable
//    [z/Z]   ... move scene along z-axis
//    [w]     ... toggle wire mode
//    [c]     ... compile shaders
//    [mouse] ... scene rotation (left button)
//-----------------------------------------------------------------------------
#include "common.h"
#include "models/cornell_box.h"

// GLOBAL VARIABLES____________________________________________________________
bool   g_WireMode = false; // Wire mode enabled/disabled
GLuint g_Program  =     0; // Shader program ID

// IMPLEMENTATION______________________________________________________________

//-----------------------------------------------------------------------------
// Name: display()
// Desc: 
//-----------------------------------------------------------------------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    glUseProgram(g_Program);
    glPolygonMode(GL_FRONT_AND_BACK, g_WireMode ? GL_LINE : GL_FILL);
    Tools::DrawCornellBox();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


//-----------------------------------------------------------------------------
// Name: init()
// Desc: 
//-----------------------------------------------------------------------------
void init() {
    // Default scene distance
    Variables::Transform.SceneZOffset = 3.0f;

    // Set OpenGL state variables
    glEnable(GL_DEPTH_TEST);
    glLineWidth(2.0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Load shader program
    compileShaders();
}


// Include GUI and control stuff
#include "controls.hpp"
