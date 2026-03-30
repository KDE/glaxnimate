# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set(SOURCES
    extraformats_module.cpp

    avd/avd_parser.cpp
    avd/avd_format.cpp
    avd/avd_renderer.cpp

    aep/aep_format.cpp
    aep/aep_loader.cpp
    aep/string_decoder.cpp
    aep/gradient_xml.cpp

    rive/rive_format.cpp
    rive/rive_html_format.cpp
    rive/rive_loader.cpp
    rive/rive_serializer.cpp
    rive/type_def.cpp
    rive/type_system.cpp
)

add_library(GlaxnimateExtraFormats OBJECT ${SOURCES})
glaxnimate_enable_exceptions(GlaxnimateExtraFormats PUBLIC)
