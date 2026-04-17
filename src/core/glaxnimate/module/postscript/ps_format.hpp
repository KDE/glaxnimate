/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "glaxnimate/io/base.hpp"

namespace glaxnimate::ps {


class PostScriptFormat : public io::ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "postscript"; }
    QString name() const override { return i18n("PostScript"); }
    QStringList extensions(Direction direction) const override;
    bool can_save() const override { return false; }
    bool can_open() const override { return true; }
    bool can_save_static() const override { return false; }

protected:
    bool on_open(QIODevice &file, const QString &filename, model::Document *document, const QVariantMap &setting_values) override;
};

} // namespace glaxnimate::ps
