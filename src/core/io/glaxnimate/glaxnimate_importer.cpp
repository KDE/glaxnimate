/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_format.hpp"

#include "import_state.hpp"
#include "model/assets/assets.hpp"

using namespace glaxnimate;

bool io::glaxnimate::GlaxnimateFormat::on_open ( QIODevice& file, const QString&, model::Document* document, const QVariantMap& )
{
    QJsonDocument jdoc;

    try {
        jdoc = QJsonDocument::fromJson(file.readAll());
    } catch ( const QJsonParseError& err ) {
        error(tr("Could not parse JSON: %1").arg(err.errorString()));
        return false;
    }

    if ( !jdoc.isObject() )
    {
        error(tr("No JSON object found"));
        return false;
    }

    QJsonObject top_level = jdoc.object();

    if ( !top_level["animation"].isObject() )
    {
        error(tr("Missing animation object"));
        return false;
    }

    int document_version = top_level["format"].toObject()["format_version"].toInt(0);
    detail::ImportState state(this, document, document_version);
    if ( document_version > format_version )
        warning(tr("Opening a file from a newer version of Glaxnimate"));

    state.load_metadata(document, top_level);
    auto assets = top_level[document_version < 3 ? "defs" : "assets"].toObject();
    if ( document_version < 8 )
    {
        QJsonArray precomps;
        if ( assets.contains("precompositions") )
            precomps = assets["precompositions"].toArray();
        precomps.push_front(top_level["animation"]);
        assets["precompositions"] = precomps;
    }
    state.load_object(document->assets(), assets);
    state.resolve();

    return true;
}
