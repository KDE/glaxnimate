/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "help_dialog.hpp"

#include <QScrollArea>
#include <QLabel>
#include <QGridLayout>
#include <QFrame>

#include "io/io_registry.hpp"
#include "style/scroll_area_event_filter.hpp"
#include "glaxnimate_app.hpp"

glaxnimate::android::HelpDialog::HelpDialog(QWidget *parent)
    : BaseDialog(parent)
{
    QGridLayout* ml = new QGridLayout(this);
    ml->setContentsMargins(0, 0, 0, 0);
    setLayout(ml);

    QScrollArea* area = new QScrollArea(this);
    ml->addWidget(area, 0, 0);
    (new gui::ScrollAreaEventFilter(area))->setParent(area);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setWidgetResizable(true);

    QWidget* wid = new QWidget(area);
    area->setWidget(wid);

    QGridLayout* lay = new QGridLayout(wid);
    lay->setContentsMargins(0, 0, 0, 0);
    wid->setLayout(lay);

    QString supported_formats_import;
    for ( const auto& fmt : io::IoRegistry::instance().importers() )
    {
        if ( fmt->slug() == "raster" )
            continue;
        if ( !supported_formats_import.isEmpty() )
            supported_formats_import += "\n";
        supported_formats_import += " - " + fmt->name();
    }
    QString supported_formats_export = supported_formats_import;
    supported_formats_import += "\n - PNG (Needs to be traced into vector)";

    const std::vector<std::pair<QString, QString>> buttons = {
        {
            "document-new",
            i18n("Clears the current document and creates a new empty one")
        },
        {
            "document-open",
            i18n("Prompts the user to select a document to open.\nCurrently the following formats are supported:\n%1", supported_formats_import)
        },
        {
            "document-import",
            i18n("Prompts the user to select a document to append as an object to the current one.")
        },
        {
            "document-save",
            i18n("Save the current document, prompting to select a file for new documents")
        },
        {
            "document-save-as",
            i18n("Save the current document, always prompting to select a file.")
        },
        {
            "document-export",
            i18n("Save a copy of current document, prompting to select a format and a file.\nCurrently the following formats are supported:\n%1", supported_formats_export)
        },
        {
            "view-preview",
            i18n("Saves the current frame as a still image")
        },
        {
            "telegram",
            i18n("Creates a sticker pack to export to Telegram.\nNote that only recent version of Telegram support this.\nIf your Telegram version is too old, you can Export the file to TGS and upload that on Telegram.")
        },
        {
            "edit-cut",
            i18n("Cuts the selection into the clipboard.")
        },
        {
            "edit-copy",
            i18n("Copies the selection into the clipboard.")
        },
        {
            "edit-paste",
            i18n("Pastes from the clipboard into the current document.")
        },
        {
            "edit-delete",
            i18n("Removes the selected item.")
        },
        {
            "edit-undo",
            i18n("Undoes the last action.")
        },
        {
            "edit-redo",
            i18n("Redoes the last undone action.")
        },
        {
            "document-properties",
            i18n("Opens the side pane used to change the advanced properties for the selected object.")
        },
        {
            "player-time",
            i18n("Shows the timeline and playback controls.")
        },
        {
            "fill-color",
            i18n("Opens the side pane used to select the fill color.")
        },
        {
            "object-stroke-style",
            i18n("Opens the side pane used to select the stroke color and style.")
        },
        {
            "question",
            i18n("Shows this help.")
        },
        {
            "edit-select",
            i18n("Select tool, you can use it to select objects and change their transform.")
        },
        {
            "edit-node",
            i18n("Edit tool, used to edit existing items (eg: moving, bezier nodes, setting rounded corners on a rectangle, etc.)")
        },
        {
            "draw-brush",
            i18n("Shows a tray with the curve drawing tools.")
        },
        {
            "draw-bezier-curves",
            i18n("Create Bezier curves using nodes and handles.")
        },
        {
            "draw-freehand",
            i18n("Draw curves freehand.")
        },
        {
            "shapes",
            i18n("Shows a tray with the shape drawing tools.")
        },
        {
            "draw-rectangle",
            i18n("Draws rectangles.")
        },
        {
            "draw-ellipse",
            i18n("Draws ellipses.")
        },
        {
            "draw-polygon-star",
            i18n("Draws a star, after it's been created you can change it into a regular polygon and change the number of sides from the properties pane.")
        },
        {
            "draw-text",
            i18n("Create and edit text shapes.")
        },
        {
            "overflow-menu",
            i18n("Toggles the menu showing non-drawing buttons.")
        },
        {
            "media-playback-start",
            i18n("Starts playback.")
        },
        {
            "media-playlist-repeat",
            i18n("Toggles looping for the playback (on by default).")
        },
        {
            "go-first",
            i18n("Jumps to the first frame.")
        },
        {
            "go-previous",
            i18n("Goes to the previous frame.")
        },
        {
            "go-next",
            i18n("Goes to the next frame.")
        },
        {
            "go-last",
            i18n("Jumps to the last frame.")
        },
        {
            "keyframe-record",
            i18n("When enabled (which is the default) whenever you change an object property, a new keyframe is added for that property.")
        },
        {
            "layer-lower",
            i18n("Push the selection further back.")
        },
        {
            "layer-raise",
            i18n("Brings the selection further to the front.")
        },
    };

    QSize pix_size(128, 128);

    int row = 0;

    QLabel* logo = new QLabel(wid);
    logo->setPixmap(QIcon(gui::GlaxnimateApp::instance()->data_file("images/splash.svg")).pixmap(pix_size * 2));
    logo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    lay->addWidget(logo, row++, 0, 1, 2, Qt::AlignCenter);

    QLabel* global_desc = new QLabel(wid);
    global_desc->setText(tr(R"(
Glaxnimate is a vector animation program.
You can use it to create and modify Animated SVG, Telegram Animated Stickers, and Lottie animations.
Follows a guide of the main icons and what they do.
)"));
    global_desc->setWordWrap(true);
    lay->addWidget(global_desc, row++, 0, 1, 2);

    for ( const auto& p : buttons )
    {
        QFrame *line = new QFrame(wid);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        lay->addWidget(line, row++, 0, 1, 2);

        QLabel* preview = new QLabel(wid);
        preview->setPixmap(QIcon::fromTheme(p.first).pixmap(pix_size));
        preview->setMinimumSize(pix_size);
        preview->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        lay->addWidget(preview, row, 0, Qt::AlignTop|Qt::AlignHCenter);

        QLabel* desc = new QLabel(wid);
        desc->setText(p.second);
        desc->setMinimumSize(pix_size);
        desc->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        desc->setWordWrap(true);
        lay->addWidget(desc, row, 1);

        row++;
    }
}
