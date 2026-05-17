/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "glaxnimate/io/base.hpp"
#include "ps_interpreter.hpp"


namespace glaxnimate::ps {


class PsToGlaxnimate : public Interpreter
{
public:
    PsToGlaxnimate(model::Document* document);

    void apply_metadata() const;

    model::Composition* current_page();

protected:
    void on_comment(const QByteArray &text) override;
    void on_meta_comment(const QByteArray& key, const QByteArray& value) override;
    void on_fill(const GraphicsState &gstate, bool evenodd) override;
    void on_stroke(const GraphicsState& gstate) override;
    void on_show_page(bool copy) override;
    void on_image(const ImageData &image, const GraphicsState &gstate) override;
    void on_draw_text(const TextDrawOptions &options, const GraphicsState &gstate) override;

    virtual void on_comp_finished(model::Composition* comp);

private:
    void apply_page_metadata() const;
    QPointF convert(const QPointF& p) const;
    math::bezier::Point convert(const math::bezier::Point& p) const;
    math::bezier::Bezier convert(const math::bezier::Bezier& p) const;
    QTransform convert(const QTransform& tf) const;
    QRectF parse_bounding_box(QStringView box) const;
    std::optional<QString> get_page_meta(const QByteArray& page, const QByteArray& doc) const;
    void new_comp();
    void use_page();

    bool last_comp_used = false;
    QString object_name;
    model::Document* document;
    model::Composition* comp = nullptr;
};

class Loader : public PsToGlaxnimate
{
public:
    Loader(io::ImportExport* importer, model::Document* document, const QVariantMap &settings);

    bool success() const;

protected:
    void on_print(const QString &text) override;
    void on_warning(const QString &text) override;
    void on_error(const QString &text) override;

private:

    io::ImportExport* importer;
    bool has_error = false;
};

} // namespace glaxnimate::ps
