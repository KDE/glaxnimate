/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
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
/**
 * Whether cli_main would request to quit before the GUI
 */
bool cli_no_gui(const glaxnimate::cli::ParsedArguments& args);


} // namespace glaxnimate::gui
