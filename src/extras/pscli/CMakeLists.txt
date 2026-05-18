# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

add_executable(pscli pscli.cpp)
target_link_libraries(pscli Glaxnimate::Core Qt::Widgets)

check_library_exists(readline "rl_callback_handler_install" "" Readline_FOUND)
if ( Readline_FOUND )
    message(STATUS "With readline")
    target_compile_definitions(pscli PRIVATE -DWITH_READLINE)
    target_link_libraries(pscli readline)
endif()
