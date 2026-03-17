/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractSpinBox>
#include <QVector2D>

namespace glaxnimate::gui {


class SpinBox2D : public QAbstractSpinBox
{
    Q_OBJECT

    Q_PROPERTY(QPointF valuePoint READ valuePoint WRITE setValuePoint)
    Q_PROPERTY(QVector2D valueVector2D READ valueVector2D WRITE setValueVector2D)
    Q_PROPERTY(QSizeF valueSize READ valueSize WRITE setValueSize)

    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(int decimals READ decimals WRITE setDecimals)
    Q_PROPERTY(bool ratioLock READ ratioLock WRITE setRatioLock)
    Q_PROPERTY(double ratio READ ratio WRITE setRatio)
    Q_PROPERTY(QString suffix READ suffix WRITE setSuffix)

public:
    explicit SpinBox2D(QWidget *parent = nullptr);
    ~SpinBox2D();

    QPointF valuePoint() const;
    QVector2D valueVector2D() const;
    QSizeF valueSize() const;
    double valueX() const;
    double valueY() const;

    double minimum() const;
    double maximum() const;
    double singleStep() const;
    int decimals() const;
    QString separator() const;
    bool ratioLock() const;
    double ratio() const;
    QString suffix();

    void setMinimum(double min);
    void setMaximum(double max);
    void setSingleStep(double step);
    void setDecimals(int dec);
    void setSeparator(const QString &sep);
    void setRatioLock(bool lock, bool update_ratio=true);
    void setRatio(double ratio);
    void setRatio(double x, double y);
    void setSuffix(const QString &suffix);
    void setValuePoint(const QPointF &val);
    void setValueVector2D(const QVector2D &val);
    void setValueSize(const QSizeF &val);

    QValidator::State validate(QString &input, int &pos) const override;
    void fixup(QString &input) const override;
    void stepBy(int steps) override;

Q_SIGNALS:
    void valueChanged();

public Q_SLOTS:
    void setValue(double x, double y);

protected:
    StepEnabled stepEnabled() const override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject* source, QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    class Private;
    std::unique_ptr<Private> d;

};

} // namespace glaxnimate::gui
