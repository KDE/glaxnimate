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

} // namespace glaxnimate::command
