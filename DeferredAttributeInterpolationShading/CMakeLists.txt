get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

Message(STATUS "-----------------------------------------------------------------")
Message(STATUS "Processing ${TARGET_NAME}:")

# ####################################################################################
# Add source files and shaders
#
file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/*.inl ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
file(GLOB SHADER_FILES *.vert *.frag *.geom *.tcs *.tes *.mesh)

# ####################################################################################
# Some build related definitions
#
if(WIN32)
    add_definitions(/wd4996) # remove printf warning
    add_definitions(/wd4244) # remove double to float conversion warning
    add_definitions(/wd4305) # remove double to float truncation warning
endif()

# ####################################################################################
# Add executables
#
add_executable(${TARGET_NAME} ${SOURCE_FILES} ${SHADER_FILES})

_add_target_definitions(${TARGET_NAME})

target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ####################################################################################
# Linkages
#
target_link_libraries(${TARGET_NAME} PRIVATE libcommon)

# Copy shader files, even if target is not changed
add_custom_target(
    ${TARGET_NAME}_copy_shaders ALL
    DEPENDS ${SHADER_FILES}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SHADER_FILES} $<TARGET_FILE_DIR:${TARGET_NAME}>
    COMMENT "Copying shader files to build directory"
)
