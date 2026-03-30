/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/io/base.hpp"
#include "glaxnimate/io/svg/svg_format.hpp"


namespace glaxnimate::io::svg {


class SvgzFormat : public SvgFormat
{
    Q_OBJECT

public:
    QString slug() const override { return "svgz"; }
    QString name() const override { return i18n("Compressed SVG"); }
    QStringList extensions(Direction) const override;

protected:
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;
    bool on_save(QIODevice & file, const QString & filename, model::Composition* comp, const QVariantMap & setting_values) override;
    bool on_save_static(QIODevice & file, const QString & filename, model::Composition* comp, model::FrameTime time, const QVariantMap & setting_values) override;
};


} // namespace glaxnimate::io::svg

