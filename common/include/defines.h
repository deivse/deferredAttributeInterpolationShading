#ifndef _COMMON_INCLUDE_SETUP_H_
#define _COMMON_INCLUDE_SETUP_H_

#include <glm/fwd.hpp>

/**
 * @brief Defines, predeclarations and stuff
 *
 */

// #define PGR2_SHOW_TOOL_TIPS
#ifndef WINDOW_RESIZE_FACTOR
    #define WINDOW_RESIZE_FACTOR 1.0f
#endif
#ifndef IMGUI_RESIZE_FACTOR
    #define IMGUI_RESIZE_FACTOR WINDOW_RESIZE_FACTOR
#endif

// FUNCTION POINTER TYPES______________________________________________________
/* Function pointer types */
typedef void (*TInitGLCallback)(void);
typedef int (*TShowGUICallback)();
typedef void (*TDisplayCallback)(void);
typedef void (*TReleaseOpenGLCallback)(void);
typedef void (*TWindowSizeChangedCallback)(const glm::ivec2&);
typedef void (*TMouseButtonChangedCallback)(int, int);
typedef void (*TMousePositionChangedCallback)(double, double);
typedef void (*TKeyboardChangedCallback)(int, int, int);

// FORWARD DECLARATIONS________________________________________________________
void compileShaders(void* = nullptr);

// INTERNAL CONSTANTS__________________________________________________________
#define PGR2_SHOW_MEMORY_STATISTICS 0x0F000001
#define PGR2_DISABLE_VSYNC 0x0F000002
#define PGR2_DISABLE_BUFFER_SWAP 0x0F000004

#endif // _COMMON_INCLUDE_SETUP_H_
