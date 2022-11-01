#include "type_system.hpp"

glaxnimate::io::rive::TypeSystem::TypeSystem(int version)
    : definitions(version == 6 ? &defined_objects6 : &defined_objects7)
{
}


const glaxnimate::io::rive::ObjectDefinition * glaxnimate::io::rive::TypeSystem::get_definition(glaxnimate::io::rive::TypeId type_id)
{
    auto it = definitions->find(type_id);
    if ( it == definitions->end() )
    {
        emit type_not_found(int(type_id));
        return nullptr;
    }

    return &it->second;
}

bool glaxnimate::io::rive::TypeSystem::gather_definitions(glaxnimate::io::rive::ObjectType& type, glaxnimate::io::rive::TypeId type_id)
{
    auto* def = get_definition(type_id);
    if ( !def )
        return false;

    type.definitions.push_back(def);

    if ( def->extends != TypeId::NoType )
    {
        if ( !gather_definitions(type, def->extends) )
            return false;
    }

    for ( const auto& prop : def->properties )
    {
        type.property_from_name[prop.name] = &prop;
        type.property_from_id[prop.id] = &prop;
        type.properties.push_back(&prop);
    }

    return true;
}

const glaxnimate::io::rive::ObjectType * glaxnimate::io::rive::TypeSystem::get_type(glaxnimate::io::rive::TypeId type_id)
{
    auto it = types.find(type_id);
    if ( it != types.end() )
        return &it->second;

    ObjectType type(type_id);
    if ( !gather_definitions(type, type_id) )
        return nullptr;

    return &types.emplace(type_id, std::move(type)).first->second;
}

QString glaxnimate::io::rive::TypeSystem::type_name(glaxnimate::io::rive::TypeId type_id)
{
    if ( auto def = get_definition(type_id) )
        return def->name;
    return {};
}
