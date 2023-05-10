/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include "io/base.hpp"
#include "io/io_registry.hpp"

namespace glaxnimate::io::glaxnimate {


class GlaxnimateFormat : public ImportExport
{
    Q_OBJECT

public:
    static const int format_version;

    QString slug() const override { return "glaxnimate"; }
    QString name() const override { return tr("Glaxnimate Animation"); }
    // RAWR = Reasonable Animation at Whatever Resolution
    QStringList extensions() const override { return {"rawr"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }

    static QJsonDocument to_json(model::Document* document);
    static QJsonObject to_json(model::Object* object);
    static QJsonValue to_json(model::BaseProperty* property);
    static QJsonValue to_json(const QVariant& value);
    static QJsonValue to_json(const QVariant& value, model::PropertyTraits traits);
    static QJsonObject format_metadata();
    static GlaxnimateFormat* instance() { return autoreg.registered; }

protected:
    bool on_save(QIODevice& file, const QString&, model::Composition* comp, const QVariantMap&) override;
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;

private:
    static Autoreg<GlaxnimateFormat> autoreg;
};


} // namespace glaxnimate::io::glaxnimate
