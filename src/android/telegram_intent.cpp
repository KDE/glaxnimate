/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "telegram_intent.hpp"

#ifdef Q_OS_ANDROID

#include <QJniEnvironment>
#include <QJniObject>
#include <QApplication>

#include <KLocalizedString>


glaxnimate::android::TelegramIntent::Result glaxnimate::android::TelegramIntent::send_stickers(const QStringList& filenames, const QStringList& emoji)
{
    QJniObject generator_name = QJniObject::fromString(qApp->applicationName());
    QJniObject messenger(
        "org/mattbas/glaxnimate/jnimessenger/JniMessenger",
        "(Ljava/lang/String;)V",
        generator_name.object<jstring>()
    );

    for ( int i = 0; i < filenames.size(); i++ )
    {
        QJniObject sticker_file = QJniObject::fromString(filenames[i]);
        QJniObject sticker_emoji = QJniObject::fromString(emoji[i]);
        messenger.callMethod<void>("add_sticker", "(Ljava/lang/String;Ljava/lang/String;)V", sticker_file.object<jstring>(), sticker_emoji.object<jstring>());
    }


    QJniObject intent = messenger.callObjectMethod("import_stickers", "()Landroid/content/Intent;");
    const QJniObject activity = QNativeInterface::QAndroidApplication::context();
    {
        QJniEnvironment env;
        activity.callMethod<void>("startActivity", "(Landroid/content/Intent;)V", intent.object<jobject>());

        if (env->ExceptionCheck())
        {
            env->ExceptionDescribe();
            env->ExceptionClear();
            return i18n("Could not start activity, is Telegram installed?");
        }
    }
    return {};
}

#else
glaxnimate::android::TelegramIntent::Result glaxnimate::android::TelegramIntent::send_stickers(const QStringList& filenames, const QStringList& emoji)
{
    Q_UNUSED(filenames)
    Q_UNUSED(emoji)
    return {};
}
#endif
