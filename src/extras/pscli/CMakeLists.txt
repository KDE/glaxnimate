# SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: BSD-2-Clause

find_package(Readline)
add_executable(pscli pscli.cpp)
target_link_libraries(pscli Glaxnimate::Core Qt::Widgets readline)
