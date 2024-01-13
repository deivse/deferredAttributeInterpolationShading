cmake_minimum_required(VERSION 3.5)
Project("PGR2Examples")

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

#####################################################################################
# Set CPU architectures
#
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  set ( ARCH "x64" CACHE STRING "CPU Architecture" )
  message( STATUS "Building for x64." )
else ()
  message( STATUS "Building for x86." )
  set ( ARCH "x86" CACHE STRING "CPU Architecture" )
endif()

#####################################################################################
# General compile flags
#

#####################################################################################
# Project directories
#
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin_${ARCH} CACHE PATH "Directory where executables will be stored" )
set( CMAKE_FIND_ROOT_PATH "" )
set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )


#####################################################################################
# Set default build to Release
#
if( NOT CMAKE_BUILD_TYPE )
  set( CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE )
endif( NOT CMAKE_BUILD_TYPE )


#####################################################################################
# OS related defines and settings
#
if( UNIX ) 
  set( OS "linux" )
  add_definitions( -DLINUX -g -std=c++11 )
else( UNIX )
  if( WIN32 )
    set( OS "win" )
    add_definitions( -DNOMINMAX )
  endif( WIN32 )
endif( UNIX )


#####################################################################################
# Macro for adding new project to the solution
#
macro(_add_target_definitions target)  
  target_compile_definitions(${target} PRIVATE PROJECT_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/")
  target_compile_definitions(${target} PRIVATE PROJECT_NAME="${target}")  
endmacro()


#####################################################################################
# Macro for loading shared library
#
macro( _find_shared_library target folder libName )
  if( ARCH STREQUAL "x86" )
    message( STATUS "Looking for ${folder}x86/${libName}" )
    file( GLOB SHARED_LIBRARIES "${folder}x86/${libName}" )
  else()
    message( STATUS "Looking for ${folder}x64/${libName}" )
    file( GLOB SHARED_LIBRARIES "${folder}x64/${libName}" )
  endif()
  
  list( LENGTH SHARED_LIBRARIES NUMD_LIBRARIES )
  if( NUMD_LIBRARIES EQUAL 0 )
    if( ARCH STREQUAL "x86" )
      message( STATUS "Following file was found but requested ${folder}x86/${libName}" )
	else()
      message( STATUS "Following file was found but requested ${folder}x64/${libName}" )
	endif()
    set ( ${target} "NOTFOUND" )
  else()
    list( GET SHARED_LIBRARIES 0 ${target} )
  endif()
endmacro()


#####################################################################################
# Macro for copying shared files (libraries) to output directory
#
macro( _copy_files_to_target target fileList )
  if( WIN32 )
    foreach ( _file ${fileList} )
      if( EXISTS "${_file}" )
        add_custom_command(
          TARGET ${target} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${_file}
            $<TARGET_FILE_DIR:${target}>
            VERBATIM
        )
      endif()
    endforeach()
  endif()
endmacro()

#####################################################################################
# OpenGL libraries
#
find_package(OpenGL)

#####################################################################################
# libcommon (Library providing shared functionality)
#
add_subdirectory(common)

#####################################################################################
set(IGNORED_DIRECTORIES "build_all" "common" "downloaded_resources" "__*" "TiledDeferredShading")

# Process PGR2 projects
FILE (GLOB samples "./*")
FOREACH( sample ${samples} )
  if( IS_DIRECTORY ${sample} )
    if( EXISTS ${sample}/CMakeLists.txt )
      # extract the name from the path
      string( REGEX REPLACE "^.*/([^/]*)$" "\\1" SAMPLE_NAME ${sample} )
      if( (NOT (${SAMPLE_NAME} MATCHES "^__")) AND NOT ${SAMPLE_NAME} IN_LIST IGNORED_DIRECTORIES)
        # build the option to include the project or not
        set( "BUILD_${SAMPLE_NAME}" TRUE CACHE BOOL "Include the ${SAMPLE_NAME} sample" )
        # add the project
        if( BUILD_${SAMPLE_NAME})
          add_subdirectory(./${SAMPLE_NAME})
        endif()
      endif()
    endif()
  endif()
ENDFOREACH( sample ${samples} )


#####################################################################################
# Print cmake configuration info
#
Message( STATUS "BASE_PATH = ${BASE_PATH}" )
Message( STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}" )
