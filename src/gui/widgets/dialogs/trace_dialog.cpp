#include "trace_dialog.hpp"
#include "ui_trace_dialog.h"

#include <vector>
#include <algorithm>
#include <unordered_map>

#include <QEvent>
#include <QGraphicsScene>
#include <QStandardItemModel>

#include <QtColorWidgets/ColorDelegate>

#include "model/defs/bitmap.hpp"
#include "model/shapes/group.hpp"
#include "model/shapes/layer.hpp"
#include "model/shapes/path.hpp"
#include "model/shapes/fill.hpp"
#include "model/shapes/image.hpp"
#include "utils/trace.hpp"
#include "command/undo_macro_guard.hpp"
#include "command/object_list_commands.hpp"

class TraceDialog::Private
{
public:
    struct TraceResult
    {
        QColor color;
        math::bezier::MultiBezier bezier;
    };

    enum Mode
    {
        Alpha,
        Closest,
        Exact,
        Pixel
    };

    model::Image* image;
    model::Group* created = nullptr;
    QImage source_image;
    Ui::TraceDialog ui;
    QGraphicsScene scene;
    utils::trace::TraceOptions options;
    color_widgets::ColorDelegate delegate;

    void trace_mono(std::vector<TraceResult>& result)
    {
        result.resize(1);
        ui.progress_bar->setMaximum(100);

        result[0].color = ui.color_mono->color();
        utils::trace::Tracer tracer(source_image, options);
        tracer.set_target_alpha(ui.spin_alpha_threshold->value(), ui.check_inverted->isChecked());
        connect(&tracer, &utils::trace::Tracer::progress, ui.progress_bar, &QProgressBar::setValue);
        tracer.set_progress_range(0, 100);
        tracer.trace(result[0].bezier);
    }

    void trace_exact(std::vector<TraceResult>& result)
    {
        int count = ui.list_colors->model()->rowCount();
        result.resize(count);
        ui.progress_bar->setMaximum(100 * count);

        for ( int i = 0; i < count; i++ )
        {
            result[i].color = ui.list_colors->item(i)->data(Qt::DisplayRole).value<QColor>();
            utils::trace::Tracer tracer(source_image, options);
            tracer.set_target_color(result[i].color);
            connect(&tracer, &utils::trace::Tracer::progress, ui.progress_bar, &QProgressBar::setValue);
            tracer.set_progress_range(100 * i, 100 * (i+1));
            tracer.trace(result[i].bezier);
        }
    }

    void trace_closest(std::vector<TraceResult>& result)
    {
        int count = ui.list_colors->model()->rowCount();
        result.resize(count);
        ui.progress_bar->setMaximum(100 * count);

        QVector<QRgb> colors;
        colors.reserve(count+1);
        for ( int i = 0; i < count; i++ )
        {
            result[i].color = ui.list_colors->item(i)->data(Qt::DisplayRole).value<QColor>();
            colors.push_back(result[i].color.rgb());
        }
        colors.push_back(qRgba(0, 0, 0, 0));
        QImage converted = source_image.convertToFormat(QImage::Format_Indexed8, colors, Qt::ThresholdDither);

        for ( int i = 0; i < count; i++ )
        {
            utils::trace::Tracer tracer(converted, options);
            tracer.set_target_index(i);
            connect(&tracer, &utils::trace::Tracer::progress, ui.progress_bar, &QProgressBar::setValue);
            tracer.set_progress_range(100 * i, 100 * (i+1));
            tracer.trace(result[i].bezier);
        }
    }

    std::vector<TraceResult> trace()
    {
        options.set_min_area(ui.spin_min_area->value());
        options.set_smoothness(ui.spin_smoothness->value() / 100.0);

        std::vector<TraceResult> result;

        ui.progress_bar->show();
        ui.progress_bar->setValue(0);

        switch ( ui.combo_mode->currentIndex() )
        {
            case Mode::Alpha: trace_mono(result); break;
            case Mode::Closest: trace_closest(result); break;
            case Mode::Exact: trace_exact(result); break;
        }


        ui.progress_bar->hide();
        return result;
    }

    void result_to_shapes(model::ShapeListProperty& prop, const TraceResult& result)
    {
        auto fill = std::make_unique<model::Fill>(image->document());
        fill->color.set(result.color);
        prop.insert(std::move(fill));

        for ( const auto& bez : result.bezier.beziers() )
        {
            auto path = std::make_unique<model::Path>(image->document());
            path->shape.set(bez);
            prop.insert(std::move(path));
        }
    }

