
find_package(glm REQUIRED CONFIG)
find_package(imgui REQUIRED CONFIG)
find_package(implot REQUIRED CONFIG)
find_package(glfw3 REQUIRED CONFIG)
find_package(glbinding REQUIRED CONFIG)
find_package(stb REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

target_compile_definitions(imgui::imgui INTERFACE IMGUI_USER_CONFIG="${CMAKE_SOURCE_DIR}/third-party/imgui/imconfig.h")
target_link_libraries(imgui::imgui INTERFACE glm::glm) # Because we define conversion operators from ImVec to glm::vec types in imconfig

SET(RESOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(PLATFORM_LIBRARIES)

if(UNIX)
    LIST(APPEND PLATFORM_LIBRARIES
        ${OPENGL_LIBRARY}
        "-lpthread -lm"
        "-ldl"
    )
else(UNIX)
    LIST(APPEND PLATFORM_LIBRARIES
        ${OPENGL_LIBRARY}
    )
endif(UNIX)

FILE(GLOB_RECURSE COMMON_SRC src/*.cpp src/*/*.cpp include/*.h include/*/*.h)

set(PGR2_APP_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/pgr2.ico)
configure_file(src/resources.rc.in resources.rc @ONLY)

set(IMGUI_BINDINGS_DIR ${CMAKE_SOURCE_DIR}/third-party/imgui/bindings)
set(IMGUI_BINDINGS_SRC ${IMGUI_BINDINGS_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_BINDINGS_DIR}/imgui_impl_opengl3.cpp)

add_library(libcommon STATIC ${CMAKE_CURRENT_BINARY_DIR}/resources.rc ${COMMON_SRC} ${IMGUI_BINDINGS_SRC})
target_include_directories(libcommon PUBLIC ${IMGUI_BINDINGS_DIR})

target_include_directories(libcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(libcommon PUBLIC spdlog::spdlog glm::glm imgui::imgui implot::implot glfw glbinding::glbinding glbinding::glbinding-aux stb::stb)
target_compile_definitions(libcommon PUBLIC RESOURCE_DIRECTORY="${RESOURCE_DIRECTORY}/"  GLFW_INCLUDE_NONE=1)

if(MSVC)
    target_compile_definitions(libcommon PUBLIC _CRT_SECURE_NO_WARNINGS)
endif(MSVC)
