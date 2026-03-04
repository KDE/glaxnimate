/*
 * SPDX-FileCopyrightText: 2019-2025 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "emoji_set_dialog.hpp"
#include "ui_emoji_set_dialog.h"

#include <array>

#include <QEvent>
#include <QUrl>
#include <QJsonDocument>
#include <QPainter>
#include <QSvgRenderer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QBuffer>

#include <KLocalizedContext>
#include <KTar>
#include <KCompressionDevice>

#include "emoji/emoji_set.hpp"
#include "glaxnimate_app.hpp"
#include "emoji_dialog.hpp"
#include "glaxnimate/utils/data_paths.hpp"

class glaxnimate::emoji::EmojiSetDialog::Private
{
public:
    Ui::EmojiSetDialog ui;
    std::vector<EmojiSet> sets;
    QNetworkAccessManager downloader;
    QString selected;

    static const int preview_count = 6;
    static const int icon_size = 72;

    enum Columns
    {
        Name,
        License,
        Preview0,
        Downloaded = Preview0 + preview_count,
    };

    void set_download_status(int row, const char* theme_icon, const QString& msg)
    {
        auto item_loaded = new QTableWidgetItem();
        item_loaded->setIcon(QIcon::fromTheme(theme_icon));
        item_loaded->setToolTip(msg);
        ui.emoji_set_view->setItem(row, Private::Downloaded, item_loaded);
    }

    void show_preview(QIODevice* file, int row, int i)
    {
        QPixmap pix(icon_size, icon_size);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        QSvgRenderer renderer(file->readAll());
        renderer.render(&painter);
        if ( !pix.isNull() )
        {
            auto item = new QTableWidgetItem;
            item->setData(Qt::DecorationRole, pix);
            item->setSizeHint(pix.size() + QSize(5, 5));
            ui.emoji_set_view->setItem(row, Private::Preview0 + i, item);
        }
    }
};

glaxnimate::emoji::EmojiSetDialog::EmojiSetDialog(QWidget* parent)
    : QDialog(parent), d(std::make_unique<Private>())
{
    d->ui.setupUi(this);

    d->ui.emoji_set_view->verticalHeader()->setVisible(false);
    d->ui.emoji_set_view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    d->ui.emoji_set_view->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    d->ui.emoji_set_view->verticalHeader()->setMaximumSectionSize(Private::icon_size);
    d->ui.emoji_set_view->verticalHeader()->setMinimumSectionSize(Private::icon_size);
    d->ui.emoji_set_view->verticalHeader()->setDefaultSectionSize(Private::icon_size);
    d->ui.emoji_set_view->setIconSize(QSize(Private::icon_size, Private::icon_size));


    d->ui.progress_bar->setVisible(false);

    reload_sets();
}

glaxnimate::emoji::EmojiSetDialog::~EmojiSetDialog() = default;

void glaxnimate::emoji::EmojiSetDialog::reload_sets()
{
    d->ui.emoji_set_view->setRowCount(0);

    auto sets_path = gui::GlaxnimateApp::instance()->data_file("emoji/sets.json");
    if ( sets_path.isEmpty() )
        return;

    QFile sets_file(sets_path);
    if ( !sets_file.open(QFile::ReadOnly) )
        return;

    QJsonDocument doc = QJsonDocument::fromJson(sets_file.readAll());
    sets_file.close();

    d->sets.clear();

    for ( const auto& val : doc.array() )
        d->sets.push_back(EmojiSet::load(val.toObject()));


    static const std::array<QString, Private::preview_count> preview_emoji = {
        "1f61c",        // ;P
        "1f336",        // hot pepper
        "1f432",        // dragon face
        "1f1ea-1f1fa",  // EU flag
        "1faa2",        // knot
        "2764",         // red heart
    };

    int row = 0;
    for ( auto& p : d->sets )
    {
        p.path = QDir(utils::writable_data_path("emoji/" + p.name));
        d->ui.emoji_set_view->setRowCount(row+1);
        d->ui.emoji_set_view->setItem(row, Private::Name, new QTableWidgetItem(p.name));
        d->ui.emoji_set_view->setItem(row, Private::License, new QTableWidgetItem(p.license));

        for ( int i = 0; i < Private::preview_count; i++ )
        {
            QString path = p.image_path(EmojiSetDirectory::Scalable, preview_emoji[i]);
            if ( p.path.exists(path) )
            {
                QFile file(path);
                file.open(QIODevice::ReadOnly);
                d->show_preview(&file, row, i);
            }
            else
            {
                auto reply = d->downloader.get(QNetworkRequest(p.preview_url(preview_emoji[i])));
                connect(reply, &QNetworkReply::finished, this, [this, row, i, reply]{
                    if ( reply->error() )
                        return;
                    d->show_preview(reply, row, i);
                });
            }
        }

        if ( p.path.exists() )
            d->set_download_status(row, "package-installed-updated", i18nc("Package install status", "Installed"));
        else
            d->set_download_status(row, "package-available", i18nc("Package install status", "Not Installed"));

        row++;
    }
}

static const KArchiveDirectory* enter_tar_dir(const KArchiveDirectory* dir, const QString& path)
{
    auto entry = dir->entry(path);
    if ( entry && entry->isDirectory() )
        return static_cast<const KArchiveDirectory*>(entry);
    return nullptr;
}

static bool extract_all(const KArchiveDirectory* dir, QDir output, const QString& prefix)
{
    for ( const auto& chunk : prefix.split("/") )
    {
        if ( chunk.length() )
        {
            dir = enter_tar_dir(dir, chunk);
            if ( !dir )
                return false;
        }
    }

    if ( !output.mkpath(prefix) )
        return false;

    if ( !output.cd(prefix) )
        return false;

    bool ok = true;
    for ( const auto& path : dir->entries() )
    {
        if ( auto entry = dir->file(path) )
        {
            // entry->copyTo doesn't work
            QFile file(output.absoluteFilePath(path));
            if ( file.open(QFile::WriteOnly) )
                file.write(entry->data());
            else
                ok = false;
        }
    }
    return ok;

}

void glaxnimate::emoji::EmojiSetDialog::download_selected()
{
    int row = d->ui.emoji_set_view->currentRow();
    if ( row < 0 || row >= int(d->sets.size()) )
        return;

    QNetworkRequest request(d->sets[row].download.url);

    request.setMaximumRedirectsAllowed(3);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto reply = d->downloader.get(request);
    d->ui.progress_bar->setVisible(true);
    d->ui.progress_bar->setValue(0);
    d->ui.progress_bar->setMaximum(100);
    d->set_download_status(row, "package-installed-outdated", i18nc("Package install status", "Downloading"));
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total){
        d->ui.progress_bar->setValue(received);
        d->ui.progress_bar->setMaximum(total);
    });
    connect(reply, &QNetworkReply::finished, this, [this, row, reply]{

        if ( reply->error() || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200 )
        {
            d->ui.progress_bar->setVisible(false);
            auto reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
            d->set_download_status(row, "package-broken", i18nc("Package install status, %1 reason", "Could not download: %1", reason));
            return;
        }

        QString prefix_scalable = d->sets[row].download.paths[EmojiSetDirectory::Scalable].path;
        QString prefix_raster = d->sets[row].download.paths[72].path;

        QByteArray data = reply->readAll();
        reply->close();

        // Without this extra step reading everything into a buffer, KTar hangs
        {
            QBuffer buf(&data);
            buf.open(QIODevice::ReadOnly);

            KCompressionDevice compressed(&buf, false, KCompressionDevice::GZip);
            compressed.open(QIODevice::ReadOnly);

            data = compressed.readAll();
        }

        QBuffer buf1(&data);
        // buf1.open(QIODevice::ReadOnly);

        QDir output = d->sets[row].path;
        KTar tar(&buf1);
        bool has_error = !tar.open(QIODevice::ReadOnly);
        bool extract_failed = false;
        auto dir = tar.directory();
        if ( !dir )
        {
            has_error = true;
        }
        else
        {
            extract_failed = !extract_all(dir, output, prefix_scalable);
            extract_failed = !extract_all(dir, output, prefix_raster) || extract_failed;
        }
        if ( has_error )
            d->set_download_status(row, "package-broken", tar.errorString());
        else if ( !output.exists() )
            d->set_download_status(row, "package-broken", i18nc("Package install status", "Didn't download any files"));
        else if ( extract_failed )
            d->set_download_status(row, "package-broken", i18nc("Package install status", "Some files failed to extract"));
        else
            d->set_download_status(row, "package-installed-updated", i18nc("Package install status", "Installed"));

        d->ui.button_add_emoji->setEnabled(d->sets[row].path.exists());
        d->ui.progress_bar->setVisible(false);
    });
}


void glaxnimate::emoji::EmojiSetDialog::set_selected(int row)
{
    bool enabled = row >= 0 && row < int(d->sets.size());
    d->ui.button_download->setEnabled(enabled);
    d->ui.button_view_website->setEnabled(enabled);
    d->ui.button_add_emoji->setEnabled(enabled && d->sets[row].path.exists());
}

void glaxnimate::emoji::EmojiSetDialog::view_website()
{
    int row = d->ui.emoji_set_view->currentRow();
    if ( row < 0 || row >= int(d->sets.size()) )
        return;
    QDesktopServices::openUrl(d->sets[row].url);
}


void glaxnimate::emoji::EmojiSetDialog::add_emoji()
{
    int row = d->ui.emoji_set_view->currentRow();
    if ( row < 0 || row >= int(d->sets.size()) )
        return;

    d->selected.clear();
    const auto& set = d->sets[row];
    EmojiDialog dialog;
    int size = 72;
    if ( !set.download.paths.count(72) )
        size = set.download.paths.rbegin()->first;
    dialog.from_emoji_set(set, size);
    dialog.load_emoji(EmojiDialog::Image);
    if ( dialog.exec() )
    {
       d->selected = set.image_path(EmojiSetDirectory::Scalable, dialog.current_slug());
       accept();
    }
}

const QString & glaxnimate::emoji::EmojiSetDialog::selected_svg() const
{
    return d->selected;
}
