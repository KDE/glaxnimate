/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "validation.hpp"
#include "model/visitor.hpp"
#include "model/shapes/image.hpp"
#include "model/assets/composition.hpp"


using namespace glaxnimate;
using namespace glaxnimate::io::lottie;

void glaxnimate::io::lottie::ValidationVisitor::on_visit_document(model::Document*, model::Composition* main)
{
    if ( !main )
        return;

    if ( fixed_size.isValid() )
    {
        qreal width = main->height.get();
        if ( width != fixed_size.width() )
            fmt->error(i18n("Invalid width: %1, should be %2", width, fixed_size.width()));

        qreal height = main->height.get();
        if ( height != fixed_size.height() )
            fmt->error(i18n("Invalid height: %1, should be %2", height, fixed_size.height()));
    }

    if ( !allowed_fps.empty() )
    {
        qreal fps = main->fps.get();
        if ( std::find(allowed_fps.begin(), allowed_fps.end(), fps) == allowed_fps.end() )
        {
            QStringList allowed;
            for ( auto f : allowed_fps )
                allowed.push_back(QString::number(f));

            fmt->error(i18n("Invalid fps: %1, should be %2", fps, allowed.join(" or ")));
        }
    }

    if ( max_frames > 0 )
    {
        auto duration = main->animation->duration();
        if ( duration > max_frames )
            fmt->error(i18n("Too many frames: %1, should be less than %2", duration, max_frames));
    }
}


namespace {

class DiscordVisitor : public ValidationVisitor
{
public:
    explicit DiscordVisitor(LottieFormat* fmt)
        : ValidationVisitor(fmt)
    {
        allowed_fps.push_back(60);
        fixed_size = QSize(320, 320);
    }

private:
    void on_visit(model::DocumentNode * node) override
    {
        if ( qobject_cast<model::Image*>(node) )
        {
            show_error(node, i18n("Images are not supported"), app::log::Error);
        }
    }
};

} // namespace

void glaxnimate::io::lottie::validate_discord(model::Document* document, model::Composition* main, glaxnimate::io::lottie::LottieFormat* format)
{
    DiscordVisitor(format).visit(document, main);
}

