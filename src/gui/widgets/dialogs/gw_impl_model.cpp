#include "glaxnimate_window_p.hpp"

#include <queue>
#include <QClipboard>

#include "command/shape_commands.hpp"
#include "command/structure_commands.hpp"
#include "app/settings/widget_builder.hpp"
#include "model/shapes/group.hpp"
#include "misc/clipboard_settings.hpp"
#include "widgets/dialogs/shape_parent_dialog.hpp"


model::Composition* GlaxnimateWindow::Private::current_composition()
{
    return current_document->main();
}

model::ShapeElement* GlaxnimateWindow::Private::current_shape()
{
    model::DocumentNode* curr = current_document_node();
    if ( curr )
    {
        if ( auto curr_shape = qobject_cast<model::ShapeElement*>(curr) )
            return curr_shape;
    }
    return nullptr;
}

model::ShapeListProperty* GlaxnimateWindow::Private::current_shape_container()
{
    model::DocumentNode* sh = current_document_node();
    if ( auto lay = qobject_cast<model::Composition*>(sh) )
        return &lay->shapes;

    if ( !qobject_cast<model::Layer*>(sh) )
        sh = sh->docnode_parent();

    while ( sh )
    {
        if ( auto grp = qobject_cast<model::Group*>(sh) )
            return &grp->shapes;
        if ( auto lay = qobject_cast<model::Composition*>(sh) )
            return &lay->shapes;
        sh = sh->docnode_parent();
    }
    return &current_composition()->shapes;
}

model::DocumentNode* GlaxnimateWindow::Private::current_document_node()
{
    if ( auto dn = document_node_model.node(ui.view_document_node->currentIndex()) )
        return dn;
    return current_document->main();
}

void GlaxnimateWindow::Private::set_current_document_node(model::DocumentNode* node)
{
    ui.view_document_node->setCurrentIndex(document_node_model.node_index(node));
}


void GlaxnimateWindow::Private::layer_new_layer()
{
    auto layer = std::make_unique<model::Layer>(current_document.get());
    layer->animation->last_frame.set(current_document->main()->animation->last_frame.get());
    QPointF pos = current_document->rect().center();
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    layer_new_impl(std::move(layer));
}

void GlaxnimateWindow::Private::layer_new_fill()
{
    auto layer = std::make_unique<model::Fill>(current_document.get());
    layer->color.set(ui.fill_style_widget->current_color());
    layer_new_impl(std::move(layer));
}

void GlaxnimateWindow::Private::layer_new_stroke()
{
    auto layer = std::make_unique<model::Stroke>(current_document.get());
    layer->set_pen_style(ui.stroke_style_widget->pen_style());
    layer_new_impl(std::move(layer));
}

void GlaxnimateWindow::Private::layer_new_group()
{
    auto layer = std::make_unique<model::Group>(current_document.get());
    QPointF pos = current_document->rect().center();
    layer->transform.get()->anchor_point.set(pos);
    layer->transform.get()->position.set(pos);
    layer_new_impl(std::move(layer));
}

void GlaxnimateWindow::Private::layer_new_impl(std::unique_ptr<model::ShapeElement> layer)
{
    current_document->set_best_name(layer.get(), {});
    layer->set_time(current_document_node()->time());

    model::ShapeElement* ptr = layer.get();

    auto cont = current_shape_container();
    int position = cont->index_of(current_shape());
    current_document->push_command(new command::AddShape(cont, std::move(layer), position));

    ui.view_document_node->setCurrentIndex(document_node_model.node_index(ptr));
}

void GlaxnimateWindow::Private::layer_delete()
{
    auto current = current_shape();
    if ( !current || current->docnode_locked() )
        return;
    current->push_command(new command::RemoveShape(current, current->owner()));
}

void GlaxnimateWindow::Private::layer_duplicate()
{
    auto current = current_shape();
    if ( !current )
        return;
    current->push_command(command::duplicate_shape(current));
}

std::vector<model::DocumentNode *> GlaxnimateWindow::Private::cleaned_selection()
{
    return scene.cleaned_selection();
}


void GlaxnimateWindow::Private::delete_selected()
{
    auto selection = cleaned_selection();
    if ( selection.empty() )
        return;

    current_document->undo_stack().beginMacro(tr("Delete"));
    for ( auto item : selection )
    {
        if ( auto shape = qobject_cast<model::Shape*>(item) )
            if ( !shape->docnode_locked() )
                current_document->push_command(new command::RemoveShape(shape, shape->owner()));
    }
    current_document->undo_stack().endMacro();
}

