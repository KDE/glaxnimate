# SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(SOURCES
app/application.cpp
app/settings/settings_group.cpp
app/settings/settings.cpp
app/scripting/script_engine.cpp
app/log/logger.cpp
app/log/log_model.cpp
app/cli.cpp
)

add_library(${PROJECT_SLUG} OBJECT ${SOURCES})
target_link_libraries(${PROJECT_SLUG} PUBLIC Qt${QT_VERSION_MAJOR}::Widgets QtColorWidgets)
target_include_directories(${PROJECT_SLUG} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
kde_target_enable_exceptions(${PROJECT_SLUG} PUBLIC)
