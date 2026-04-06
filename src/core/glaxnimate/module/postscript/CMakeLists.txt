# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set(SOURCES
    postscript_module.cpp
    ps_interpreter.cpp
    ps_value.cpp
    ps_gstate.cpp
)

add_library(GlaxnimatePostScript OBJECT ${SOURCES})
glaxnimate_enable_exceptions(GlaxnimatePostScript PUBLIC)
