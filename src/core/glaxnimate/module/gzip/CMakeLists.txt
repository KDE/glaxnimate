# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set(SOURCES
    gzip-zlib.cpp
    gzip.cpp
    gzip_module.cpp
    svgz_format.cpp
    tgs_format.cpp
)

add_library(GlaxnimateGzip OBJECT ${SOURCES})

if ( EMSCRIPTEN )
    target_compile_options(GlaxnimateGzip PUBLIC "-sUSE_ZLIB=1")
    target_link_options(GlaxnimateGzip PUBLIC "-sUSE_ZLIB=1")
else()
    find_package(ZLIB REQUIRED)
    target_link_libraries(GlaxnimateGzip PUBLIC ZLIB::ZLIB)
endif()

glaxnimate_enable_exceptions(GlaxnimateGzip PUBLIC)

