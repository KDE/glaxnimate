/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "document_templates.hpp"

#include <numeric>

#include <QAction>

#include "app/application.hpp"

#include "io/glaxnimate/glaxnimate_format.hpp"
#include "model/assets/assets.hpp"

using namespace glaxnimate::gui;
using namespace glaxnimate;


settings::DocumentTemplate::DocumentTemplate(const QString& filename, bool* loaded)
: filename(filename), document(load(loaded))
{}

QSize settings::DocumentTemplate::size() const
{
    if ( auto comp = main_comp() )
        return comp->size();
    return {};
}

model::Composition* settings::DocumentTemplate::main_comp() const
{
    if ( document->assets()->compositions->values.empty() )
        return nullptr;
    return document->assets()->compositions->values[0];
}

model::FrameTime settings::DocumentTemplate::duration() const
{
    if ( auto comp = main_comp() )
        return comp->animation->last_frame.get() - comp->animation->first_frame.get();

    return 0;
}

QString settings::DocumentTemplate::name() const
{
    if ( auto comp = main_comp() )
        return comp->name.get();
    return QObject::tr("Unnamed");
}

QString settings::DocumentTemplate::long_name() const
{
    return name_template(main_comp()).arg(name());
}

float settings::DocumentTemplate::fps() const
{
    if ( auto comp = main_comp() )
        return comp->fps.get();

    return 60;
}

std::unique_ptr<model::Document> settings::DocumentTemplate::create(bool* ok) const
{
    std::unique_ptr<model::Document> document;
    document = load(ok);
    document->assets()->refresh_uuid();
    for ( const auto& comp : document->assets()->compositions->values )
        document->set_best_name(comp.get());
    return document;
}

std::unique_ptr<model::Document> settings::DocumentTemplate::load(bool* ok) const
{
    std::unique_ptr<model::Document> document = std::make_unique<model::Document>("");
    QFile file(filename);
    *ok = io::glaxnimate::GlaxnimateFormat().open(file, filename, document.get(), {});
    return document;
}

bool settings::DocumentTemplate::operator<(const settings::DocumentTemplate& other) const
{
    QString n1 = name();
    QString n2 = other.name();
    if ( n1 == n2 )
    {
        if ( size().width() == other.size().width() )
            return fps() > other.fps();
        return size().width() > other.size().width();
    }
    return n1 < n2;
}

QString settings::DocumentTemplate::aspect_ratio() const
{
    return aspect_ratio(size());
}

QString settings::DocumentTemplate::aspect_ratio(const QSize& size)
{
    if ( size.width() > 0 && size.height() > 0 )
    {
        int gcd = std::gcd(size.width(), size.height());
        return QString("%1:%2").arg(size.width()/gcd).arg(size.height()/gcd);
    }

    return {};
}

QString settings::DocumentTemplate::name_template(model::Composition* comp)
{
    if ( !comp )
        return "%1";

    QString aspect;
    auto w = comp->width.get();
    auto h = comp->height.get();

    //: %5 is the file name, %1x%2 is the size, %3 is the aspect ratio, %4 is the frame rate
    return DocumentTemplates::tr("%5 - %1x%2 %3 %4fps")
        .arg(w)
        .arg(h)
        .arg(aspect_ratio(QSize(w, h)))
        .arg(comp->fps.get())
    ;
}

QAction* settings::DocumentTemplates::create_action(const DocumentTemplate& templ, QObject *parent)
{
    QAction* action = new QAction(QIcon::fromTheme("document-new-from-template"), templ.long_name(), parent);
    connect(action, &QAction::triggered, this, [&templ, this]{
        emit create_from(templ);
    });
    return action;
}

void settings::DocumentTemplates::load()
{
    templates_.clear();
    names.clear();

    for ( QDir dir : app::Application::instance()->data_paths("templates") )
    {
        for ( const auto& filename : dir.entryList(QDir::Files|QDir::Readable) )
        {
            bool ok = false;
            DocumentTemplate templ(dir.absoluteFilePath(filename), &ok);
            if ( ok && !names.count(templ.long_name()) )
            {
                names.insert(templ.long_name());
                templates_.push_back(std::move(templ));
            }
        }
    }

    std::sort(templates_.begin(), templates_.end());

    emit loaded(templates_);
}

settings::DocumentTemplates::DocumentTemplates()
{
    load();
}

bool settings::DocumentTemplates::save_as_template(model::Document* document)
{
    if ( !document->assets()->compositions->values.empty() )
        return false;

    model::Composition* comp = document->assets()->compositions->values[0];

    QString name_base = comp ? comp->name.get() : "";
    if ( name_base.isEmpty() )
        name_base = tr("Template");

    // Find a nice name, avoiding duplicates
    QString name_full_template = DocumentTemplate::name_template(comp);
    QString name_template = tr("%1 %2").arg(name_base);
    QString name = name_base;
    QString name_full = name_full_template.arg(name);
    for ( int count = 1; names.count(name_full); count++ )
    {
        name = name_template.arg(count);
        name_full = name_full_template.arg(name);
    }

    // Generate a file name
    QString basename;
    for ( const QChar& ch : name )
    {
        if ( ch.isLetterOrNumber() || ch == '_' || ch == '-' )
            basename += ch;
        else
            basename += '_';
    }

    io::glaxnimate::GlaxnimateFormat format;
    QDir dirname = app::Application::instance()->writable_data_path("templates");
    QString path = dirname.absoluteFilePath(basename + "." + format.extensions()[0]);

    if ( !dirname.exists() )
    {
        dirname.cdUp();
        dirname.mkpath("templates");
    }

    QFile file(path);
    if ( !format.save(file, path, comp, {}) )
        return false;
    file.close();

    bool ok = false;
    DocumentTemplate templ(path, &ok);
    if ( !ok )
        return false;

    auto it = std::lower_bound(templates_.begin(), templates_.end(), templ);
    templates_.insert(it, std::move(templ));
    names.insert(name_full);

    emit loaded(templates_);

    return true;
}

settings::DocumentTemplates & settings::DocumentTemplates::instance()
{
    static settings::DocumentTemplates instance;
    return instance;
}

const std::vector<settings::DocumentTemplate> & settings::DocumentTemplates::templates() const
{
    return templates_;
}
