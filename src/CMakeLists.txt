# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/core/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gui/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(core)
if ( NOT ANDROID )
    add_subdirectory(trace)
endif()
add_subdirectory(plugin)
add_subdirectory(gui)

if ( MOBILE_UI )
    add_subdirectory(android)
else()
    add_executable(${PROJECT_SLUG} WIN32)
    kde_target_enable_exceptions(${PROJECT_SLUG} PUBLIC)

    target_link_libraries(${PROJECT_SLUG} Glaxnimate::Core Glaxnimate::Gui Glaxnimate::Plugin QtColorWidgets)
    if ( NOT ANDROID )
        target_link_libraries(${PROJECT_SLUG} Glaxnimate::Trace)
    endif()

    install(TARGETS ${PROJECT_SLUG} DESTINATION bin)

    ## Icon for Windows and OSX
    #file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../data/images/logo.svg")
    file(COPY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../data/images/logo.svg" "${CMAKE_CURRENT_BINARY_DIR}/sc-${PROJECT_SLUG}.svg")
    file(COPY_FILE "${PROJECT_SOURCE_DIR}/data/images/${PROJECT_SLUG}.png" "${CMAKE_CURRENT_BINARY_DIR}/512-${PROJECT_SLUG}.png")
    ecm_add_app_icon(${PROJECT_SLUG} ICONS "${CMAKE_CURRENT_BINARY_DIR}/sc-${PROJECT_SLUG}.svg" "${CMAKE_CURRENT_BINARY_DIR}/512-${PROJECT_SLUG}.png") # icon name = variable name

    if (APPLE)
    # Apple app package
    set_target_properties(${PROJECT_SLUG} PROPERTIES
        MACOSX_BUNDLE_DISPLAY_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} ${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.glaxnimate"
        MACOSX_BUNDLE_COPYRIGHT "2019-2025 The ${PROJECT_NAME} Authors")

    endif()

endif()
