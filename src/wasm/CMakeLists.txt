# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required (VERSION 3.16 FATAL_ERROR)

project(GlaxnimateWasm VERSION 0.6.57 LANGUAGES CXX)

# C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_BUILD_TYPE "Release")

# Qt
find_package(Qt6 COMPONENTS Core Gui Xml Network REQUIRED)
set(CMAKE_AUTOMOC ON)

# Glaxnimate lib minimal options
set(GLAXNIMATE_CORE_MINIMAL ON)
add_subdirectory(../core  ${CMAKE_CURRENT_BINARY_DIR}/lib)

# Defining the target
add_executable(glaxnimate_wasm wasm.cpp)
target_link_libraries(glaxnimate_wasm Glaxnimate::Core)
target_link_options(glaxnimate_wasm PRIVATE
    --bind
    -sALLOW_MEMORY_GROWTH=1
    -sPTHREAD_POOL_SIZE=4
    -sMODULARIZE=1
    --no-entry
    -sEXPORT_NAME="Glaxnimate"
    "SHELL:--extern-post-js ${CMAKE_CURRENT_SOURCE_DIR}/custom.js"
)
set_target_properties(
    glaxnimate_wasm
    PROPERTIES
    OUTPUT_NAME "glaxnimate"
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/custom.js"
)

configure_file(example/example.html ${CMAKE_CURRENT_BINARY_DIR}/example.html COPYONLY)
configure_file(example/example.rawr ${CMAKE_CURRENT_BINARY_DIR}/example.rawr COPYONLY)
configure_file(Readme.md ${CMAKE_CURRENT_BINARY_DIR}/Readme.md COPYONLY)
# Make it easy to publish iff we're doing a release build
if ( CMAKE_BUILD_TYPE STREQUAL "Release" )
    configure_file(package.json ${CMAKE_CURRENT_BINARY_DIR}/package.json)
endif()


# source "$HOME/src/emsdk/emsdk_env.sh"
# cmake ../src/wasm
# -DCMAKE_TOOLCHAIN_FILE=$HOME/Qt/6.10.2/wasm_singlethread/lib/cmake/Qt6/qt.toolchain.cmake
# -DQT_HOST_PATH=$HOME/Qt/6.10.2/gcc_64
# -DCMAKE_CXX_FLAGS="-matomics -mbulk-memory"
# -DCMAKE_EXE_LINKER_FLAGS="-lembind"
# -DCMAKE_BUILD_TYPE=Release # or Debug

# In docker image:
# source /opt/emsdk/emsdk_env.sh
# cmake -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE -DQT_HOST_PATH=$QT_HOST_PATH -DCMAKE_CXX_FLAGS="-matomics -mbulk-memory" -DCMAKE_EXE_LINKER_FLAGS="-lembind" -DCMAKE_BUILD_TYPE=Release /glax/src/wasm
