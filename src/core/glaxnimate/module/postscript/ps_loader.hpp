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

    bool success() const;

protected:
    void on_print(const QString &text) override;
    void on_warning(const QString &text) override;
    void on_error(const QString &text) override;
    void on_comment(const QString &text) override;
    void on_fill(const GraphicsState &gstate) override;

private:
    QPointF convert(const QPointF& p) const;
    math::bezier::Point convert(const math::bezier::Point& p) const;
    math::bezier::Bezier convert(const math::bezier::Bezier& p) const;

    io::ImportExport* importer;
    model::Document* document;
    model::Composition* comp = nullptr;
    bool has_error = false;
};

} // namespace glaxnimate::ps
