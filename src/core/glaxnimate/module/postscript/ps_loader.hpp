/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "glaxnimate/io/base.hpp"
#include "ps_interpreter.hpp"



namespace glaxnimate::ps {

class Loader : public Interpreter
{
public:
    Loader(io::ImportExport* importer, model::Document* document, const QVariantMap &settings);

protected:
    void on_print(const QString &text) override;
    void on_warning(const QString &text) override;
    void on_error(const QString &text) override;
    void on_comment(const QString &text) override;
    void on_fill(const GraphicsState &gstate) override;

private:
    QPointF convert_coord(const QPointF& p) const;

    io::ImportExport* importer;
    model::Document* document;
    model::Composition* comp = nullptr;
};

} // namespace glaxnimate::ps
