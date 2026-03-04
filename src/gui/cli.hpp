/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QApplication>

#include "cli_utils/cli.hpp"
#include "glaxnimate_app.hpp"

namespace glaxnimate::gui {


glaxnimate::cli::ParsedArguments parse_cli(const QStringList& args);
void cli_main(gui::GlaxnimateApp& app, glaxnimate::cli::ParsedArguments& args);


} // namespace glaxnimate::gui
