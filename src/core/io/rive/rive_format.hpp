#pragma once

#include <QJsonDocument>
#include "io/base.hpp"
#include "io/io_registry.hpp"

namespace glaxnimate::io::rive {


class RiveFormat : public ImportExport
{
    Q_OBJECT

public:
    static const int format_version;

    QString slug() const override { return "rive"; }
    QString name() const override { return tr("Rive Animation"); }
    QStringList extensions() const override { return {"riv"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }

    static RiveFormat* instance() { return autoreg.registered; }

    QJsonDocument to_json(const QByteArray& binary_data);

    static constexpr const int version_maj = 7;
    static constexpr const int version_min = 0;

protected:
    bool on_save(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;
    bool on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&) override;

private:
    static Autoreg<RiveFormat> autoreg;
};


} // namespace glaxnimate::io::rive

