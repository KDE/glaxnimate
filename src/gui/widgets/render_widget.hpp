/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <memory>
#include <QGraphicsView>

#include "model/assets/composition.hpp"

namespace glaxnimate::gui {

class RenderWidget
{
public:
    RenderWidget();
    explicit RenderWidget(QWidget* parent);
    RenderWidget(RenderWidget&&);
    RenderWidget& operator=(RenderWidget&&);
    ~RenderWidget() noexcept;

    QWidget* widget() const;
    void set_overlay(QGraphicsView* view);
    void set_composition(model::Composition* comp);
    void render();
    // TODO IPC image stuff

private:
    class Private;
    std::unique_ptr<Private> d;
};

} // namespace glaxnimate::gui
