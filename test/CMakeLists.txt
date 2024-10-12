cmake_minimum_required(VERSION 3.22.1)
project(godot_midi_test VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

#########################
##### Setup Doctest #####
#########################
# Pull in doctest repository.  Useful for pulling in documentation and updates.
include(ExternalProject)
find_package(Git REQUIRED)

ExternalProject_Add(
    doctest
    PREFIX ${CMAKE_BINARY_DIR}/doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    TIMEOUT 10
    UPDATE_COMMAND ${GIT_EXECUTABLE} pull
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)

# Expose required variable (DOCTEST_INCLUDE_DIR) to parent scope
ExternalProject_Get_Property(doctest source_dir)
set(DOCTEST_INCLUDE_DIR ${source_dir}/doctest CACHE INTERNAL "Path to include folder for doctest")

# Add doctest globally
include_directories(${DOCTEST_INCLUDE_DIR})

##############################
##### Find library files #####
##############################
set(godot_midi_DIR ${CMAKE_SOURCE_DIR}/../game/addons/godot_midi)
set(godot_midi_SRC ${CMAKE_SOURCE_DIR}/../extension)
set(godot_cpp_DIR ${CMAKE_SOURCE_DIR}/../godot-cpp)

if (UNIX)
    if (CMAKE_SIZEOF_VOID_P EQUAL 4)
        find_library(GodotCPP_LIBRARY
            NAMES libgodot-cpp.linux.template_debug.x86_32.so
            PATHS ${godot_cpp_DIR}/bin
        )
        find_library(GodotMidi_LIBRARY
            NAMES libgodotmidi.linux.template_debug.x86_32.so
            PATHS ${godot_midi_DIR}/bin/linux
        )
    else()
        find_library(GodotCPP_LIBRARY
            NAMES libgodot-cpp.linux.template_debug.x86_64.so
            PATHS ${godot_cpp_DIR}/bin
        )
        find_library(GodotMidi_LIBRARY
            NAMES libgodotmidi.linux.template_debug.x86_64.so
            PATHS ${godot_midi_DIR}/bin/linux
        )
    endif()
elseif (WIN32)
    if (CMAKE_SIZEOF_VOID_P EQUAL 4)
        find_library(GodotCPP_LIBRARY
            NAMES libgodot-cpp.windows.template_debug.x86_32.lib
            PATHS ${godot_cpp_DIR}/bin
        )
        find_library(GodotMidi_LIBRARY
            NAMES godotmidi.windows.template_debug.x86_32.lib
            PATHS ${godot_midi_DIR}/bin/windows
        )
        find_file(GodotMidi_DLL
            NAMES godotmidi.windows.template_debug.x86_32.dll
            PATHS ${godot_midi_DIR}/bin/windows
        )
    else()
        find_library(GodotCPP_LIBRARY
            NAMES libgodot-cpp.windows.template_debug.x86_64.lib
            PATHS ${godot_cpp_DIR}/bin
        )
        find_library(GodotMidi_LIBRARY
            NAMES godotmidi.windows.template_debug.x86_64.lib
            PATHS ${godot_midi_DIR}/bin/windows
        )
        find_file(GodotMidi_DLL
            NAMES godotmidi.windows.template_debug.x86_64.dll
            PATHS ${godot_midi_DIR}/bin/windows
        )
    endif()
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

if (GodotCPP_LIBRARY)
    message(STATUS "Godot CPP library found: ${GodotCPP_LIBRARY}")
else()
    message(FATAL_ERROR "Godot CPP library not found.  Build Godot CPP first.")
endif()

if (GodotMidi_LIBRARY)
    message(STATUS "GodotMidi library found: ${GodotMidi_LIBRARY}")
else()
    message(FATAL_ERROR "GodotMidi library was not found.  Build GodotMidi first.")
endif()

###########################
##### Setup Godot CPP #####
###########################
add_library(Godot_CPP STATIC IMPORTED)
set_target_properties(Godot_CPP PROPERTIES IMPORTED_LOCATION ${GodotCPP_LIBRARY})
target_include_directories(Godot_CPP
    INTERFACE ${godot_cpp_DIR}/gdextension/
    INTERFACE ${godot_cpp_DIR}/gen/include
    INTERFACE ${godot_cpp_DIR}/include
)

include_directories(${godot_cpp_dir})

###########################
##### Setup GodotMidi #####
###########################
add_library(GodotMidi SHARED IMPORTED)
set_target_properties(GodotMidi PROPERTIES IMPORTED_LOCATION ${GodotMidi_LIBRARY})
set_target_properties(GodotMidi PROPERTIES IMPORTED_IMPLIB ${GodotMidi_DLL})
target_include_directories(GodotMidi INTERFACE ${gdexample_DIR}/src)

add_dependencies(GodotMidi Godot_CPP)
include_directories(${gdexample_DIR}/src)

get_target_property(GDEXAMPLE_INCLUDE_DIRS GodotMidi INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "GodotMidi Include Directories: ${GDEXAMPLE_INCLUDE_DIRS}")

#############################
##### Build test binary #####
#############################
add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/test_godo_midi.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME}
    PUBLIC GodotMidi
    PUBLIC Godot_CPP
)

#####################
##### Run tests #####
#####################
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Running tests..."
    COMMAND ${CMAKE_CTEST_COMMAND} --build-config $<CONFIG> --output-on-failure -C $<CONFIG>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)