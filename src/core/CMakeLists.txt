# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set(SOURCES

app_info.cpp

command/structure_commands.cpp
command/shape_commands.cpp
command/animation_commands.cpp

io/base.cpp
io/binary_stream.cpp
io/utils.cpp
io/glaxnimate/glaxnimate_format.cpp
io/glaxnimate/glaxnimate_importer.cpp
io/glaxnimate/glaxnimate_mime.cpp
io/lottie/cbor_write_json.cpp
io/lottie/lottie_format.cpp
io/lottie/lottie_html_format.cpp
io/lottie/validation.cpp
io/mime/mime_serializer.cpp
io/raster/raster_format.cpp
io/raster/spritesheet_format.cpp
io/rive/rive_format.cpp
io/rive/rive_html_format.cpp
io/rive/rive_loader.cpp
io/rive/rive_serializer.cpp
io/rive/type_def.cpp
io/rive/type_system.cpp
io/svg/detail.cpp
io/svg/svg_format.cpp
io/svg/svg_parser.cpp
io/svg/svg_renderer.cpp
io/avd/avd_parser.cpp
io/avd/avd_format.cpp
io/avd/avd_renderer.cpp
io/aep/aep_format.cpp
io/aep/aep_loader.cpp
io/aep/string_decoder.cpp
io/aep/gradient_xml.cpp
io/io_registry.cpp

math/geom.cpp
math/polynomial.cpp
math/ellipse_solver.cpp
math/bezier/bezier.cpp
math/bezier/point.cpp
math/bezier/operations.cpp
math/bezier/cubic_struts.cpp
math/bezier/meta.cpp
math/bezier/bezier_length.cpp

model/document.cpp
model/document_node.cpp
model/object.cpp
model/transform.cpp
model/factory.cpp
model/animation_container.cpp
model/stretchable_time.cpp
model/comp_graph.cpp
model/mask_settings.cpp
model/visitor.cpp
model/custom_font.cpp

model/animation/keyframe_transition.cpp
model/animation/animatable.cpp
model/animation/animatable_path.cpp
model/property/property.cpp
model/property/reference_property.cpp
model/property/option_list_property.cpp

model/assets/assets.cpp
model/assets/brush_style.cpp
model/assets/named_color.cpp
model/assets/bitmap.cpp
model/assets/gradient.cpp
model/assets/asset_base.cpp
model/assets/asset.cpp
model/assets/composition.cpp
model/assets/embedded_font.cpp
model/assets/network_downloader.cpp

model/shapes/shape.cpp
model/shapes/fill.cpp
model/shapes/rect.cpp
model/shapes/group.cpp
model/shapes/ellipse.cpp
model/shapes/path.cpp
model/shapes/stroke.cpp
model/shapes/polystar.cpp
model/shapes/styler.cpp
model/shapes/layer.cpp
model/shapes/image.cpp
model/shapes/precomp_layer.cpp
model/shapes/text.cpp
model/shapes/repeater.cpp
model/shapes/trim.cpp
model/shapes/inflate_deflate.cpp
model/shapes/path_modifier.cpp
model/shapes/round_corners.cpp
model/shapes/offset_path.cpp
model/shapes/zig_zag.cpp

renderer/renderer.cpp

utils/gzip.cpp
)


set(GLAXNIMATE_CORE_KDE ON)

if ( GLAXNIMATE_CORE_MINIMAL )
    set(GLAXNIMATE_VIDEO_ENABLED OFF)
    set(PYTHON_SCRIPTING_ENABLED OFF)
    set(GLAXNIMATE_CORE_KDE OFF)
    set(LIB_NAME_CORE glaxnimate_lib)
    add_definitions(-DWITHOUT_POTRACE)

    add_subdirectory(../../external/thorvg/ ${CMAKE_CURRENT_BINARY_DIR}/thorvg)
    set(RENDERER_LIBRARIES thorvg)
    list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../external/QtAppSetup/src/app/log/logger.cpp
        # ${CMAKE_CURRENT_SOURCE_DIR}/../../external/QtAppSetup/src/app/settings/settings.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../external/QtAppSetup/src/app/settings/settings_group.cpp
    )

else()
    list(APPEND SOURCES
        plugin/plugin.cpp
        plugin/action.cpp
        plugin/io.cpp

        utils/quantize.cpp
        utils/trace.cpp
        utils/trace_wrapper.cpp
    )
endif()


add_library(${LIB_NAME_CORE} STATIC)


# Video
if ( NOT ANDROID )
    set(GLAXNIMATE_VIDEO_DEFAULT ON)
else()
    set(GLAXNIMATE_VIDEO_DEFAULT OFF)
endif()

option(GLAXNIMATE_VIDEO_ENABLED "Toggles support for video format I/O" ${GLAXNIMATE_VIDEO_DEFAULT})

if ( GLAXNIMATE_VIDEO_ENABLED )
    target_compile_definitions(${LIB_NAME_CORE} PUBLIC -DGLAXNIMATE_VIDEO_ENABLED)
    list(APPEND SOURCES
        io/video/video_format.cpp
    )
    find_package(Libav COMPONENTS codec util format swscale REQUIRED)
    include_directories(${Libav_INCLUDE_DIRS})
    target_link_libraries(${LIB_NAME_CORE} PUBLIC ${Libav_LIBRARIES} )
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/application_info_generated.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/application_info_generated.hpp
)

# Potrace
if ( GLAXNIMATE_POTRACE_ENABLED )
    target_compile_definitions(${LIB_NAME_CORE} PUBLIC -DGLAXNIMATE_POTRACE_ENABLED)
    if ( GLAXNIMATE_SYSTEM_POTRACE )
        target_include_directories(${LIB_NAME_CORE} PUBLIC ${Potrace_INCLUDE_DIRS})
    endif()
endif()

# KDE provides gzip support
if ( GLAXNIMATE_CORE_KDE )
    list(APPEND SOURCES
        io/lottie/tgs_format.cpp
    )
    target_compile_definitions(${LIB_NAME_CORE} PUBLIC -DGLAXNIMATE_CORE_KDE)

    kde_target_enable_exceptions(${LIB_NAME_CORE} PUBLIC)
    target_link_libraries(${LIB_NAME_CORE} PUBLIC
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
target_link_libraries(${LIB_NAME_CORE} PUBLIC ${RENDERER_LIBRARIES})

# Common

target_include_directories(${LIB_NAME_CORE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${LIB_NAME_CORE} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${LIB_NAME_CORE} PUBLIC
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Gui # Needed for QColor / QUndoCommand
    Qt${QT_MAJOR_VERSION}::Xml
    Qt${QT_MAJOR_VERSION}::Network
    ${Potrace_LIBRARIES}
)
set_property(TARGET ${LIB_NAME_CORE} APPEND PROPERTY AUTOMOC_MACRO_NAMES "GLAXNIMATE_OBJECT")
target_include_directories(${LIB_NAME_CORE} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> ${CMAKE_CURRENT_SOURCE_DIR}/../../external/QtAppSetup/src)

if ( NOT GLAXNIMATE_CORE_MINIMAL )
    target_link_libraries(${LIB_NAME_CORE} PUBLIC QtAppSetup)
endif ()

target_sources(${LIB_NAME_CORE} PRIVATE ${SOURCES})
