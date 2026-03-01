# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause
file(
    GLOB SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/common/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/renderer/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/renderer/sw_engine/*.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/loaders/raw/*.cpp
)


if ( OPENGL_ENABLED )
    set(THORVG_GL_RASTER_SUPPORT ON)
    set(THORVG_GL_TARGET_GL ON)

    file(
        GLOB GL_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/renderer/gl_engine/*.cpp
    )
    list(APPEND SOURCES ${GL_SOURCES})
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h ${CMAKE_CURRENT_BINARY_DIR}/config.h)
add_library(thorvg SHARED ${SOURCES})
target_include_directories(
    thorvg PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/renderer/sw_engine/
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/renderer/gl_engine/
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/thorvg/src/loaders/raw
)

if ( OPENGL_ENABLED )
    find_package(OpenGL REQUIRED)
    target_link_libraries(thorvg PUBLIC ${OPENGL_gl_LIBRARY})
endif()
