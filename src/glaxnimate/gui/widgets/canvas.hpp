#pragma once

#include <QGraphicsView>
#include <QMimeData>

#include <memory>

#include "widgets/dialogs/selection_manager.hpp"

namespace glaxnimate::gui::tools {
class Tool;
} // namespace glaxnimate::gui::tools

namespace glaxnimate::gui {


class Canvas : public QGraphicsView
{
    Q_OBJECT

public:
    Canvas(QWidget* parent = nullptr);
    ~Canvas();

public:
    /**
     *  \brief Get the global zoom factor
     *
     *  \return A value representing the scaling factor, 1 = 100%
    */
    qreal get_zoom_factor() const;


    /// Overload QGraphicsView::translate
    void translate(const QPointF& d);

    void set_active_tool(tools::Tool* tool);
    void set_tool_target(glaxnimate::gui::SelectionManager* window);

public slots:
    /**
     *  \brief Translate and resize sceneRect
     *
     *  Translate the scene, if the result is not contained within sceneRect,
     *  the sceneRect is expanded
     *
     *  \param delta Translation amount
     */
    void translate_view(const QPointF& delta);

    /**
     * \brief Zoom view by factor
     *
     *  The zooming is performed relative to the current transformation
     *
     *  \param factor scaling factor ( 1 = don't zoom )
     */
    void zoom_view(qreal factor);

    /**
     * \brief Zoom view by factor
     *
     *  The zooming is performed relative to the current transformation
     *
     *  \param factor scaling factor ( 1 = don't zoom )
     *  \param anchor Point to keep stable (scene coords)
     */
    void zoom_view_anchor(qreal factor, const QPointF& anchor);

    /**
     * \brief Set zoom factor
     *
     *  The zooming is performed absolutely
     *
     *  \param factor scaling factor ( 1 = no zoom )
     */
    void set_zoom(qreal factor);

    /**
     * \brief Set zoom factor
     *
     *  The zooming is performed absolutely
     *
     *  \param factor scaling factor ( 1 = no zoom )
     *  \param anchor Point to keep stable (scene coords)
     */
    void set_zoom_anchor(qreal factor, const QPointF& anchor);

    /**
     * \brief Flips the view horizontally
     */
    void flip_horizontal();

    void zoom_in();
    void zoom_out();
    void reset_zoom() { set_zoom(1); }

    void set_rotation(qreal radians);
    void reset_rotation() { set_rotation(0); }

    void view_fit();

signals:
    /**
     *  \brief Emitted when zoom is changed
     *  \param percent Zoom percentage
     */
    void zoomed(qreal percent);

    /**
     *  \brief Emitted when rotation is changed
     *  \param angle in radians
     */
    void rotated(qreal angle);

    void dropped(const QMimeData* data);

    void mouse_moved(const QPointF& p);

protected:
    void mousePressEvent(QMouseEvent * event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void mouseDoubleClickEvent(QMouseEvent * event) override;
    void wheelEvent(QWheelEvent * event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent * event) override;
    void keyReleaseEvent(QKeyEvent * event) override;
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent * event) override;
    void dragMoveEvent(QDragMoveEvent * event) override;
    void dropEvent(QDropEvent * event) override;

    bool event(QEvent* event) override;
    bool viewportEvent(QEvent *event) override;

private:
    void do_rotate(qreal radians, const QPointF& scene_anchor);

    class Private;
    std::unique_ptr<Private> d;
};


} // namespace glaxnimate::gui
