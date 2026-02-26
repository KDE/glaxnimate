#include "render_widget.hpp"

#include <QPainter>
#include <QVBoxLayout>

using namespace glaxnimate;

class RenderWidgetUtil
{
public:
    model::Composition* composition = nullptr;
    std::unique_ptr<renderer::Renderer> renderer;
    QRectF rect;
    QTransform world_transform;

    RenderWidgetUtil(std::unique_ptr<renderer::Renderer> renderer) :
        renderer(std::move(renderer))
    {}

    void render_composition()
    {
        if ( !composition )
            return;

        renderer->render_start();
        QTransform render_t = world_transform;
        const QTransform& t = world_transform;
        render_t.setMatrix(t.m11(), t.m12(), t.m13(), t.m21(), t.m22(), t.m23(), 0, 0, t.m33());
        renderer->transform(render_t);
        renderer->translate(-rect.left(), -rect.top());
        composition->paint(renderer.get(), composition->time(), model::VisualNode::Canvas);
        renderer->render_end();
    }
};


template<class BaseWidget>
class BasicRenderWidget : public BaseWidget, public RenderWidgetUtil
{
public:
    BasicRenderWidget(QWidget* widget, std::unique_ptr<renderer::Renderer> renderer) :
        BaseWidget(widget), RenderWidgetUtil(std::move(renderer))
    {}

protected:
    void paintEvent(QPaintEvent* ) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(0, 0, this->width(), this->height(), Qt::black);
/*
        const QTransform& t = world_transform;
        // x scale but they should always be the same
        qreal scale = std::sqrt(t.m11() * t.m11() + t.m21() * t.m21());

        QImage img(qCeil(rect.width() * scale), qCeil(rect.height() * scale), QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::transparent);
        // img.fill(QColor(100, 0, 0, 100));
        img.setDevicePixelRatio(scale);

        // Render onto the image
        renderer->set_image_surface(&img);
        render_composition();

        painter.drawImage(rect.topLeft(), img);*/
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

void glaxnimate::gui::RenderWidget::render(const QRectF& exposed_rect, const QTransform& world_tf)
{
    d->util->rect = exposed_rect;
    d->util->world_transform = world_tf;
}

void glaxnimate::gui::RenderWidget::set_overlay(QWidget* view)
{
    d->layout->addWidget(view);
}


glaxnimate::gui::RenderWidget::~RenderWidget() noexcept = default;

