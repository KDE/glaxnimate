/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unordered_map>
#include <QUrl>
#include <KLocalizedString>

#include "app/utils/qstring_hash.hpp"
#include "settings/custom_settings_group.hpp"


namespace glaxnimate::gui::settings {

class ApiCredentials : public SingletonSettingsGroup<ApiCredentials>
{
public:
    struct Credential
    {
        QString name;
        QString value = "";
        QString hidden_default = "";
    };

    struct Api
    {
        KLazyLocalizedString name;
        QUrl info_url;
        std::vector<Credential> credentials;

        QString credential(const QString& name) const;
    };

    ApiCredentials();

    QString slug() const override { return "api_credentials"; }
    QString icon() const override { return QStringLiteral("dialog-password"); }
    KLazyLocalizedString label() const override { return kli18n("API Credentials"); }
    bool has_visible_settings() const override { return !apis_.empty(); }

    QString value(const QString& api, const QString credential) const;

    QVariant get_variant(const QString& setting) const override;

    void load ( KConfig & settings ) override;

    void save ( KConfig & settings ) override;

    const Api& api(const QString& name) const
    {
        return apis_.at(name);
    }

    QWidget * make_widget ( QWidget * parent ) override;


private:
    std::map<QString, Api> apis_;
};

} // namespace glaxnimate::gui::settings

