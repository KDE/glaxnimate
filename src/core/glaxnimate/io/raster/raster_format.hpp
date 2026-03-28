/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QImageReader>

#include "glaxnimate/io/base.hpp"

namespace glaxnimate::io::raster {


class RasterFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "raster"; }
    QString name() const override { return i18n("Raster Image"); }
    QStringList extensions(io::ImportExport::Direction direction) const override;
    bool can_save() const override { return false; }
    bool can_save_static() const override { return true; }
    bool can_open() const override { return true; }
    int priority() const override { return -1; }

protected:
    bool on_open(QIODevice& dev, const QString&, model::Document* document, const QVariantMap&) override;
    bool on_save_static(QIODevice &file, const QString &filename, model::Composition *comp, model::FrameTime time, const QVariantMap &setting_values) override;
};


} // namespace glaxnimate::io::raster

