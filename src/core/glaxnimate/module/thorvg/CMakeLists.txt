# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set(SOURCES
    thorvg_module.cpp
)

add_library(GlaxnimateThorVG OBJECT ${SOURCES})
kde_target_enable_exceptions(GlaxnimateThorVG PUBLIC)
