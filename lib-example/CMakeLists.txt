# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

# Simple example of using Glaxnimate as a plain renderer

cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

project(GlaxnimateLibExample LANGUAGES CXX)

# C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_BUILD_TYPE "Debug")

# Qt
find_package(Qt6 COMPONENTS Core Gui Xml Network REQUIRED)
set(CMAKE_AUTOMOC ON)

# Glaxnimate lib minimal options
set(GLAXNIMATE_ROOT ../)
set(GLAXNIMATE_CORE_MINIMAL ON)
add_subdirectory(${GLAXNIMATE_ROOT}/src/core  ${CMAKE_CURRENT_BINARY_DIR}/lib)
include_directories(${GLAXNIMATE_ROOT}/src/core)

# Defining the target

add_executable(lib_example main.cpp)
target_link_libraries(lib_example glaxnimate_lib)
