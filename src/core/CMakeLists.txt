# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set(SOURCES

glaxnimate/app_info.cpp

glaxnimate/command/structure_commands.cpp
glaxnimate/command/shape_commands.cpp
glaxnimate/command/animation_commands.cpp
glaxnimate/command/clipboard.cpp

glaxnimate/io/base.cpp
glaxnimate/io/binary_stream.cpp
glaxnimate/io/utils.cpp
glaxnimate/io/glaxnimate/glaxnimate_format.cpp
glaxnimate/io/glaxnimate/glaxnimate_importer.cpp
glaxnimate/io/glaxnimate/glaxnimate_mime.cpp
glaxnimate/io/glaxnimate/glaxnimate_html_format.cpp
glaxnimate/io/lottie/cbor_write_json.cpp
glaxnimate/io/lottie/lottie_format.cpp
glaxnimate/io/lottie/lottie_html_format.cpp
glaxnimate/io/lottie/validation.cpp
glaxnimate/io/mime/mime_serializer.cpp
glaxnimate/io/raster/raster_format.cpp
glaxnimate/io/raster/spritesheet_format.cpp
glaxnimate/io/svg/detail.cpp
glaxnimate/io/svg/svg_format.cpp
glaxnimate/io/svg/svg_parser.cpp
glaxnimate/io/svg/svg_renderer.cpp


glaxnimate/log/logger.cpp

glaxnimate/math/geom.cpp
glaxnimate/math/polynomial.cpp
glaxnimate/math/ellipse_solver.cpp
glaxnimate/math/bezier/bezier.cpp
glaxnimate/math/bezier/point.cpp
glaxnimate/math/bezier/operations.cpp
glaxnimate/math/bezier/cubic_struts.cpp
glaxnimate/math/bezier/meta.cpp
glaxnimate/math/bezier/bezier_length.cpp

glaxnimate/model/document.cpp
glaxnimate/model/document_node.cpp
glaxnimate/model/object.cpp
glaxnimate/model/transform.cpp
glaxnimate/model/factory.cpp
glaxnimate/model/animation_container.cpp
glaxnimate/model/stretchable_time.cpp
glaxnimate/model/comp_graph.cpp
glaxnimate/model/mask_settings.cpp
glaxnimate/model/visitor.cpp
glaxnimate/model/custom_font.cpp

glaxnimate/model/animation/keyframe_transition.cpp
glaxnimate/model/animation/keyframe_base.cpp
glaxnimate/model/animation/animatable.cpp
glaxnimate/model/animation/animatable_path.cpp
glaxnimate/model/animation/animatable_base.cpp
glaxnimate/model/animation/meta_animatable.cpp
glaxnimate/model/property/property.cpp
glaxnimate/model/property/reference_property.cpp
glaxnimate/model/property/option_list_property.cpp

glaxnimate/model/assets/assets.cpp
glaxnimate/model/assets/brush_style.cpp
glaxnimate/model/assets/named_color.cpp
glaxnimate/model/assets/bitmap.cpp
glaxnimate/model/assets/gradient.cpp
glaxnimate/model/assets/asset_base.cpp
glaxnimate/model/assets/asset.cpp
glaxnimate/model/assets/composition.cpp
glaxnimate/model/assets/embedded_font.cpp
glaxnimate/model/assets/network_downloader.cpp

glaxnimate/model/shapes/shape.cpp

glaxnimate/model/shapes/style/fill.cpp
glaxnimate/model/shapes/style/stroke.cpp
glaxnimate/model/shapes/style/styler.cpp

glaxnimate/model/shapes/shapes/rect.cpp
glaxnimate/model/shapes/shapes/ellipse.cpp
glaxnimate/model/shapes/shapes/path.cpp
glaxnimate/model/shapes/shapes/polystar.cpp
glaxnimate/model/shapes/shapes/text.cpp

glaxnimate/model/shapes/composable/group.cpp
glaxnimate/model/shapes/composable/layer.cpp
glaxnimate/model/shapes/composable/image.cpp
glaxnimate/model/shapes/composable/precomp_layer.cpp
glaxnimate/model/shapes/composable/composable.cpp

glaxnimate/model/shapes/modifiers/repeater.cpp
glaxnimate/model/shapes/modifiers/trim.cpp
glaxnimate/model/shapes/modifiers/inflate_deflate.cpp
glaxnimate/model/shapes/modifiers/path_modifier.cpp
glaxnimate/model/shapes/modifiers/round_corners.cpp
glaxnimate/model/shapes/modifiers/offset_path.cpp
glaxnimate/model/shapes/modifiers/zig_zag.cpp

glaxnimate/renderer/renderer.cpp

glaxnimate/settings/settings_group.cpp


glaxnimate/utils/data_paths.cpp
glaxnimate/module/module.cpp
)


set(GLAXNIMATE_CORE_KDE ON)

