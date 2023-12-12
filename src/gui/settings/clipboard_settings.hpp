/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include "settings/custom_settings_group.hpp"
#include "io/mime/mime_serializer.hpp"
#include <KLocalizedString>

namespace glaxnimate::gui::settings {

class ClipboardSettings : public CustomSettingsGroup
{
public:
    QString slug() const override { return QStringLiteral("clipboard"); }
    KLazyLocalizedString label() const override { return kli18n("Clipboard"); }
    QString icon() const override { return QStringLiteral("klipper"); }
    void load(KConfig & settings) override;
    void save(KConfig & settings) override;
    QWidget * make_widget(QWidget * parent) override;

    struct MimeSettings
    {
        io::mime::MimeSerializer* serializer;
        bool enabled;
        QIcon icon;
    };

    static const std::vector<MimeSettings>& mime_types();

};

} // namespace glaxnimate::gui::settings