    void add_color(const QColor& c = Qt::black)
    {
        auto item = new QListWidgetItem();
        item->setData(Qt::EditRole, c);
        item->setData(Qt::DisplayRole, c);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui.list_colors->addItem(item);
    }
};

TraceDialog::TraceDialog(model::Image* image, QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);
    d->image = image;
    d->source_image = image->image->pixmap().toImage();
    if ( d->source_image.format() != QImage::Format_RGBA8888 )
        d->source_image = d->source_image.convertToFormat(QImage::Format_RGBA8888);

    d->ui.preview->setScene(&d->scene);
    d->ui.spin_min_area->setValue(d->options.min_area());
    d->ui.spin_smoothness->setValue(d->options.smoothness() * 100);
    d->ui.progress_bar->hide();

    d->ui.list_colors->setItemDelegate(&d->delegate);

    auto item = static_cast<QStandardItemModel*>(d->ui.combo_mode->model())->item(Private::Pixel);
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    d->ui.combo_mode->setCurrentIndex(Private::Closest);

    d->ui.spin_color_count->setValue(4);
    auto_colors();
}

TraceDialog::~TraceDialog() = default;

void TraceDialog::changeEvent ( QEvent* e )
{
    QDialog::changeEvent(e);

    if ( e->type() == QEvent::LanguageChange)
    {
        d->ui.retranslateUi(this);
    }
}

void TraceDialog::update_preview()
{
    d->scene.clear();

    for ( const auto& result : d->trace() )
    {
        d->scene.addPath(result.bezier.painter_path(), Qt::NoPen, result.color);
    }

    d->ui.preview->fitInView(d->scene.sceneRect(), Qt::KeepAspectRatio);
}

void TraceDialog::apply()
{
    auto trace = d->trace();

    auto layer = std::make_unique<model::Group>(d->image->document());
    d->created = layer.get();
    layer->name.set(tr("Traced %1").arg(d->image->object_name()));
    layer->transform->copy(d->image->transform.get());

    if ( trace.size() == 1 )
    {
        d->result_to_shapes(layer->shapes, trace[0]);
    }
    else
    {
        for ( const auto& result : trace )
        {
            auto group = std::make_unique<model::Group>(d->image->document());
            group->name.set(result.color.name());
            d->result_to_shapes(group->shapes, result);
            layer->shapes.insert(std::move(group));
        }
    }

    d->image->push_command(new command::AddObject<model::ShapeElement>(
        d->image->owner(), std::move(layer), d->image->position()+1
    ));

    d->created->recursive_rename();

    accept();
}

model::DocumentNode * TraceDialog::created() const
{
    return d->created;
}

void TraceDialog::change_mode(int mode)
{
    if ( mode == Private::Alpha )
        d->ui.stacked_widget->setCurrentIndex(0);
    else
        d->ui.stacked_widget->setCurrentIndex(1);
}


void TraceDialog::add_color()
{
    d->add_color();
    d->ui.spin_color_count->setValue(d->ui.list_colors->model()->rowCount());
}

void TraceDialog::remove_color()
{
    int curr = d->ui.list_colors->currentRow();
    if ( curr == -1 )
    {
        curr = d->ui.list_colors->model()->rowCount() - 1;
        if ( curr == -1 )
            return;
    }
    d->ui.list_colors->model()->removeRow(curr);
    d->ui.spin_color_count->setValue(d->ui.list_colors->model()->rowCount());
}

void TraceDialog::auto_colors()
{
    std::unordered_map<QRgb, int> count;
    const uchar* data = d->source_image.constBits();

    for ( int i = 0, c = d->source_image.width() * d->source_image.height(); i < c; i++ )
        if ( data[i*4+3] > 128 )
            ++count[qRgb(data[i*4], data[i*4+1], data[i*4+2])];

    using Pair = std::pair<QRgb, int>;
    std::vector<Pair> sortme(count.begin(), count.end());
    count.clear();
    std::sort(sortme.begin(), sortme.end(), [](const Pair& a, const Pair& b){ return a.second > b.second; });

    for ( int i = 0; i < qMin<int>(sortme.size(), d->ui.spin_color_count->value()); i++ )
    {
        QColor c(sortme[i].first);

        if ( i >= d->ui.list_colors->model()->rowCount() )
        {
            d->add_color(c);
        }
        else
        {
            auto item = d->ui.list_colors->item(i);
            item->setData(Qt::EditRole, c);
            item->setData(Qt::DisplayRole, c);
        }
    }
}