# SPDX-License-Identifier: GPL-2.0-or-later
#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  taken from github.com/osmcode/osm-gis-export
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


#-----------------------------------------------------------------------------
#
#  Project version
#
#-----------------------------------------------------------------------------

project(polygon-to-tile-list)

set(TILELISTMAKER_VERSION_MAJOR 0)
set(TILELISTMAKER_VERSION_MINOR 0)
set(TILELISTMAKER_VERSION_PATCH 1)

set(TILELISTMAKER_VERSION
    ${TILELISTMAKER_VERSION_MAJOR}.${TILELISTMAKER_VERSION_MINOR}.${TILELISTMAKER_VERSION_PATCH})

set(AUTHOR "Michael Reichert <michael.reichert@geofabrik.de>")

#-----------------------------------------------------------------------------
#
#  Find external dependencies
#
#-----------------------------------------------------------------------------

#----------------------------------------------------------------------
# geos
find_path(GEOS_INCLUDE_DIR geos_c.h)
find_library(GEOS_LIBRARY NAMES geos)

if(GEOS_INCLUDE_DIR AND GEOS_LIBRARY)
    SET(GEOS_FOUND 1)
    include_directories(SYSTEM ${GEOS_INCLUDE_DIR})
else()
    message(WARNING "GEOS library is required but not found, please install it or configure the paths.")
endif()

#----------------------------------------------------------------------
# GDAL
find_package(GDAL)

if(GDAL_INCLUDE_DIR AND GDAL_LIBRARY)
    SET(GDAL_FOUND 1)
    include_directories(SYSTEM ${GDAL_INCLUDE_DIR})
else()
    message(WARNING "GDAL library is required but not found, please install it or configure the paths.")
endif()

#-----------------------------------------------------------------------------
#
#  Decide which C++ version to use (Minimum/default: C++11).
#
#-----------------------------------------------------------------------------
if(NOT MSVC)
    if(NOT USE_CPP_VERSION)
        set(USE_CPP_VERSION c++11)
    endif()
    message(STATUS "Use C++ version: ${USE_CPP_VERSION}")
    # following only available from cmake 2.8.12:
    #   add_compile_options(-std=${USE_CPP_VERSION})
    # so using this instead:
    add_definitions(-std=${USE_CPP_VERSION})
endif()


#-----------------------------------------------------------------------------
#
#  Compiler and Linker flags
#
#-----------------------------------------------------------------------------
set(USUAL_COMPILE_OPTIONS "-O3 -g")

set(CMAKE_CXX_FLAGS_DEV "${USUAL_COMPILE_OPTIONS}"
    CACHE STRING "Flags used by the compiler during developer builds."
    FORCE)

set(CMAKE_EXE_LINKER_FLAGS_DEV ""
    CACHE STRING "Flags used by the linker during developer builds."
    FORCE)
mark_as_advanced(
    CMAKE_CXX_FLAGS_DEV
    CMAKE_EXE_LINKER_FLAGS_DEV
)

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${USUAL_COMPILE_OPTIONS}"
    CACHE STRING "Flags used by the compiler during RELWITHDEBINFO builds."
    FORCE)


#-----------------------------------------------------------------------------
#
#  Build Type
#
#-----------------------------------------------------------------------------
set(CMAKE_CONFIGURATION_TYPES "Debug Release RelWithDebInfo MinSizeRel Dev")

# In 'Dev' mode: compile with very strict warnings and turn them into errors.
if(CMAKE_BUILD_TYPE STREQUAL "Dev")
    if(MSVC)
        set(WARNING_OPTIONS "/W3 /wd4514" CACHE STRING "Recommended warning options")
    else()
        set(WARNING_OPTIONS "-Werror -fno-omit-frame-pointer -Wall -Wextra -pedantic -Wredundant-decls -Wdisabled-optimization -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wold-style-cast" CACHE STRING "Recommended warning options")
    endif()
endif()

# Force RelWithDebInfo build type if none was given
if(CMAKE_BUILD_TYPE)
    set(build_type ${CMAKE_BUILD_TYPE})
else()
    set(build_type "RelWithDebInfo")
endif()

set(CMAKE_BUILD_TYPE ${build_type}
    CACHE STRING
    "Choose the type of build, options are: ${CMAKE_CONFIGURATION_TYPES}."
    FORCE)


#-----------------------------------------------------------------------------
#
#  Optional "cppcheck" target that checks C++ code
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for cppcheck")
find_program(CPPCHECK cppcheck)

if(CPPCHECK)
    message(STATUS "Looking for cppcheck - found")
    set(CPPCHECK_OPTIONS --enable=all)

    # cpp doesn't find system includes for some reason, suppress that report
    set(CPPCHECK_OPTIONS ${CPPCHECK_OPTIONS} --suppress=missingIncludeSystem)

    file(GLOB ALL_CODE *.cpp)

    set(CPPCHECK_FILES ${ALL_CODE})

    add_custom_target(cppcheck
        ${CPPCHECK}
        --std=c++11 ${CPPCHECK_OPTIONS}
        ${CPPCHECK_FILES}
    )
else()
    message(STATUS "Looking for cppcheck - not found")
    message(STATUS "  Build target 'cppcheck' will not be available.")
endif(CPPCHECK)


#-----------------------------------------------------------------------------

add_definitions(${WARNING_OPTIONS})

add_subdirectory(src)


#-----------------------------------------------------------------------------
