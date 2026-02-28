#include "render_widget.hpp"

#include <QPainter>
#include <QVBoxLayout>
#include <QScrollBar>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QPaintEvent>

#include "app/application.hpp"

using namespace glaxnimate;

namespace {

class RenderWidgetUtil
{
public:
    model::Composition* composition = nullptr;
    std::unique_ptr<renderer::Renderer> renderer;
    QTransform world_transform;
    QPointF offset;
    QGraphicsView* view = nullptr;
    QBrush back;

    RenderWidgetUtil(std::unique_ptr<renderer::Renderer> renderer) :
        renderer(std::move(renderer))
    {
        back.setTexture(QPixmap(app::Application::instance()->data_file("images/widgets/background.png")));
    }

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
};


class BasicRenderWidget : public QWidget, public RenderWidgetUtil
{
public:

    BasicRenderWidget(QWidget* parent, std::unique_ptr<renderer::Renderer> renderer) :
        QWidget(parent), RenderWidgetUtil(std::move(renderer))
    {}

protected:
    void render_composition()
    {
        renderer->render_start();
        renderer->transform(world_transform);
        composition->paint(renderer.get(), composition->time(), model::VisualNode::Canvas);
        renderer->render_end();
    }

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

        if ( renderer->set_painter_surface(&painter, width(), height()) )
        {
            render_composition();
        }
        else
        {
            QImage img(this->width(), this->height(), QImage::Format_ARGB32_Premultiplied);
            img.fill(Qt::transparent);
            renderer->set_image_surface(&img);
            render_composition();
            painter.drawImage(0, 0, img);
        }
    }

};

class OpenGlRenderWidget : public QOpenGLWidget, public QOpenGLFunctions, public RenderWidgetUtil
{
public:
    explicit OpenGlRenderWidget(QWidget* parent, std::unique_ptr<renderer::Renderer> renderer) :
        QOpenGLWidget(parent), RenderWidgetUtil(std::move(renderer))
    {
        setUpdatesEnabled(true);
    }

    void* native_gl_context()
    {
        auto* context = this->context();
        if ( !context )
            return nullptr;

#if defined(Q_OS_WIN)
        auto* iface = context->nativeInterface<QNativeInterface::QWGLContext>();
        return iface ? iface->nativeContext() : nullptr;
#elif defined(Q_OS_MACOS)
        auto* iface = context->nativeInterface<QNativeInterface::QCocoaGLContext>();
        return iface ? iface->nativeContext() : nullptr;
#else
        auto* eglIface = context->nativeInterface<QNativeInterface::QEGLContext>();
        if (eglIface) return eglIface->nativeContext();

        auto* glxIface = context->nativeInterface<QNativeInterface::QGLXContext>();
        return glxIface ? glxIface->nativeContext() : nullptr;
#endif
    }

protected:
    void initializeGL() override
    {
        initializeOpenGLFunctions();
    }

    void resizeGL(int w, int h) override
    {
        renderer->set_gl_surface(native_gl_context(), defaultFramebufferObject(), w, h);
    }

    void paintGL() override
    {
        if ( !composition )
            return;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        update_transform();


        renderer->render_start();

        // Background
        renderer->fill_rect(QRectF(0, 0, width(), height()), palette().base());
        // TODO checkered pattern

        renderer->layer_start();
        renderer->transform(world_transform);
        composition->paint(renderer.get(), composition->time(), model::VisualNode::Canvas);
        renderer->layer_end();
        renderer->render_end();
    }

    bool force_paint = false;
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if ( event->type() == QEvent::Paint && !force_paint )
        {
            force_paint = true;
            QPaintEvent pe(rect());
            paintEvent(&pe);
            force_paint = false;
        }
        return QOpenGLWidget::eventFilter(watched, event);
    }

};

} // namespace

class glaxnimate::gui::RenderWidget::Private
{
public:
    QWidget* widget;
    RenderWidgetUtil* util;
    QVBoxLayout* layout;

    void init_renderer(QWidget* parent)
    {
        // TODO setting for the render quality
        auto renderer = renderer::default_renderer(5);
        if ( renderer->supports_surface(renderer::OpenGL) )
        {
            auto wid = new OpenGlRenderWidget(parent, std::move(renderer));
            widget = wid;
            util = wid;
        }
        else
        {
            auto wid = new BasicRenderWidget(parent, std::move(renderer));
            widget = wid;
            util = wid;
        }
    }

    Private(QWidget* parent)
    {
        init_renderer(parent);
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
    view->installEventFilter(d->widget);
}


glaxnimate::gui::RenderWidget::~RenderWidget() noexcept = default;

