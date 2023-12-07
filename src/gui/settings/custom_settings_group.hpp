/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <QIcon>
#include <KConfig>
#include <KLazyLocalizedString>

namespace glaxnimate::gui::settings {

class CustomSettingsGroup
{
public:
    virtual ~CustomSettingsGroup() = default;
    virtual QString slug() const = 0;
    virtual KLazyLocalizedString label() const = 0;
    virtual QString icon() const = 0;
    virtual void load(KConfig& settings) = 0;
    virtual void save(KConfig& settings) = 0;
    virtual QWidget* make_widget(QWidget* parent) = 0;
    virtual bool has_visible_settings() const { return true; }
    virtual QVariant get_variant(const QString& setting) const
    {
        Q_UNUSED(setting);
        return {};

    }
    virtual bool set_variant(const QString& setting_slug, const QVariant& value)
    {
        Q_UNUSED(setting_slug);
        Q_UNUSED(value);
        return false;
    }

    virtual QVariant get_default(const QString& setting_slug) const
    {
        Q_UNUSED(setting_slug);
        return {};
    }
    virtual QVariant define(const QString& setting_slug, const QVariant& default_value)
    {
        Q_UNUSED(setting_slug);
        Q_UNUSED(default_value);
        return {};
    }
};


template<class Derived>
class SingletonSettingsGroup : public CustomSettingsGroup
{
public:
    SingletonSettingsGroup()
    {
        instance_ = static_cast<Derived*>(this);
    }

    static Derived& instance()
    {
        return *instance_;
    }

private:
    static Derived* instance_;
};
template<class Derived> Derived* SingletonSettingsGroup<Derived>::instance_ = nullptr;


} // namespace glaxnimate::gui::settings

