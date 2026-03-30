# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set(SOURCES
    postscript_format.cpp
    pdf_format.cpp
    cairo_module.cpp
)

add_library(GlaxnimateCairo OBJECT ${SOURCES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

target_link_libraries(GlaxnimateCairo PUBLIC ${CAIRO_LIBRARIES})

kde_target_enable_exceptions(GlaxnimateCairo PUBLIC)
