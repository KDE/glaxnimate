/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unordered_set>

#include "utils/pseudo_mutex.hpp"

namespace glaxnimate::model {

class ReferencePropertyBase;
class DocumentNode;

class AssetBase
{
public:
    using User = ReferencePropertyBase;

    virtual ~AssetBase() {}

    /**
     * \brief Removes the asset if it isn't needed
     * \param clean_lists when \b true, remove even if the asset is in a useful list
     * \return Whether it has been removed
     */
    virtual bool remove_if_unused(bool clean_lists) = 0;
};



} // namespace glaxnimate::model

