# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

include_directories(${CMAKE_SOURCE_DIR}/external/QtAppSetup/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/core/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gui/)

if ( PYTHON_SCRIPTING_ENABLED )
    add_compile_definitions(PYTHON_SCRIPTING_ENABLED)
    add_subdirectory(python)
endif()

add_subdirectory(core)
add_subdirectory(gui)

if ( MOBILE_UI )
    add_subdirectory(android)
else()
    add_executable(${PROJECT_SLUG} WIN32)
    kde_target_enable_exceptions(${PROJECT_SLUG} PUBLIC)
    target_link_libraries(${PROJECT_SLUG} ${LIB_NAME_CORE} ${LIB_NAME_GUI} ${LIB_NAME_PYTHON} QtAppSetup)
    install(TARGETS ${PROJECT_SLUG} DESTINATION bin)

    if (APPLE)
    # Apple app package
    set_target_properties(${PROJECT_SLUG} PROPERTIES
        MACOSX_BUNDLE_DISPLAY_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} ${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.glaxnimate"
        MACOSX_BUNDLE_COPYRIGHT "2019-2024 The ${PROJECT_NAME} Authors")

    endif()

endif()
