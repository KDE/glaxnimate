# SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required (VERSION 3.16 FATAL_ERROR)
# cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0077 NEW)
cmake_policy(SET CMP0153 NEW)


project(Glaxnimate VERSION 0.5.80 LANGUAGES CXX C)
include(FeatureSummary)

set(PROJECT_SLUG "glaxnimate")
set(LIB_NAME_CORE "${PROJECT_SLUG}_core")
set(LIB_NAME_GUI "${PROJECT_SLUG}_gui")
set(LIB_NAME_PYTHON "${PROJECT_SLUG}_python_lib")
set(LIB_NAME_DESKTOP "${PROJECT_SLUG}_desktop")
set(DATA_INSTALL "share/${PROJECT_SLUG}/${PROJECT_SLUG}")
set(PROJECT_ID "org.kde.glaxnimate")

set(PROJECT_HOMEPAGE_URL "https://glaxnimate.org/")
set(URL_DOCS "https://docs.glaxnimate.org/")
set(URL_ISSUES "https://bugs.kde.org/enter_bug.cgi?product=glaxnimate")
set(URL_DONATE "https://glaxnimate.org/donate/")

# You can use it to change the url used by `make docs`
set(DOCS_SITE_URL ${URL_DOCS} CACHE STRING "")
set(DOCS_REPO_URL "https://gitlab.com/mattbas/glaxnimate")
set(DOCS_CHAT_URL "https://t.me/Glaxnimate")

set(PROJECT_DESCRIPTION "Simple vector animation program")
set(PROJECT_MANINTAINER_NAME "Mattia Basaglia")
set(PROJECT_MANINTAINER_EMAIL "glanimate@mattbas.org")
set(PROJECT_MANINTAINER "${PROJECT_MANINTAINER_NAME} <${PROJECT_MANINTAINER_EMAIL}>")

option(MOBILE_UI "Build the mobile user interface" OFF)

add_definitions(-DTRANSLATION_DOMAIN=\"glaxnimate\")

if (ANDROID)
    set(MOBILE_UI ON)
endif()

if(MOBILE_UI)
    message(STATUS "Using mobile UI")
    add_compile_definitions(MOBILE_UI)
else()
    message(STATUS "Using desktop UI")
endif()

# CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(misc)

# C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Git
set(VERSION_NAKED ${PROJECT_VERSION})
set(VERSION_SUFFIX "-git-" CACHE STRING "")

if (VERSION_SUFFIX STREQUAL "-git-" AND EXISTS ${CMAKE_SOURCE_DIR}/.git)
    # Probably a Git workspace; determine the revision.
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} log "-n" 1 "--pretty=format:%h"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_REVISION
        )
        message(STATUS "Git revision: ${GIT_REVISION}")
        set(PROJECT_VERSION "${PROJECT_VERSION}-git-${GIT_REVISION}")
    else()
        message(STATUS "Git revision could not be determined")
    endif()
elseif(VERSION_SUFFIX MATCHES "^\\..*")
    set(PROJECT_VERSION "${PROJECT_VERSION}${VERSION_SUFFIX}")
elseif(NOT VERSION_SUFFIX STREQUAL "")
    set(PROJECT_VERSION "${PROJECT_VERSION}+${VERSION_SUFFIX}")
endif()

message(STATUS "Building ${PROJECT_NAME} ${PROJECT_VERSION}")

# Snapcraft stuff here because we don't care if cmake fails on CI
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/deploy/snapcraft.in.yaml"
    "${CMAKE_CURRENT_BINARY_DIR}/snapcraft.yaml"
)
option(SNAP_IS_SUCH_A_PAIN "" OFF)
if ( SNAP_IS_SUCH_A_PAIN  )
    return()
endif()

# Fucking Windows being shit
if ( WIN32 AND MinGW )
    # Without this shit it gives a bunch of errors like this:
    # * referenced in section `.rdata' of * defined in discarded section `.gnu.linkonce.*`
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-lto")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-lto")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-lto")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(CMAKE_ENABLE_EXPORTS TRUE)
endif()


# Python
if ( NOT MOBILE_UI )
    set(PYTHON_SCRIPTING_ENABLED ON)
else()
    set(PYTHON_SCRIPTING_ENABLED OFF)
endif()

if ( PYTHON_SCRIPTING_ENABLED )
    add_definitions(-DPYTHON_SCRIPTING_ENABLED)
    find_program(PYTHON_PIP pip3)
    if(NOT PYTHON_PIP)
        find_program(PYTHON_PIP pip)
    endif()

    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    message(STATUS ${Python3_EXECUTABLE})

    set(PYBIND11_FINDPYTHON ON)
else()
    message(STATUS "Python Disabled")
endif()

# QT_MAJOR_VERSION is the common name in KDE apps and infrastructure.
# BUILD_WITH_QT6 is another thing on KDE infrastructure.
# However some third-party submodules use QT_VERSION_MAJOR so we need to set that too.
if (NOT DEFINED QT_MAJOR_VERSION)
    if(BUILD_WITH_QT6)
        set(QT_MAJOR_VERSION 6)
    else()
        set(QT_MAJOR_VERSION 5)
    endif()
endif()
set(QT_VERSION_MAJOR ${QT_MAJOR_VERSION})

# Qt
find_package(Qt${QT_MAJOR_VERSION} COMPONENTS Core Widgets Xml UiTools Concurrent Network Svg REQUIRED)
message(STATUS "Using Qt ${QT_MAJOR_VERSION}")

