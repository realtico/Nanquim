# FindSDL2_gfx.cmake
# -------------------
# Find the SDL2_gfx library (primitives, rotozoom, etc.)
#
# This module defines:
#   SDL2_gfx_FOUND        - True if SDL2_gfx was found
#   SDL2_gfx_INCLUDE_DIRS - Include directories
#   SDL2_gfx_LIBRARIES    - Libraries to link
#
# And the imported target:
#   SDL2_gfx::SDL2_gfx

if(TARGET SDL2_gfx::SDL2_gfx)
    return()
endif()

# Try pkg-config first (Linux, macOS with Homebrew)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(_SDL2_GFX QUIET SDL2_gfx)
endif()

# Find include directory
find_path(SDL2_GFX_INCLUDE_DIR
    NAMES SDL2_gfxPrimitives.h
    HINTS
        ${_SDL2_GFX_INCLUDE_DIRS}
        ENV SDL2DIR
        ENV SDL2GFXDIR
    PATH_SUFFIXES SDL2 include/SDL2 include
)

# Find library
find_library(SDL2_GFX_LIBRARY
    NAMES SDL2_gfx SDL2_gfx-1.0
    HINTS
        ${_SDL2_GFX_LIBRARY_DIRS}
        ENV SDL2DIR
        ENV SDL2GFXDIR
    PATH_SUFFIXES lib lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2_gfx
    REQUIRED_VARS SDL2_GFX_LIBRARY SDL2_GFX_INCLUDE_DIR
)

if(SDL2_gfx_FOUND)
    set(SDL2_gfx_INCLUDE_DIRS ${SDL2_GFX_INCLUDE_DIR})
    set(SDL2_gfx_LIBRARIES ${SDL2_GFX_LIBRARY})

    add_library(SDL2_gfx::SDL2_gfx UNKNOWN IMPORTED)
    set_target_properties(SDL2_gfx::SDL2_gfx PROPERTIES
        IMPORTED_LOCATION "${SDL2_GFX_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_GFX_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(SDL2_GFX_INCLUDE_DIR SDL2_GFX_LIBRARY)
