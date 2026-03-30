/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "glaxnimate/io/base.hpp"
#include "glaxnimate/io/io_registry.hpp"

namespace glaxnimate::io::aep {

struct RiffChunk;

class AepFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "aep"; }
    QString name() const override { return i18n("Adobe After Effects Project"); }
    QStringList extensions(Direction) const override { return {"aep"}; }
    bool can_save() const override { return false; }
    bool can_open() const override { return true; }

protected:
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap& options) override;

    bool riff_to_document(const RiffChunk& chunk, model::Document* document, const QString& filename);
};


class AepxFormat : public AepFormat
{
    Q_OBJECT

public:
    QString slug() const override { return "aepx"; }
    QString name() const override { return i18n("Adobe After Effects Project XML"); }
    QStringList extensions(Direction) const override { return {"aepx"}; }
    bool can_save() const override { return false; }
    bool can_open() const override { return true; }

protected:
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap& options) override;
};

} // namespace glaxnimate::io::aep