# KF
set(KF_DEP_VERSION "5.92.0")
find_package(ECM ${KF_DEP_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
if (QT_MAJOR_VERSION STREQUAL "6")
    include(ECMAddAndroidApk)
endif()
include(ECMAddAppIcon)

# TODO: The following definitons should be enabled
remove_definitions(
    -DQT_NO_CAST_FROM_ASCII
)

if (NOT DEFINED KF_MAJOR)
  set(KF_MAJOR ${QT_MAJOR_VERSION})
endif()

if (KF_MAJOR STREQUAL "6")
  set(KF_DEP_VERSION "5.240.0")
endif()

find_package(KF6BreezeIcons ${KF_DEP_VERSION} QUIET)

find_package(KF${KF_MAJOR} ${KF_DEP_VERSION}
    REQUIRED COMPONENTS
    I18n
    CoreAddons
    WidgetsAddons
    XmlGui
    Archive
    Completion
    IconThemes
    Config
)
if ( NOT ANDROID )
    find_package(KF${KF_MAJOR} ${KF_DEP_VERSION}
        REQUIRED COMPONENTS
        Crash
    )
endif()


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Potrace
option(GLAXNIMATE_SYSTEM_POTRACE "Use the system potrace libarary" ON)
if (NOT GLAXNIMATE_SYSTEM_POTRACE)
    message(STATUS "Using bundled potrace")
    set(CMAKE_C_COMPILE_OBJECT ${CMAKE_CXX_COMPILE_OBJECT})
    add_subdirectory(external/potrace/)
    set(Potrace_LIBRARIES potrace)
    include_directories(external/potrace/src/)
else()
    find_package(Potrace REQUIRED)
endif()

# Debug flags
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -pedantic -Wextra")
message(STATUS "${CMAKE_BUILD_TYPE} build")

# Renderer
option(OPENGL_ENABLED "Whether to enable OpenGL features" OFF)
if ( OPENGL_ENABLED )
    add_definitions(-DOPENGL_ENABLED)
endif()
add_subdirectory(external/thorvg/)
set(RENDERER_LIBRARIES thorvg)


if ( NOT ANDROID )
    set(GLAXNIMATE_VIDEO_DEFAULT ON)
else()
    set(GLAXNIMATE_VIDEO_DEFAULT OFF)
endif()

option(VIDEO_ENABLED "Toggles support for video format I/O" ${GLAXNIMATE_VIDEO_DEFAULT})

if ( VIDEO_ENABLED )
    add_definitions(-DVIDEO_ENABLED)
endif()


# sub projects and stuff

SET(BUILD_STATIC_LIBS ON CACHE BOOL "")
SET(BUILD_SHARED_LIBS OFF CACHE BOOL "")
add_definitions(-DQTCOLORWIDGETS_STATICALLY_LINKED)

add_subdirectory(external/QtAppSetup)

add_subdirectory(data ${DATA_INSTALL})

add_subdirectory(src bin)

set(QTCOLORWIDGETS_DESIGNER_PLUGIN ON)
set(QTCOLORWIDGETS_BUILD_STATIC_LIBS ON)
add_subdirectory(external/Qt-Color-Widgets EXCLUDE_FROM_ALL)

set(ALL_SOURCE_DIRECTORIES src)
find_sources(ALL_SOURCES *.cpp *.hpp)

if(BUILD_TESTING)
    find_package(Qt${QT_MAJOR_VERSION}Test REQUIRED)
    message(STATUS "Tests enabled")
    add_subdirectory(test)
endif()

add_subdirectory(docs)

# Doxygen

set(DOXYGEN_FOUND ON)
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    create_doxygen_target(doxygen)
endif(DOXYGEN_FOUND)

# Release

add_custom_target(
    release_0
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deploy/release_0.sh ${CMAKE_CURRENT_BINARY_DIR}/CMakeCache.txt
)
add_custom_target(
    release_1
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deploy/release_1.sh ${CMAKE_CURRENT_BINARY_DIR}/CMakeCache.txt
)
add_custom_target(
    release_check
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/deploy/release_check.py ${VERSION_NAKED}
)

# Misc files
if (ANDROID)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/android/android/version.gradle.in ${CMAKE_BINARY_DIR}/version.gradle)
endif()
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/deploy/org.kde.glaxnimate.metainfo.xml DESTINATION share/metainfo)

ki18n_install(po)
file(CREATE_LINK ${CMAKE_CURRENT_BINARY_DIR}/locale ${CMAKE_CURRENT_BINARY_DIR}/share/locale SYMBOLIC)

# Packaging
# -- see https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/PackageGenerators
set(CPACK_PACKAGE_CONTACT ${PROJECT_MANINTAINER})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})

execute_process(COMMAND dpkg-architecture -qDEB_BUILD_ARCH OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5xml5 (>= 5.10.0), libqt5widgets5 (>= 5.10.0), libpython3.10 (>= 3.10.0), zlib1g (>= 1:1.2.11), libpotrace0 (>= 1.16-2), libavformat58, libswscale5, qt5-image-formats-plugins, libarchive13")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${PROJECT_HOMEPAGE_URL})

set(CPACK_BUNDLE_NAME ${PROJECT_NAME})

set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_BINARY_DIR}/glaxnimate.icns")

include(CPack)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
