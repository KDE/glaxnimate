/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/command/clipboard.hpp"

#include <QMimeData>
#include <QDataStream>
#include <QIODevice>


QMimeData * glaxnimate::command::copy_helper(
    const std::vector<model::DocumentNode *> nodes,
    const std::vector<io::mime::MimeSerializer *>& supported_mimes
)
{
    QMimeData* data = new QMimeData;
    for ( const auto& serializer : supported_mimes )
    {
        serializer->to_mime_data(*data, nodes);
    }
    return data;
}

using integer_type = int;
static const QString keyframe_mime = QStringLiteral("application/x.glaxnimate-keyframes");

QMimeData *glaxnimate::command::keyframes_to_mime_data(const ClipboardProperties &selection)
{
    if ( selection.properties.empty() )
        return nullptr;

    QMimeData* data = new QMimeData;
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    stream << integer_type(selection.properties.size());

    for ( const auto& prop : selection.properties )
    {
        stream << integer_type(prop.property_type);
        stream << integer_type(prop.keyframes.size());
        if ( !prop.keyframes.empty() )
        {
            auto start_time = prop.keyframes.begin()->time;
            for ( const auto& kf : prop.keyframes )
            {
                stream << kf.time - start_time << kf.value;
            }
        }
    }
    data->setData(keyframe_mime, encoded);
    return data;
}

QMimeData *glaxnimate::command::keyframe_to_mime_data(int property_type, const QVariant &value)
{

    QMimeData* data = new QMimeData;
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    stream << integer_type(1); // 1 property
    stream << integer_type(property_type);
    stream << integer_type(1); // 1 keyframe
    stream << model::FrameTime(0); // time offset
    stream << value;
    data->setData(keyframe_mime, encoded);
    return data;
}

glaxnimate::command::ClipboardProperties glaxnimate::command::keyframes_from_mime_data(const QMimeData *data)
{
    ClipboardProperties result;

    if ( data->hasFormat(keyframe_mime) )
    {
        QByteArray encoded = data->data(keyframe_mime);
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        integer_type prop_count = 0;
        stream >> prop_count;
        result.properties.resize(prop_count);
        for ( integer_type pi = 0; pi < prop_count; pi++ )
        {
            integer_type kf_count = 0;
            stream >> result.properties[pi].property_type >> kf_count;
            for ( integer_type ki = 0; ki < kf_count; ki++ )
            {
                ClipboardKeyframe kf;
                stream >> kf.time >> kf.value;
                result.properties[pi].keyframes.insert(kf.time, kf);
            }
        }
    }

    return result;
}

bool glaxnimate::command::keyframes_mime_data_has_type(const QMimeData *data, int property_type)
{
    if ( !data->hasFormat(keyframe_mime) )
        return false;

    QByteArray encoded = data->data(keyframe_mime);
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    integer_type prop_count = 0;
    stream >> prop_count;

    for ( integer_type pi = 0; pi < prop_count; pi++ )
    {
        integer_type type = model::PropertyTraits::Unknown;
        integer_type kf_count = 0;
        stream >> type >> kf_count;
        if ( type == property_type )
            return kf_count > 0;

        for ( integer_type ki = 0; ki < kf_count; ki++ )
        {
            ClipboardKeyframe kf;
            stream >> kf.time >> kf.value;
        }

    }

    return false;

}

QUndoCommand *glaxnimate::command::keyframes_paste_command(
    const QMimeData *data,
    model::AnimatableBase *target_property,
    int property_type,
    model::FrameTime start_time
)
{

    auto clipboard_data = command::keyframes_from_mime_data(data);
    auto* prop = clipboard_data.by_type(property_type);
    if ( !prop || prop->keyframes.empty() )
        return nullptr;

    auto cmd = new QUndoCommand(i18np("Paste keyframe", "Paste keyframes", prop->keyframes.size()));
    for ( const auto& kf : prop->keyframes )
    {
        target_property->command_add_smooth_keyframe(start_time + kf.time, kf.value, true, cmd);
    }

    return cmd;
}

bool glaxnimate::command::has_keyframe_data(const QMimeData *data)
{
    return data->hasFormat(keyframe_mime);
}
