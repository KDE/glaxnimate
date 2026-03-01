/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "render_widget.hpp"

#include <QPainter>
#include <QVBoxLayout>
#include <QScrollBar>

#include "app/application.hpp"

using namespace glaxnimate;


class glaxnimate::gui::RenderWidget::Private
{
public:
    model::Composition* composition = nullptr;
    std::unique_ptr<renderer::Renderer> renderer;
    QTransform world_transform;
    QPointF offset;
    QImage background;
    QRectF background_target;
    RenderWidget* emitter;

    QWidget* widget;
    QGraphicsView* view = nullptr;
    QVBoxLayout* layout;

    Private(RenderWidget* emitter, QWidget* parent)
        : emitter(emitter)
    {
        init_renderer(parent);
        layout = new QVBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
    }

    void init_renderer(QWidget* parent);


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


namespace {

class BasicRenderWidget : public QWidget
{
public:
    QBrush back;
    glaxnimate::gui::RenderWidget::Private* d;

    BasicRenderWidget(QWidget* parent, glaxnimate::gui::RenderWidget::Private* d) :
        QWidget(parent), d(d)
    {
        back.setTexture(QPixmap(app::Application::instance()->data_file("images/widgets/background.png")));
    }

protected:
    void render_composition()
    {
        d->renderer->render_start();
        d->renderer->transform(d->world_transform);
        d->composition->paint(d->renderer.get(), d->composition->time(), model::VisualNode::Canvas);
        d->renderer->render_end();
    }

    void paintEvent(QPaintEvent* ) override
    {
        Q_EMIT d->emitter->request_background();

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(0, 0, this->width(), this->height(), this->palette().base());

        if ( !d->composition )
            return;

        d->update_transform();

        painter.setTransform(d->world_transform);
        painter.fillRect(QRectF(QPointF(0, 0), d->composition->size()), back);
        if ( !d->background.isNull() )
            painter.drawImage(d->background_target, d->background);
        painter.setTransform({});

        if ( d->renderer->set_painter_surface(&painter, width(), height()) )
        {
            render_composition();
        }
        else
        {
            QImage img(this->width(), this->height(), QImage::Format_ARGB32_Premultiplied);
            img.fill(Qt::transparent);
            d->renderer->set_image_surface(&img);
            render_composition();
            painter.drawImage(0, 0, img);
        }
    }

};

} // namespace

#ifdef OPENGL_ENABLED


#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QPaintEvent>

namespace {

class OpenGlRenderWidget : public QOpenGLWidget
{
    QImage back;
    glaxnimate::gui::RenderWidget::Private* d;

public:
    explicit OpenGlRenderWidget(QWidget* parent, glaxnimate::gui::RenderWidget::Private* d) :
        QOpenGLWidget(parent), d(d)
    {
        setUpdatesEnabled(true);
        back = QImage(app::Application::instance()->data_file("images/widgets/background.png")).convertedTo(QImage::Format_ARGB32_Premultiplied);
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
    /*void initializeGL() override
    {
        initializeOpenGLFunctions();
    }*/

    void resizeGL(int w, int h) override
    {
        d->renderer->set_gl_surface(native_gl_context(), defaultFramebufferObject(), w, h);
    }

    void paintGL() override
    {
        Q_EMIT d->emitter->request_background();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        d->renderer->render_start();

        // Background
        d->renderer->fill_rect(QRectF(0, 0, width(), height()), palette().base());

        if ( !d->composition )
        {
            d->renderer->render_end();
            return;
        }

        d->renderer->layer_start();
        d->update_transform();
        d->renderer->transform(d->world_transform);
        d->renderer->fill_pattern(QRectF(0, 0, d->composition->width.get(), d->composition->height.get()), back);
        if ( !d->background.isNull() )
            d->renderer->draw_image(d->background, d->background_target);
        d->composition->paint(d->renderer.get(), d->composition->time(), model::VisualNode::Canvas);
        d->renderer->layer_end();
        d->renderer->render_end();
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
#else

using OpenGlRenderWidget = BasicRenderWidget;

#endif

void glaxnimate::gui::RenderWidget::Private::init_renderer(QWidget* parent)
{
    // TODO setting for the render quality
    renderer = renderer::default_renderer(5);
    if ( renderer->supports_surface(renderer::OpenGL) )
    {
        widget = new OpenGlRenderWidget(parent, this);
    }
    else
    {
        widget = new BasicRenderWidget(parent, this);
    }
}


glaxnimate::gui::RenderWidget::RenderWidget(QWidget* parent)
    : d(std::make_unique<Private>(this, parent))
{
}

glaxnimate::gui::RenderWidget::RenderWidget() = default;
glaxnimate::gui::RenderWidget::RenderWidget(RenderWidget&& o) : d(std::move(o.d))
{
    d->emitter = this;
}
glaxnimate::gui::RenderWidget& glaxnimate::gui::RenderWidget::operator=(RenderWidget&& o)
{
    std::swap(o.d, d);
    d->emitter = this;
    return *this;
}


QWidget * glaxnimate::gui::RenderWidget::widget() const
{
    return d->widget;
}

void glaxnimate::gui::RenderWidget::set_composition(model::Composition* comp)
{
    d->composition = comp;
}

void glaxnimate::gui::RenderWidget::render()
{
    d->widget->update();
}

void glaxnimate::gui::RenderWidget::set_overlay(QGraphicsView* view)
{
    d->view = view;
    d->layout->addWidget(view);
    view->installEventFilter(d->widget);
}


glaxnimate::gui::RenderWidget::~RenderWidget() noexcept = default;

void glaxnimate::gui::RenderWidget::set_background(QImage image, const QRectF& target)
{
    d->background = std::move(image);
    d->background_target = target;
}

void glaxnimate::gui::RenderWidget::clear_background()
{
    d->background = {};
}

