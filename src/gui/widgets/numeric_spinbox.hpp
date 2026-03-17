/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once


#include <QSpinBox>

namespace glaxnimate::gui {

class NumericSpinBox : public QDoubleSpinBox
{
    Q_OBJECT

public:
    NumericSpinBox(QWidget* parent = nullptr);

    double valueFromText(const QString &text) const override;

    QValidator::State validate(QString & input, int & pos) const override;

    static double parse(QLocale locale, QStringView text);
};

} // namespace glaxnimate::gui