void GlaxnimateWindow::Private::cut()
{
    auto selection = copy();
    if ( selection.empty() )
        return;

    current_document->undo_stack().beginMacro(tr("Cut"));
    for ( auto item : selection )
    {
        if ( auto shape = qobject_cast<model::Shape*>(item) )
            if ( !shape->docnode_locked() )
                current_document->push_command(new command::RemoveShape(shape, shape->owner()));
    }
    current_document->undo_stack().endMacro();
}

std::vector<model::DocumentNode*> GlaxnimateWindow::Private::copy()
{
    auto selection = cleaned_selection();

    if ( !selection.empty() )
    {
        QMimeData* data = new QMimeData;
        for ( const auto& mime : ClipboardSettings::mime_types() )
        {
            if ( mime.enabled )
                mime.serializer->to_mime_data(*data, selection);
        }

        QGuiApplication::clipboard()->setMimeData(data);
    }

    return selection;
}

void GlaxnimateWindow::Private::paste()
{
    const QMimeData* data = QGuiApplication::clipboard()->mimeData();
    io::mime::DeserializedData raw_pasted;
    for ( const auto& mime : ClipboardSettings::mime_types() )
    {
        if ( mime.enabled )
        {
            raw_pasted = mime.serializer->from_mime_data(*data, current_document.get());
            if ( !raw_pasted.empty() )
                break;
        }
    }
    if ( raw_pasted.empty() )
    {
        status_message(tr("Nothing to paste"));
        return;
    }

    /// \todo precompositions
    for ( auto it = raw_pasted.compositions.begin(); it != raw_pasted.compositions.end(); )
    {
        if ( auto main_comp = qobject_cast<model::MainComposition*>(it->get()) )
        {
            raw_pasted.shapes.reserve(raw_pasted.shapes.size() + main_comp->shapes.size());
            auto raw = main_comp->shapes.raw();
            raw_pasted.shapes.insert(raw_pasted.shapes.end(), raw.move_begin(), raw.move_end());
            raw.clear();
            it = raw_pasted.compositions.erase(it);
        }
        else
        {
            ++it;
        }
    }

    current_document->undo_stack().beginMacro(tr("Paste"));

    model::ShapeListProperty* shape_cont = current_shape_container();

    std::vector<model::DocumentNode*> select;
    if ( !raw_pasted.shapes.empty() )
    {
        int shape_insertion_point = shape_cont->size();
        for ( auto& shape : raw_pasted.shapes )
        {
            select.push_back(shape.get());
            shape->recursive_rename();
            current_document->push_command(new command::AddShape(shape_cont, std::move(shape), shape_insertion_point++));
        }
    }

    for ( auto& color : raw_pasted.named_colors )
    {
        current_document->push_command(new command::AddObject(
            &current_document->defs()->colors,
            std::move(color),
            current_document->defs()->colors.size()
        ));
    }

    current_document->undo_stack().endMacro();

    QItemSelection item_select;
    for ( auto node : select )
    {
        item_select.push_back(QItemSelectionRange(document_node_model.node_index(node)));
    }
    ui.view_document_node->selectionModel()->select(item_select, QItemSelectionModel::ClearAndSelect|QItemSelectionModel::Rows);
}


void GlaxnimateWindow::Private::move_current(command::ReorderCommand::SpecialPosition pos)
{
    auto current = current_shape();
    if ( !current )
        return;
    auto cmd = std::make_unique<command::ReorderCommand>(current, pos);
    if ( !cmd->has_action() )
        return;
    current->push_command(cmd.release());
}

void GlaxnimateWindow::Private::group_shapes()
{
    auto data = command::GroupShapes::collect_shapes(cleaned_selection());
    if ( data.parent )
        current_document->push_command(
            new command::GroupShapes(data)
        );
}

void GlaxnimateWindow::Private::ungroup_shapes()
{
    model::Group* group = qobject_cast<model::Group*>(current_document_node());

    if ( !group )
    {
        auto sp = current_shape_container();
        if ( !sp )
            return;
        group = qobject_cast<model::Group*>(sp->object());
    }

    if ( group )
        current_document->push_command(new command::UngroupShapes(group));
}


void GlaxnimateWindow::Private::move_to()
{
    auto sel = cleaned_selection();
    std::vector<model::ShapeElement*> shapes;
    shapes.reserve(sel.size());
    for ( const auto& node : sel )
    {
        if ( auto shape = qobject_cast<model::ShapeElement*>(node) )
            shapes.push_back(shape);
    }

    if ( shapes.empty() )
        return;


    if ( auto parent = ShapeParentDialog(&document_node_model, this->parent).get_shape_parent() )
    {
        current_document->undo_stack().beginMacro(tr("Move Shapes"));
        for ( auto shape : shapes )
            if ( shape->owner() != parent )
                shape->push_command(new command::MoveShape(shape, shape->owner(), parent, parent->size()));
        current_document->undo_stack().endMacro();
    }
}
