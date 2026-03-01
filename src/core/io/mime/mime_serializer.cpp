/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mime_serializer.hpp"

#include "app/log/log.hpp"

#include "model/object.hpp"
#include "model/document.hpp"
#include "model/assets/assets.hpp"


glaxnimate::io::mime::DeserializedData glaxnimate::io::mime::MimeSerializer::from_mime_data(const QMimeData& data) const
{
    if ( !can_deserialize() )
        return {};

    for ( const QString& mime : mime_types() )
        if ( data.hasFormat(mime) )
            return deserialize(data.data(mime));

    return {};
}

void glaxnimate::io::mime::MimeSerializer::message(const QString& message, app::log::Severity severity) const
{
    app::log::Log(slug()).log(message, severity);
}


glaxnimate::io::mime::DeserializedData glaxnimate::io::mime::MimeSerializer::deserialize(const QByteArray&) const
{
    return {};
}

glaxnimate::io::mime::DeserializedData::DeserializedData() = default;
glaxnimate::io::mime::DeserializedData::DeserializedData(DeserializedData &&) = default;
glaxnimate::io::mime::DeserializedData & glaxnimate::io::mime::DeserializedData::operator=(DeserializedData &&) = default;
glaxnimate::io::mime::DeserializedData::~DeserializedData() = default;

bool glaxnimate::io::mime::DeserializedData::empty() const
{
    return !document || main->shapes.empty();
}


void glaxnimate::io::mime::DeserializedData::initialize_data()
{
    document = std::make_unique<model::Document>("");
    main = document->assets()->compositions->values.insert(std::make_unique<model::Composition>(document.get()));
}
