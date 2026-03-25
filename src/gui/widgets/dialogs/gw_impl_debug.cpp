/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "glaxnimate_window_p.hpp"

#include <QShortcut>
#include <QJsonDocument>
#include <QTemporaryFile>
#include <QDomDocument>
#include <QFontDatabase>
#include <QMenu>
#include <QToolBar>
#include <QDesktopServices>

#include "glaxnimate/io/base.hpp"
#include "glaxnimate/io/glaxnimate/glaxnimate_format.hpp"
#include "glaxnimate/io/rive/rive_format.hpp"
#include "glaxnimate/io/lottie/lottie_format.hpp"
#include "glaxnimate/utils/gzip.hpp"
#include "glaxnimate/model/custom_font.hpp"

#include "widgets/timeline/timeline_widget.hpp"
#include "widgets/dialogs/clipboard_inspector.hpp"

#include "glaxnimate_app.hpp"

void debug_cmd(const QUndoCommand* cmd, std::string ind)
{
    qDebug() << ind.c_str()
        << cmd->text()
        << cmd->id()
        << cmd->childCount();
    for ( int i = 0; i < cmd->childCount(); i++ )
        debug_cmd(cmd->child(i), ind + "    ");
}

namespace  {

void screenshot_widget(const QString& path, QWidget* widget)
{
    widget->show();
    QString base = widget->objectName();
    QString name = path + base.mid(base.indexOf("_")+1);
    QPixmap pic(widget->size());
    widget->render(&pic);
    name += ".png";
    pic.save(name);
}

QString pretty_json(const QJsonDocument& input)
{
    QTemporaryFile tempf(GlaxnimateApp::temp_path() + "/XXXXXX.json");
    tempf.setAutoRemove(false);
    tempf.open();
    tempf.write(input.toJson(QJsonDocument::Indented));
    return tempf.fileName();
}

QString pretty_json(const QByteArray& input)
{
    return pretty_json(QJsonDocument::fromJson(input));
}

QString pretty_xml(const QByteArray& xml)
{
    QTemporaryFile tempf(GlaxnimateApp::temp_path() + "/XXXXXX.json");
    tempf.setAutoRemove(false);
    tempf.open();
    QDomDocument doc;
    doc.setContent(xml);
    tempf.write(doc.toByteArray(4));
    return tempf.fileName();
}

QString pretty_rive(const QByteArray& input)
{
    return pretty_json(glaxnimate::io::rive::RiveFormat().to_json(input));
}

inline bool open_file(const QString& path)
{
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void json_to_pretty_temp(const QJsonDocument& doc)
{
    QTemporaryFile tempf(GlaxnimateApp::temp_path() + "/XXXXXX.json");
    tempf.setAutoRemove(false);
    tempf.open();
    tempf.write(doc.toJson(QJsonDocument::Indented));
    tempf.close();
    open_file(tempf.fileName());
}

inline void print_model_column(const QAbstractItemModel* model, int i, bool flags, const std::vector<int>& roles, const QModelIndex& index, int indent)
{
    auto logger = qDebug();
    logger.noquote();
    auto colindex = model->index(index.row(), i, index.parent());
    for ( int role : roles )
    {
        logger << QString(4*indent, ' ') << "  *" << model->data(colindex, role);
        if ( flags )
            logger << model->flags(colindex);
    }
}

struct DebugSlot
{
    QString prefix;
    QString signal;

    template<class T>
    decltype(std::declval<QDebug&>() << std::declval<T>()) print(QDebug& stream, T&& t, bool)
    {
        return stream << std::forward<T>(t);
    }

    template<class T>
    void print(QDebug&, T&&, ...){}
    void print_all(QDebug&) {}

    template<class T, class... Args>
    void print_all(QDebug& stream, T&& t, Args&&... args)
    {
        print(stream, std::forward<T>(t), true);
        print_all(stream, std::forward<Args>(args)...);
    }

    template<class... Args>
    void operator()(Args&&... args)
    {
        auto stream = qDebug();
        if ( !prefix.isEmpty() )
            stream << prefix;
        stream << signal;
        print_all(stream, std::forward<Args>(args)...);
    }
};

inline void print_model_row(const QAbstractItemModel* model, const QModelIndex& index, const std::vector<int>& columns = {}, bool flags = false, const std::vector<int>& roles = {Qt::DisplayRole}, int indent = 0)
{
    int rows = model->rowCount(index);
    int cols = model->columnCount(index);

    qDebug().noquote() << QString(4*indent, ' ') << index << "rows" << rows << "cols" << cols << "ptr" << index.internalId();

    if ( columns.empty() )
    {
        for ( int i = 0; i < cols; i++ )
            print_model_column(model, i, flags, roles, index, indent);
    }
    else
    {
        for ( int i : columns )
            print_model_column(model, i, flags, roles, index, indent);
    }
}

inline void print_model(const QAbstractItemModel* model, const std::vector<int>& columns = {}, bool flags = false, const std::vector<int>& roles = {Qt::DisplayRole}, const QModelIndex& index = {}, int indent = 0)
{
    print_model_row(model, index, columns, flags, roles, indent);
    for ( int i = 0; i < model->rowCount(index); i++ )
    {
        QModelIndex ci = model->index(i, 0, index);
        if ( !ci.isValid() )
            qDebug().noquote() << QString(4*(indent+1), ' ') << "invalid";
        else
            print_model(model, columns, flags, roles, ci, indent+1);
    }
}

inline void connect_debug(QAbstractItemModel* model, const QString& prefix)
{
    QObject::connect(model, &QAbstractItemModel::columnsAboutToBeInserted,  model, DebugSlot{prefix, "columnsAboutToBeInserted"});
    QObject::connect(model, &QAbstractItemModel::columnsAboutToBeMoved,     model, DebugSlot{prefix, "columnsAboutToBeMoved"});
    QObject::connect(model, &QAbstractItemModel::columnsAboutToBeRemoved,   model, DebugSlot{prefix, "columnsAboutToBeRemoved"});
    QObject::connect(model, &QAbstractItemModel::columnsInserted,           model, DebugSlot{prefix, "columnsInserted"});
    QObject::connect(model, &QAbstractItemModel::columnsMoved,              model, DebugSlot{prefix, "columnsMoved"});
    QObject::connect(model, &QAbstractItemModel::columnsRemoved,            model, DebugSlot{prefix, "columnsRemoved"});
    QObject::connect(model, &QAbstractItemModel::dataChanged,               model, DebugSlot{prefix, "dataChanged"});
    QObject::connect(model, &QAbstractItemModel::headerDataChanged,         model, DebugSlot{prefix, "headerDataChanged"});
    QObject::connect(model, &QAbstractItemModel::layoutAboutToBeChanged,    model, DebugSlot{prefix, "layoutAboutToBeChanged"});
    QObject::connect(model, &QAbstractItemModel::layoutChanged,             model, DebugSlot{prefix, "layoutChanged"});
    QObject::connect(model, &QAbstractItemModel::modelAboutToBeReset,       model, DebugSlot{prefix, "modelAboutToBeReset"});
    QObject::connect(model, &QAbstractItemModel::modelReset,                model, DebugSlot{prefix, "modelReset"});
    QObject::connect(model, &QAbstractItemModel::rowsAboutToBeInserted,     model, DebugSlot{prefix, "rowsAboutToBeInserted"});
    QObject::connect(model, &QAbstractItemModel::rowsAboutToBeMoved,        model, DebugSlot{prefix, "rowsAboutToBeMoved"});
    QObject::connect(model, &QAbstractItemModel::rowsAboutToBeRemoved,      model, DebugSlot{prefix, "rowsAboutToBeRemoved"});
    QObject::connect(model, &QAbstractItemModel::rowsInserted,              model, DebugSlot{prefix, "rowsInserted"});
    QObject::connect(model, &QAbstractItemModel::rowsMoved,                 model, DebugSlot{prefix, "rowsMoved"});
    QObject::connect(model, &QAbstractItemModel::rowsRemoved,               model, DebugSlot{prefix, "rowsRemoved"});
}

} // namespace



void GlaxnimateWindow::Private::init_debug()
{
    QMenu* menu_debug = new QMenu("Debug", parent);

    auto shortcut = new QShortcut(QKeySequence(Qt::META|Qt::Key_D), canvas);
    connect(shortcut, &QShortcut::activated, parent, [menu_debug]{
            menu_debug->exec(QCursor::pos());
    });

    // Models
    QMenu* menu_print_model = new QMenu("Print Model", menu_debug);
    menu_debug->addAction(menu_print_model->menuAction());

    menu_print_model->addAction("Document Node - Full", [this]{
        print_model(&document_node_model, {1}, false);
    });

    menu_print_model->addAction("Document Node - Layers", [this]{
        print_model(layers_dock->layer_view()->model(), {1}, false);
    });

    menu_print_model->addAction("Document Node - Assets", [this]{
        print_model(&asset_model, {0}, false);
    });

    menu_print_model->addSeparator();

    menu_print_model->addAction("Properties - Single", [this]{
        print_model(&property_model, {0}, false);
    });

    menu_print_model->addAction("Properties - Full", [this]{
        print_model(timeline_dock->timelineWidget()->raw_model(), {0}, false);
    });

    menu_print_model->addAction("Properties - Full (Filtered)", [this]{
        print_model(timeline_dock->timelineWidget()->filtered_model(), {0}, false);
    });

    QMenu* menu_model_signals = new QMenu("Show Model Signals", menu_debug);
    menu_debug->addAction(menu_model_signals->menuAction());
    menu_model_signals->addAction("Document Node - Full", [this]{
        connect_debug(&document_node_model, "Document Node - Full");
    });
    menu_model_signals->addAction("Document Node - Layers", [this]{
        connect_debug(layers_dock->layer_view()->model(), "Document Node - Layers");
    });

    menu_debug->addAction("Current index", [this]{

        auto layers_index = layers_dock->layer_view()->currentIndex();
        qDebug() << "Layers" << layers_index << layers_dock->layer_view()->current_node() << layers_dock->layer_view()->node(layers_index);


        qDebug() << "Timeline" << timeline_dock->timelineWidget()->current_index_raw() << timeline_dock->timelineWidget()->current_index_filtered() << timeline_dock->timelineWidget()->current_node();
    });

    // Timeline
    QMenu* menu_timeline = new QMenu("Timeline", menu_debug);
    menu_debug->addAction(menu_timeline->menuAction());
    menu_timeline->addAction("Print lines", [this]{
        timeline_dock->timelineWidget()->timeline()->debug_lines();
    });
    QAction* toggle_timeline_debug = menu_timeline->addAction("Debug view");
    toggle_timeline_debug->setCheckable(true);
    connect(toggle_timeline_debug, &QAction::toggled, parent, [this](bool on){
        timeline_dock->timelineWidget()->timeline()->toggle_debug(on);
        GlaxnimateSettings::setDebug_timeline(on);
    });
    toggle_timeline_debug->setChecked(GlaxnimateSettings::debug_timeline());

    // Timeline
    QMenu* menu_canvas = new QMenu("Canvas", menu_debug);
    menu_debug->addAction(menu_canvas->menuAction());
    menu_canvas->addAction("Debug Scene", [this]{ scene.debug();});

    // Screenshot
    QMenu* menu_screenshot = new QMenu("Screenshot", menu_debug);
    menu_debug->addAction(menu_screenshot->menuAction());
    menu_screenshot->addAction("Menus", [this]{
        QDir("/tmp/").mkpath("glaxnimate/menus");
        for ( auto widget : this->parent->findChildren<QMenu*>() )
            screenshot_widget("/tmp/glaxnimate/menus/", widget);
    });
    menu_screenshot->addAction("Toolbars", [this]{
        QDir("/tmp/").mkpath("glaxnimate/toolbars");
        for ( auto widget : this->parent->findChildren<QToolBar*>() )
            screenshot_widget("/tmp/glaxnimate/toolbars/", widget);
    });
    menu_screenshot->addAction("Docks", [this]{
        auto state = parent->saveState();

        QDir("/tmp/").mkpath("glaxnimate/docks");
        for ( auto widget : this->parent->findChildren<QDockWidget*>() )
        {
            widget->setFloating(true);
            screenshot_widget("/tmp/glaxnimate/docks/", widget);
        }

        parent->restoreState(state);
    });

    // Source View
    QMenu* menu_source = new QMenu("View Source", menu_debug);
    menu_debug->addAction(menu_source->menuAction());
    menu_source->addAction("Raw", [this]{
        open_file(current_document->io_options().filename);
    });
    menu_source->addAction("Pretty", [this]{
        QString filename = current_document->io_options().filename;
        auto fmt = current_document->io_options().format;
        if ( !fmt )
            return;

        QFile file(filename);
        if ( !file.open(QIODevice::ReadOnly) )
            return;
        QByteArray data = file.readAll();


        if ( fmt->slug() == "lottie" || fmt->slug() == "glaxnimate" )
        {
            open_file(pretty_json(data));
        }
        else if ( fmt->slug() == "tgs" )
        {
            QByteArray decomp;
            utils::gzip::decompress(data, decomp, {});
            open_file(pretty_json(decomp));
        }
        else if ( fmt->slug() == "svg" )
        {
            if ( utils::gzip::is_compressed(data) )
            {
                QByteArray decomp;
                utils::gzip::decompress(data, decomp, {});
                data = std::move(decomp);
            }

            open_file(pretty_xml(data));
        }
        else if ( fmt->slug() == "rive" )
        {
            open_file(pretty_rive(data));
        }
        else
        {
            open_file(filename);
        }
    });
    menu_source->addAction("Current (Rawr)", [this]{
        json_to_pretty_temp(io::glaxnimate::GlaxnimateFormat().to_json(current_document.get()));
    });
    menu_source->addAction("Current (Lottie)", [this]{
        json_to_pretty_temp(
            QJsonDocument(io::lottie::LottieFormat().to_json(comp).toJsonObject())
        );
    });
    menu_source->addAction("Current (RIVE)", [this]{
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        io::rive::RiveFormat().save(buffer, "", comp, {});
        json_to_pretty_temp(io::rive::RiveFormat().to_json(buffer.data()));
    });

    // Misc
    menu_debug->addAction("Inspect Clipboard", []{
        auto dialog = new ClipboardInspector();
        dialog->show();
        connect(dialog, &QDialog::finished, dialog, &QObject::deleteLater);
    });
    menu_debug->addAction("Fonts", []{
        qDebug() << "---- Fonts ----";

        auto families = QFontDatabase::families();

        for ( const auto& family : families )
            qDebug() << family;

        qDebug() << "---- Custom ----";
        for ( const auto& font : model::CustomFontDatabase::instance().fonts() )
            qDebug() << font.family() << ":" << font.style_name();

        qDebug() << "---- Aliases ----";
        for ( const auto& p : model::CustomFontDatabase::instance().aliases() )
        {
            auto db = qDebug();
            db << p.first << "->";
            for ( const auto& n : p.second )
                db << n;
        }
        qDebug() << "----";
    });
    menu_debug->addAction("Force Autosave", [this]{autosave(true);});

    menu_debug->addAction("Inspect undo command", [this]{
        debug_cmd(current_document->undo_stack().command(current_document->undo_stack().index() - 1), "");
    });
    //menu_debug->addAction("Crash", []{volatile int* np = nullptr;*np = 123;});
}
