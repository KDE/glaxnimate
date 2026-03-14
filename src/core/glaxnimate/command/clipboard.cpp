/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate/command/clipboard.hpp"

#include <QMimeData>


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
