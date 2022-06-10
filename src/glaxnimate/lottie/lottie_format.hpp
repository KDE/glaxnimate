#pragma once

#include <QCborMap>
#include "glaxnimate/core/io/base.hpp"
#include "glaxnimate/core/io/io_registry.hpp"

namespace glaxnimate::io::lottie {


class LottieFormat : public ImportExport
{
    Q_OBJECT

public:
    QString slug() const override { return "lottie"; }
    QString name() const override { return tr("Lottie Animation"); }
    QStringList extensions() const override { return {"json"}; }
    bool can_save() const override { return true; }
    bool can_open() const override { return true; }
    std::unique_ptr<app::settings::SettingsGroup> save_settings(model::Document*) const override
    {
        return std::make_unique<app::settings::SettingsGroup>(app::settings::SettingList{
            app::settings::Setting("pretty", tr("Pretty"), tr("Pretty print the JSON"), false)
        });
    }

    QCborMap to_json(model::Document* document, bool strip = false, bool strip_raster = false);
    bool load_json(const QByteArray& data, model::Document* document);

private:
    bool on_save(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override;

    bool on_open(QIODevice& file, const QString& filename,
                 model::Document* document, const QVariantMap& setting_values) override;
private:
    static Autoreg<LottieFormat> autoreg;
};


} // namespace glaxnimate::io::lottie
