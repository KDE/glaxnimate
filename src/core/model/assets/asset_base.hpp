#pragma once

#include <unordered_set>

#include "utils/pseudo_mutex.hpp"

namespace glaxnimate::model {

class ReferenceBase;
class DocumentNode;

class AssetBase
{
public:
    using User = ReferenceBase;

    virtual ~AssetBase() {}

    /**
     * \brief Removes the asset if it isn't needed
     * \param clean_lists when \b true, remove even if the asset is in a useful list
     * \return Whether it has been removed
     */
    virtual bool remove_if_unused(bool clean_lists) = 0;
};



} // namespace glaxnimate::model

