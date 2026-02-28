# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause
file(
    GLOB SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/common/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/renderer/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/renderer/sw_engine/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/renderer/gl_engine/*.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/loaders/raw/*.cpp
)


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h ${CMAKE_CURRENT_BINARY_DIR}/config.h COPYONLY)
add_library(thorvg SHARED ${SOURCES})
target_include_directories(
    thorvg PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/renderer/sw_engine/
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/renderer/gl_engine/

    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/loaders/raw
)

find_package(OpenGL REQUIRED)
target_link_libraries(thorvg PUBLIC ${OPENGL_gl_LIBRARY})
# target_include_directories(thorvg PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

