# SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause
cmake_minimum_required (VERSION 3.13 FATAL_ERROR)
cmake_policy(SET CMP0076 NEW)

project(QtAppSetup VERSION 0.1.0 LANGUAGES CXX)
set(PROJECT_SLUG QtAppSetup)

# C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
find_package(Qt${QT_VERSION_MAJOR}Qml QUIET)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Submodules
add_subdirectory(src)
