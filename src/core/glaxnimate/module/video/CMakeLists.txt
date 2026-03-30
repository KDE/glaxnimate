# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

set(SOURCES
    video_format.cpp
    video_module.cpp
)

add_library(GlaxnimateVideo OBJECT ${SOURCES})
add_library(Glaxnimate::Video ALIAS GlaxnimateVideo)


# target_link_libraries(
#     GlaxnimateVideo PUBLIC
#     Glaxnimate::Core
# )

find_package(Libav COMPONENTS codec util format swscale REQUIRED)
include_directories(${Libav_INCLUDE_DIRS})
target_link_libraries(GlaxnimateVideo PUBLIC ${Libav_LIBRARIES} )

glaxnimate_enable_exceptions(GlaxnimateVideo PUBLIC)
