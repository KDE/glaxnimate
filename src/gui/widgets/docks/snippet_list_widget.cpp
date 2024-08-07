/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "snippet_list_widget.hpp"
#include "ui_snippet_list_widget.h"

#include <QEvent>
#include <QDesktopServices>
#include <QUrl>

#include "item_models/python_snippet_model.hpp"

using namespace glaxnimate::gui;

class SnippetListWidget::Private
{
public:
    Ui::SnippetListWidget ui;
    item_models::PythonSnippetModel model;
};

SnippetListWidget::SnippetListWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->model.reload();
    d->ui.list_view->setModel(&d->model);
}

SnippetListWidget::~SnippetListWidget() = default;

void SnippetListWidget::snippet_new()
{
    d->ui.list_view->setCurrentIndex(d->model.append());
}

void SnippetListWidget::snippet_delete()
{
    d->model.removeRows(d->ui.list_view->currentIndex().row(), 1, {});
}

void SnippetListWidget::snippet_edit()
{
    auto snippet = d->model.snippet(d->ui.list_view->currentIndex());
    if ( snippet.name().isEmpty() )
    {
        Q_EMIT warning(i18n("Snippets need a name"), i18n("Snippets"));
        return;
    }

    if ( !snippet.ensure_file_exists() )
    {
        Q_EMIT warning(i18n("Could not create snippet: `%1`", snippet.filename()), i18n("Snippets"));
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(snippet.filename()));
}

void SnippetListWidget::snippet_run()
{
    auto snippet = d->model.snippet(d->ui.list_view->currentIndex());
    if ( !snippet.name().isEmpty() )
    {
        QString src = snippet.get_source();
        if ( !src.isEmpty() )
            Q_EMIT run_snippet(src);
    }
}

void SnippetListWidget::snippet_reload()
{
    d->model.reload();
}
