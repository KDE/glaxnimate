/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "document_node.hpp"

template<>
struct std::hash<QUuid>
{
    std::size_t operator()(const QUuid& uuid) const noexcept
    {
        return qHash(uuid);
    }
};


namespace glaxnimate::model {

/**
 * \brief Maps UUIDs to nodes
 *
 * Ensures UUIDs are refreshed if needed
 */
class NodeMap
{
public:
    template<class T = DocumentNode>
    T* node_from_uuid(const QUuid& uuid) const
    {
        auto it = nodes.find(uuid);
        if ( it == nodes.end() )
            return nullptr;
        return qobject_cast<T*>(it->second);
    }

    bool remove(const QUuid& uuid)
    {
        return nodes.erase(uuid) != 0;
    }

    bool remove(DocumentNode* node)
    {
        return remove(node->uuid.get());
    }

    void add(DocumentNode* node, bool recurse, bool force_refresh)
    {
        QUuid id = node->uuid.get();

        if ( id.isNull() )
        {
            id = QUuid::createUuid();
        }
        else if ( force_refresh )
        {
            remove(id);
            id = QUuid::createUuid();
        }

        while ( true )
        {
            auto it = nodes.find(id);
            if ( it == nodes.end() || it->second == node )
                break;

            id = QUuid::createUuid();
        }

        node->uuid.set_value(id);
        nodes[id] = node;

        if ( recurse )
        {
            for ( auto prop : node->properties() )
            {
                if ( prop->traits().type == PropertyTraits::Object )
                {
                    if ( prop->traits().flags & PropertyTraits::List )
                    {
                        for ( auto v : prop->value().toList() )
                        {
                            if ( auto obj = v.value<glaxnimate::model::DocumentNode*>() )
                                add(obj, true, force_refresh);
                        }
                    }
                    else
                    {
                        if ( auto obj = qobject_cast<DocumentNode*>(static_cast<glaxnimate::model::SubObjectPropertyBase*>(prop)->sub_object()) )
                            add(obj, true, force_refresh);
                    }
                }
            }
        }
    }
private:
    std::unordered_map<QUuid, DocumentNode*> nodes;
};

} // namespace glaxnimate::model
