/*
 * SPDX-FileCopyrightText: 2012-2026 Mattia Basaglia <dev@dragon.best>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef DOCK_GRID_HPP
#define DOCK_GRID_HPP

#include <memory>
#include <QDockWidget>
#include "graphics/snapping_grid.hpp"

namespace glaxnimate::gui {

class DockGrid : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockGrid(QWidget *parent = nullptr);
    ~DockGrid();

    void set_grid(SnappingGrid* target_grid);

    /// if grid is target grid, clear target grid
    void unset_grid(SnappingGrid* grid);

protected:
    void changeEvent(QEvent *e) override;

Q_SIGNALS:
    /// Emitted when the user want to move the grid with the mouse
    void move_grid();

private Q_SLOTS:
    void position_spin_changed();
    void grid_moved(QPointF p);
    void on_button_reset_clicked();
    void on_combo_shape_currentIndexChanged(int index);
    void on_check_enable_toggled(bool arg1);
    void on_spin_size_valueChanged(int size);

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui

#endif // DOCK_GRID_HPP
