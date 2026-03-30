# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set(SOURCES
    thorvg_module.cpp
)

add_library(GlaxnimateThorVG OBJECT ${SOURCES})
glaxnimate_enable_exceptions(GlaxnimateThorVG PUBLIC)

add_subdirectory(../../../../../external/thorvg/ ${CMAKE_CURRENT_BINARY_DIR}/thorvg EXCLUDE_FROM_ALL)
target_link_libraries(GlaxnimateThorVG PUBLIC thorvg)
