# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

project(GlaxnimateWasm LANGUAGES CXX)

# C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE "Release")

# Qt
find_package(Qt6 COMPONENTS Core Gui Xml Network REQUIRED)
set(CMAKE_AUTOMOC ON)

# Glaxnimate lib minimal options
set(GLAXNIMATE_CORE_MINIMAL ON)
add_subdirectory(../core  ${CMAKE_CURRENT_BINARY_DIR}/lib)

# Defining the target

add_executable(glaxnimate_renderer wasm.cpp)
target_link_libraries(glaxnimate_renderer Glaxnimate::Core)
target_link_options(glaxnimate_renderer PRIVATE
    --bind
    -sALLOW_MEMORY_GROWTH=1
    -sPTHREAD_POOL_SIZE=4
    -sMODULARIZE=1
    --no-entry
    -sEXPORT_NAME="Glaxnimate"
    "SHELL:--extern-post-js ${CMAKE_CURRENT_SOURCE_DIR}/custom.js"
)
set_property(
    TARGET glaxnimate_renderer
    APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/custom.js"
)


# source "$HOME/src/emsdk/emsdk_env.sh"
# -DCMAKE_TOOLCHAIN_FILE=$HOME/Qt/6.9.3/wasm_singlethread/lib/cmake/Qt6/qt.toolchain.cmake
# -DQT_HOST_PATH=$HOME/Qt/6.9.3/gcc_64
# -DCMAKE_CXX_FLAGS="-matomics -mbulk-memory"
# -DCMAKE_EXE_LINKER_FLAGS="-lembind"
