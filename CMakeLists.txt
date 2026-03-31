cmake_minimum_required(VERSION 3.16)
project(Nanquim VERSION 0.2 LANGUAGES C)

# ---- C Standard ----
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ---- Options ----
option(BUILD_SHARED_LIBS "Build Nanquim as a shared library" ON)
option(NQ_BUILD_EXAMPLES "Build example programs" ON)

# ---- Find Dependencies ----
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_gfx REQUIRED)

# ---- Normalize SDL2 targets ----
# SDL2 target names vary across platforms and versions.
# CMake config shipped with SDL2 >= 2.0.12 provides SDL2::SDL2.
# Older or distro-provided FindSDL2 may only set SDL2_LIBRARIES/SDL2_INCLUDE_DIRS.
if(NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 UNKNOWN IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        IMPORTED_LOCATION "${SDL2_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
    )
endif()

# SDL2_image target normalization
if(NOT TARGET SDL2_image::SDL2_image)
    add_library(SDL2_image::SDL2_image UNKNOWN IMPORTED)
    set_target_properties(SDL2_image::SDL2_image PROPERTIES
        IMPORTED_LOCATION "${SDL2_IMAGE_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_IMAGE_INCLUDE_DIRS}"
    )
endif()

# ---- Library Target ----
add_library(nanquim
    src/nq_core.c
    src/nq_draw.c
    src/nq_assets.c
    src/nq_input.c
    src/nq_camera.c
)

target_include_directories(nanquim
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(nanquim
    PUBLIC
        SDL2::SDL2
    PRIVATE
        SDL2_image::SDL2_image
        SDL2_gfx::SDL2_gfx
)

# Math library (Unix needs -lm, MSVC links automatically)
if(NOT MSVC)
    target_link_libraries(nanquim PRIVATE m)
endif()

# Platform-specific definitions
if(BUILD_SHARED_LIBS)
    target_compile_definitions(nanquim PRIVATE NQ_BUILD_DLL)
    if(NOT WIN32)
        target_compile_options(nanquim PRIVATE -fvisibility=hidden)
    endif()
else()
    target_compile_definitions(nanquim PUBLIC NQ_STATIC)
endif()

# MSVC: enable M_PI and other POSIX math constants
if(MSVC)
    target_compile_definitions(nanquim PRIVATE _USE_MATH_DEFINES)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(nanquim PRIVATE /W4)
else()
    target_compile_options(nanquim PRIVATE -Wall -Wextra)
endif()

# Set output directories to match original layout
set_target_properties(nanquim PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# ---- Examples ----
if(NQ_BUILD_EXAMPLES)
    set(NQ_EXAMPLES
        diffraction_sim
        fourier_demo
        geometry_test
        input_demo
        polar_demo
        robot_parade
        scale_demo
        static_test
        test_pattern
        time_test
        tracking_demo
    )

    foreach(example IN LISTS NQ_EXAMPLES)
        add_executable(${example} examples/${example}.c)
        target_link_libraries(${example} PRIVATE nanquim)

        # Link math library on Unix (MSVC links automatically)
        if(NOT MSVC)
            target_link_libraries(${example} PRIVATE m)
        endif()

        # MSVC: enable M_PI in examples that use math.h
        if(MSVC)
            target_compile_definitions(${example} PRIVATE _USE_MATH_DEFINES)
        endif()

        set_target_properties(${example} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
    endforeach()

    # Copy assets to build directory so examples can find them
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
         DESTINATION "${CMAKE_BINARY_DIR}")
endif()
