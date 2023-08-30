/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QPixmap>
#include <QImage>
#include <QFileInfo>
#include <QUrl>

#include "model/assets/asset.hpp"

namespace glaxnimate::model {

class Bitmap : public Asset
{
    GLAXNIMATE_OBJECT(Bitmap)
    GLAXNIMATE_PROPERTY(QByteArray, data, {}, &Bitmap::on_refresh)
    GLAXNIMATE_PROPERTY(QString, filename, {}, &Bitmap::on_refresh)
    GLAXNIMATE_PROPERTY(QString, url, {}, &Bitmap::on_refresh)
    GLAXNIMATE_PROPERTY_RO(QString, format, {})
    GLAXNIMATE_PROPERTY_RO(int, width, -1)
    GLAXNIMATE_PROPERTY_RO(int, height, -1)
    Q_PROPERTY(bool embedded READ embedded WRITE embed)
    Q_PROPERTY(QImage image READ get_image)

public:
    using Asset::Asset;

    void paint(QPainter* painter) const;

    bool embedded() const;

    QIcon instance_icon() const override;

    QString type_name_human() const override
    {
        return tr("Bitmap");
    }

    bool from_url(const QUrl& url);
    bool from_file(const QString& file);
    bool from_base64(const QString& data);
    bool from_raw_data(const QByteArray& data);

    QUrl to_url() const;

    QString object_name() const override;

    QFileInfo file_info() const;

    const QPixmap& pixmap() const { return image; }
    void set_pixmap(const QImage& qimage, const QString& format);

    bool remove_if_unused(bool clean_lists) override;

    QImage get_image() const
    {
        return image.toImage();
    }

    /**
     * \brief If `embedded()` returns `data`, otherwise tries to load the data based on filename
     */
    QByteArray image_data() const;

    QSize size() const;

public slots:
    void refresh(bool rebuild_embedded);

    void embed(bool embedded);

private:
    QByteArray build_embedded(const QImage& img) const;

private slots:
    void on_refresh();

signals:
    void loaded();

private:
    QPixmap image;

};

} // namespace glaxnimate::model
