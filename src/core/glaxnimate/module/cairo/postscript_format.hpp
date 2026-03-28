/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once


#include "glaxnimate/io/base.hpp"

namespace glaxnimate::cairo {

class PostScriptFormat : public io::ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "postscript"; }
    QString name() const override { return i18n("PostScript"); }
    QStringList extensions(Direction direction) const override;
    bool can_save() const override { return false; }
    bool can_open() const override { return false; }
    bool can_save_static() const override { return true; }
    std::unique_ptr<settings::SettingsGroup> save_settings(model::Composition*) const override;

protected:
    bool on_save_static(QIODevice& dev, const QString&, model::Composition* comp, model::FrameTime time, const QVariantMap&) override;

};

} // namespace glaxnimate::cairo
