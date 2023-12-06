get_filename_component(TARGET_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

Message(STATUS "-----------------------------------------------------------------")
Message(STATUS "Processing ${TARGET_NAME}:")

# ####################################################################################
# Add source files and shaders
#
file(GLOB SOURCE_FILES *.cpp *.hpp *.inl *.h *.c)
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

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SHADER_FILES} $<TARGET_FILE_DIR:${TARGET_NAME}>
)

_add_target_definitions(${TARGET_NAME})

# ####################################################################################
# Linkages
#
target_link_libraries(${TARGET_NAME} PRIVATE libcommon)
