/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/module/postscript/ps_format.hpp"
#include "ps_loader.hpp"

using namespace glaxnimate;

QStringList ps::PostScriptFormat::extensions(Direction) const
{
    return {QStringLiteral("ps"), QStringLiteral("eps"), QStringLiteral("epsf")};

}

bool ps::PostScriptFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap &setting_values)
{
    Loader loader(this, document, setting_values);
    loader.execute(&file);
    return loader.success();
}
