#include "rive_format.hpp"

#include "rive_loader.hpp"

#include <QJsonArray>
#include <QJsonObject>

glaxnimate::io::Autoreg<glaxnimate::io::rive::RiveFormat> glaxnimate::io::rive::RiveFormat::autoreg;


bool glaxnimate::io::rive::RiveFormat::on_open(QIODevice& file, const QString&, model::Document* document, const QVariantMap&)
{
    RiveStream stream(&file);
    if ( stream.read(4) != "RIVE" )
    {
        error(tr("Unsupported format"));
        return false;
    }

    auto vmaj = stream.read_varuint();
    auto vmin = stream.read_varuint();
    stream.read_varuint(); // file id

    if ( stream.has_error() )
    {
        error(tr("Could not read header"));
        return false;
    }

    if ( vmaj != 7 )
    {
        error(tr("Loading unsupported rive file version %1.%2, the only supported version is %3").arg(vmaj).arg(vmin).arg(7));
        return false;
    }

    if ( stream.has_error() )
    {
        error(tr("Could not read property table"));
        return false;
    }

    return RiveLoader(stream, this).load_document(document);
}

bool glaxnimate::io::rive::RiveFormat::on_save(QIODevice&, const QString&, model::Document*, const QVariantMap&)
{
    return false;
}

QJsonDocument glaxnimate::io::rive::RiveFormat::to_json(const QByteArray& binary_data)
{
    RiveStream stream(binary_data);
    if ( stream.read(4) != "RIVE" )
        return {};

    auto vmaj = stream.read_varuint();
    stream.read_varuint(); // version min
    stream.read_varuint(); // file id

    if ( stream.has_error() || vmaj != 7 )
        return {};

    QJsonArray objects;
    for ( const auto& rive_obj : RiveLoader(stream, this).load_object_list() )
    {
        QJsonObject obj;

        QJsonArray types;
        for ( const auto& def : rive_obj.definitions )
        {
            QJsonObject jdef;
            jdef["id"] = int(def->type_id);
            jdef["name"] = def->name;
            types.push_back(jdef);
        }
        obj["class"] = types;

        QJsonArray props;
        for ( const auto& p : rive_obj.property_definitions )
        {
            QJsonObject prop;
            prop["id"] = int(p.first);
            prop["name"] = p.second.name;
            prop["type"] = int(p.second.type);
            auto value = rive_obj.properties.value(p.second.name);
            if ( value.userType() == QMetaType::QColor )
                prop["value"] = value.value<QColor>().name();
            else
                prop["value"] = QJsonValue::fromVariant(value);
            props.push_back(prop);
        }
        obj["properties"] = props;

        objects.push_back(obj);
    }

    return QJsonDocument(objects);
}
