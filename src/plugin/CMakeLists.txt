# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set(SOURCES
    plugin.cpp
    action.cpp
    io.cpp
    script_engine.cpp
)

add_library(GlaxnimatePlugin OBJECT ${SOURCES})
add_library(Glaxnimate::Plugin ALIAS GlaxnimatePlugin)


if(GLAXNIMATE_PYTHON_ENABLED)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    message(STATUS "Python3 enabled")
    add_subdirectory(../../external/pybind11 ${CMAKE_CURRENT_BINARY_DIR}/pybind11)
    target_include_directories(GlaxnimatePlugin PUBLIC
        ${Python3_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../external/pybind11/include
    )
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/pybind11/include)
    add_compile_definitions(GLAXNIMATE_PYTHON_ENABLED)
    add_subdirectory(python)
    # target_sources(GlaxnimatePlugin PRIVATE ${GLAXNIMATE_PYTHON_SOURCES})
endif()

target_link_libraries(
    GlaxnimatePlugin PUBLIC
    QtColorWidgets
    Glaxnimate::Core
    Qt${QT_MAJOR_VERSION}::Widgets
    KF${KF_MAJOR}::CoreAddons
    KF${KF_MAJOR}::I18n
)
kde_target_enable_exceptions(GlaxnimatePlugin PUBLIC)