if ( GLAXNIMATE_CORE_MINIMAL )
    set(GLAXNIMATE_CORE_KDE OFF)

    set(RENDERER_LIBRARIES thorvg)
endif()


add_library(GlaxnimateCore STATIC)
add_library(Glaxnimate::Core ALIAS GlaxnimateCore)

add_library(GlaxnimateCoreBase OBJECT)
target_link_libraries(GlaxnimateCore PUBLIC GlaxnimateCoreBase)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/glaxnimate/application_info_generated.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/glaxnimate/application_info_generated.hpp
)

# KDE provides gzip support
if ( GLAXNIMATE_CORE_KDE )
    target_compile_definitions(GlaxnimateCoreBase PUBLIC -DGLAXNIMATE_CORE_KDE)

    kde_target_enable_exceptions(GlaxnimateCoreBase PUBLIC)
    target_link_libraries(GlaxnimateCoreBase PUBLIC
        KF${KF_MAJOR}::CoreAddons
        KF${KF_MAJOR}::Archive
        KF${KF_MAJOR}::I18n
    )
else()
    message(STATUS "Building Glaxnimate core without KDE")
endif()

# Renderer
add_subdirectory(../../external/thorvg/ ${CMAKE_CURRENT_BINARY_DIR}/thorvg EXCLUDE_FROM_ALL)
set(RENDERER_LIBRARIES thorvg)
target_link_libraries(GlaxnimateCoreBase PUBLIC ${RENDERER_LIBRARIES})

# Common

target_include_directories(GlaxnimateCoreBase PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(GlaxnimateCoreBase PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(GlaxnimateCoreBase PUBLIC
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Gui # Needed for QColor / QUndoCommand
    Qt${QT_MAJOR_VERSION}::Xml
    Qt${QT_MAJOR_VERSION}::Network
    ${Potrace_LIBRARIES}
)
set_property(TARGET GlaxnimateCoreBase APPEND PROPERTY AUTOMOC_MACRO_NAMES "GLAXNIMATE_OBJECT")
target_include_directories(GlaxnimateCoreBase PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)


target_sources(GlaxnimateCoreBase PRIVATE ${SOURCES})

# Modules
set(GLAXNIMATE_MODULES_)

macro(glaxnimate_module MODULE DESCRIPTION DEFAULT)
    string(TOUPPER ${MODULE} MODULE_UPPER)
    string(TOLOWER ${MODULE} MODULE_LOWER)
    option(GLAXNIMATE_${MODULE_UPPER}_ENABLED ${DESCRIPTION} ${EXTRA_DEFAULT})

    if ( GLAXNIMATE_${MODULE_UPPER}_ENABLED )
        add_subdirectory(glaxnimate/module/${MODULE_LOWER})
        target_link_libraries(Glaxnimate${MODULE} PUBLIC
            Qt${QT_MAJOR_VERSION}::Core
            Qt${QT_MAJOR_VERSION}::Gui
            Qt${QT_MAJOR_VERSION}::Network
            GlaxnimateCoreBase
        )
        list(APPEND GLAXNIMATE_MODULES_ ${MODULE})
    endif()
endmacro()

if ( NOT DEFINED EXTRA_DEFAULT )
    set(EXTRA_DEFAULT OFF)
endif()

glaxnimate_module("ThorVG" "Default renderer" ON)
glaxnimate_module("Gzip" "Support for gzip-compressed formats" ON)
glaxnimate_module("ExtraFormats" "Experimental file format support" ON)
glaxnimate_module("Video" "Support for video format I/O" ${EXTRA_DEFAULT})
glaxnimate_module("Cairo" "Support for format I/O using cairo" OFF)

set(GLAXNIMATE_MODULES ${GLAXNIMATE_MODULES_} PARENT_SCOPE)
message(STATUS "Core Modules Enabled:")
set(GLAXNIMATE_MODULES_INCLUDES)
set(GLAXNIMATE_MODULES_REGISTER)
foreach(MODULE ${GLAXNIMATE_MODULES_})
    message(STATUS "    ${MODULE}")
    string(TOLOWER ${MODULE} MODULE_LOWER)
    string(CONCAT GLAXNIMATE_MODULES_INCLUDES ${GLAXNIMATE_MODULES_INCLUDES} "#include \"glaxnimate/module/${MODULE_LOWER}/${MODULE_LOWER}_module.hpp\"\n")
    string(CONCAT GLAXNIMATE_MODULES_REGISTER "${GLAXNIMATE_MODULES_REGISTER}registry.install<glaxnimate::${MODULE_LOWER}::Module>();\n")
    target_link_libraries(GlaxnimateCore PUBLIC Glaxnimate${MODULE})
endforeach()
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/glaxnimate/module/module_autoregister.in.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/module_autoregister.cpp
)
target_sources(GlaxnimateCore PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/module_autoregister.cpp)
