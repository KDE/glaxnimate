#include "render_widget.hpp"

#include <QPainter>
#include <QVBoxLayout>
#include <QScrollBar>

#include "app/application.hpp"

using namespace glaxnimate;

class RenderWidgetUtil
{
public:
    model::Composition* composition = nullptr;
    std::unique_ptr<renderer::Renderer> renderer;
    QTransform world_transform;
    QPointF offset;
    QGraphicsView* view = nullptr;

    RenderWidgetUtil(std::unique_ptr<renderer::Renderer> renderer) :
        renderer(std::move(renderer))
    {}

    void update_transform()
    {
        if ( !view )
            return;
        world_transform.reset();
        QScrollBar* hb = view->horizontalScrollBar();
        QScrollBar* vb = view->verticalScrollBar();
        world_transform.translate(-hb->value(), -vb->value());
        world_transform = view->transform() * world_transform;
    }

    void render_composition()
    {
        if ( !composition )
            return;

        renderer->render_start();
        renderer->transform(world_transform);
        composition->paint(renderer.get(), composition->time(), model::VisualNode::Canvas);
        renderer->render_end();
    }
};


template<class BaseWidget>
class BasicRenderWidget : public BaseWidget, public RenderWidgetUtil
{
public:
    QBrush back;

    BasicRenderWidget(QWidget* widget, std::unique_ptr<renderer::Renderer> renderer) :
        BaseWidget(widget), RenderWidgetUtil(std::move(renderer))
    {
        back.setTexture(QPixmap(app::Application::instance()->data_file("images/widgets/background.png")));
    }

protected:
    void paintEvent(QPaintEvent* ) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(0, 0, this->width(), this->height(), this->palette().base());

        if ( !composition )
            return;

        update_transform();

        painter.setTransform(world_transform);
        painter.fillRect(QRectF(QPointF(0, 0), composition->size()), back);
        painter.setTransform({});

        QImage img(this->width(), this->height(), QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        renderer->set_image_surface(&img);
        render_composition();
        painter.drawImage(0, 0, img);
    }

};

class glaxnimate::gui::RenderWidget::Private
{
public:
    QWidget* widget;
    RenderWidgetUtil* util;
    QVBoxLayout* layout;

    Private(QWidget* parent)
    {
        // TODO setting for the render quality
        auto renderer = renderer::default_renderer(5);
        auto wid = new BasicRenderWidget<QWidget>(parent, std::move(renderer));
        this->widget = wid;
        this->util = wid;

        layout = new QVBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
    }
};


glaxnimate::gui::RenderWidget::RenderWidget(QWidget* parent)
    : d(std::make_unique<Private>(parent))
{
}

glaxnimate::gui::RenderWidget::RenderWidget() = default;
glaxnimate::gui::RenderWidget::RenderWidget(RenderWidget&& o) : d(std::move(o.d)) {}
glaxnimate::gui::RenderWidget& glaxnimate::gui::RenderWidget::operator=(RenderWidget&& o)
{
    std::swap(o.d, d);
    return *this;
}


QWidget * glaxnimate::gui::RenderWidget::widget() const
{
    return d->widget;
}

void glaxnimate::gui::RenderWidget::set_composition(model::Composition* comp)
{
    d->util->composition = comp;
}

void glaxnimate::gui::RenderWidget::render()
{
    d->widget->update();
}

void glaxnimate::gui::RenderWidget::set_overlay(QGraphicsView* view)
{
    d->util->view = view;
    d->layout->addWidget(view);
}


glaxnimate::gui::RenderWidget::~RenderWidget() noexcept = default;

