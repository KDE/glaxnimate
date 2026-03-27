/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/model/document_node.hpp"
#include "glaxnimate/io/mime/mime_serializer.hpp"

namespace glaxnimate::command {

QMimeData* copy_helper(
    const std::vector< glaxnimate::model::DocumentNode* > nodes,
    const std::vector< glaxnimate::io::mime::MimeSerializer* >& supported_mimes);

struct ClipboardKeyframe
{
    model::FrameTime time = 0;
    QVariant value = {};
};

struct ClipboardProperty
{
    int property_type = model::PropertyTraits::Unknown;
    model::KeyframeContainer<ClipboardKeyframe> keyframes = {};
};

struct ClipboardProperties
{
    std::vector<ClipboardProperty> properties;

    const ClipboardProperty* by_type(int property_type) const
    {
        for ( const auto& prop : properties )
            if ( prop.property_type == property_type )
                return &prop;
        return nullptr;
    }
};


QMimeData* keyframes_to_mime_data(const ClipboardProperties &selection);
QMimeData* keyframe_to_mime_data(int property_type, const QVariant& value);
ClipboardProperties keyframes_from_mime_data(const QMimeData* data);
bool keyframes_mime_data_has_type(const QMimeData *data, int property_type);
QUndoCommand* keyframes_paste_command(const QMimeData *data, model::AnimatableBase* target_property, int property_type, model::FrameTime start_time);
bool has_keyframe_data(const QMimeData *data);

} // namespace glaxnimate::command
