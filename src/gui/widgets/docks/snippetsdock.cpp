/*
 * SPDX-FileCopyrightText: 2024 Julius Künzel <julius.kuenzel@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "snippetsdock.h"

#include "app/log/log_model.hpp"
#include "style/better_elide_delegate.hpp"
#include "ui_snippets.h"

using namespace glaxnimate::gui;

class SnippetsDock::Private
{
public:
    ::Ui::dock_snippets ui;
};

SnippetsDock::SnippetsDock(GlaxnimateWindow *parent)
    : QDockWidget(parent)
    , d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
}

SnippetsDock::~SnippetsDock() = default;
