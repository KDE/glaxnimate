/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "spin2d.hpp"

#include <QLineEdit>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QFocusEvent>
#include <QtMath>

#include "glaxnimate/utils/i18n.hpp"
#include "numeric_spinbox.hpp"

using namespace glaxnimate::gui;


class SpinBox2D::Private
{
public:
    enum Section { SectionNone = 0, SectionX = 1, SectionY = 2 };

    SpinBox2D* parent;
    double x = 0;
    double y = 0;
    double min = -999'999.99;
    double max = 999'999.99;
    double step = 1;
    int decimals = 2;
    QString separator = QStringLiteral(" × ");
    double ratio = 1;
    bool ratio_lock = false;
    QString suffix;

    bool strings_dirty = false;
    QString x_str = "0";
    QString y_str = "0";
    QAction* toggle_lock;
    bool moused = false;

    void update_lineedit()
    {
        update_strings();
        parent->lineEdit()->setText(x_str + separator + y_str);
    }

    void update_lineedit_section(Section section)
    {
        QString text = parent->lineEdit()->text();
        int pos = parent->lineEdit()->cursorPosition();
        const int sep_start = text.indexOf(separator);
        if ( sep_start < 0 )
            return;

        update_strings();


        if ( section == SectionY )
        {
            text = x_str + text.mid(sep_start);
            pos = pos - sep_start + x_str.size();
        }
        else
        {
            text = text.left(sep_start + separator.size()) + y_str;
        }

        parent->lineEdit()->setText(text);
        parent->lineEdit()->setCursorPosition(pos);;
    }


    void update_strings()
    {
        if ( !strings_dirty )
            return;
        strings_dirty = false;
        x_str = value_string(x);
        y_str = value_string(y);
    }

    QString value_string(double v)
    {
        return parent->locale().toString(v, 'f', decimals) + suffix;
    }

    int y_start()
    {
        return x_str.size() + separator.size();
    }

    void select_section(Section sec)
    {
        update_strings();
        if ( sec == SectionX )
            parent->lineEdit()->setSelection(0, x_str.size());
        else
            parent->lineEdit()->setSelection(y_start(), y_str.size());
    }

    bool section_is_selected(Section sec)
    {
        update_strings();
        int start, size;
        if ( sec == SectionX )
        {
            start = 0;
            size = x_str.size();
        }
        else
        {
            start = y_start();
            size = y_str.size();
        }

        return parent->lineEdit()->selectionStart() == start && parent->lineEdit()->selectionLength() == size;
    }

    Section section_at(int cursor_pos)
    {
        update_strings();
        return (cursor_pos <= x_str.size()) ? SectionX : SectionY;
    }

    bool value_from_text(const QString &text, double &x, double &y, Section sec)
    {
        const int idx = text.indexOf(separator);
        if ( idx < 0 )
            return false;

        QStringView view(text);
        bool ok = false;

        x = to_double(view.left(idx), ok);
        if ( !ok )
            return false;

        y = to_double(view.mid(idx + separator.length()), ok);
        if ( !ok )
            return false;

        if ( ratio_lock )
        {
            if ( sec == SectionX )
                y = x / ratio;
            else if ( sec == SectionY )
                x =  y * ratio;
        }

        return true;
    }

    double to_double(const QStringView& s, bool& ok)
    {
        ok = true;
        return bounded(
            NumericSpinBox::parse(parent->locale(), s)
            // parent->locale().toDouble(s.trimmed(), &ok)
        );
    }

    double bounded(double v)
    {
        return qBound(min, v, max);
    }

    void set_y(double y)
    {
        y = bounded(y);
        double x = ratio_lock ? y * ratio : this->x;
        parent->setValue(x, y);
    }

    void set_x(double x)
    {
        x = bounded(x);
        double y = ratio_lock ? x / ratio : this->y;
        parent->setValue(x, y);
    }

    void set_ratio(double x, double y)
    {
        if ( qFuzzyIsNull(x) || qFuzzyIsNull(y) )
            return;
        ratio = x / y;
    }

    Section current_section()
    {
        return section_at(parent->lineEdit()->cursorPosition());
    }
};

glaxnimate::gui::SpinBox2D::~SpinBox2D() = default;

glaxnimate::gui::SpinBox2D::SpinBox2D(QWidget* parent)
    : QAbstractSpinBox(parent),
    d(std::make_unique<Private>())
{
    d->parent = this;

    d->update_lineedit();

    lineEdit()->installEventFilter(this);


    connect(lineEdit(), &QLineEdit::cursorPositionChanged, this, [this](int oldp, int newp) {
        int sep_start = lineEdit()->text().indexOf(d->separator);
        if ( sep_start < 0 )
            return;

        int sep_end = sep_start + d->separator.size();

        if ( newp > sep_start && newp < sep_end )
        {
            if ( oldp < sep_end )
                newp = sep_end;
            else
                newp = sep_start;
            lineEdit()->setCursorPosition(newp);
        }
    });

    connect(lineEdit(), &QLineEdit::textEdited, this, [this](const QString &text)
    {
        double x, y;
        auto section = d->current_section();
        if ( d->value_from_text(text, x, y, section) )
        {
            d->x = x;
            d->y = y;
            d->strings_dirty = true;
            d->update_lineedit_section(section);
            Q_EMIT valueChanged();
        }
    });

    d->toggle_lock = lineEdit()->addAction(QIcon::fromTheme("object-unlocked"), QLineEdit::TrailingPosition);
    d->toggle_lock->setToolTip(i18n("Lock aspect ratio"));

    connect(d->toggle_lock, &QAction::triggered, this, [this]() {
        setRatioLock(!d->ratio_lock, true);
    });
}

void glaxnimate::gui::SpinBox2D::setValue(double x, double y)
{
    x = d->bounded(x);
    y = d->bounded(y);
    // same value
    if ( qFuzzyCompare(x, d->x) && qFuzzyCompare(y, d->y) )
        return;
    d->x = x;
    d->y = y;
    d->strings_dirty = true;
    d->update_lineedit();
    Q_EMIT valueChanged();
}

int glaxnimate::gui::SpinBox2D::decimals() const
{
    return d->decimals;
}

double glaxnimate::gui::SpinBox2D::maximum() const
{
    return d->max;
}

double glaxnimate::gui::SpinBox2D::minimum() const
{
    return d->min;
}

bool glaxnimate::gui::SpinBox2D::ratioLock() const
{
    return d->ratio_lock;
}

QString glaxnimate::gui::SpinBox2D::separator() const
{
    return d->separator;
}

void glaxnimate::gui::SpinBox2D::setDecimals(int dec)
{
    d->decimals = dec;
    d->strings_dirty = true;
    d->update_lineedit();
}

void glaxnimate::gui::SpinBox2D::setMaximum(double max)
{
    if ( max < d->max )
    {
        d->max = max;
        setValue(d->x, d->y);
    }
    else
    {
        d->max = max;
    }
}

void glaxnimate::gui::SpinBox2D::setMinimum(double min)
{
    if ( min > d->min )
    {
        d->min = min;
        setValue(d->x, d->y);
    }
    else
    {
        d->min = min;
    }
}

QPointF glaxnimate::gui::SpinBox2D::valuePoint() const
{
    return {d->x, d->y};
}

QSizeF glaxnimate::gui::SpinBox2D::valueSize() const
{
    return {d->x, d->y};
}

QVector2D glaxnimate::gui::SpinBox2D::valueVector2D() const
{
    return QVector2D(d->x, d->y);
}

double glaxnimate::gui::SpinBox2D::valueX() const
{
    return d->x;
}


double glaxnimate::gui::SpinBox2D::valueY() const
{
    return d->y;
}


void glaxnimate::gui::SpinBox2D::setValuePoint(const QPointF& val)
{
    setValue(val.x(), val.y());
    if ( d->ratio_lock )
        d->set_ratio(d->x, d->y);
}

void glaxnimate::gui::SpinBox2D::setValueVector2D(const QVector2D& val)
{
    setValue(val.x(), val.y());
    if ( d->ratio_lock )
        d->set_ratio(d->x, d->y);
}

void glaxnimate::gui::SpinBox2D::setValueSize(const QSizeF& val)
{
    setValue(val.width(), val.height());
    if ( d->ratio_lock )
        d->set_ratio(d->x, d->y);
}

void glaxnimate::gui::SpinBox2D::setRatioLock(bool lock, bool update_ratio)
{
    if ( lock && update_ratio )
        d->set_ratio(d->x , d->y);

    d->ratio_lock = lock;
    d->toggle_lock->setIcon(lock ? QIcon::fromTheme("object-locked") : QIcon::fromTheme("object-unlocked"));
    update();
}

double glaxnimate::gui::SpinBox2D::ratio() const
{
    return d->ratio;
}

void glaxnimate::gui::SpinBox2D::setRatio(double ratio)
{
    d->ratio = ratio;
}

void glaxnimate::gui::SpinBox2D::setRatio(double x, double y)
{
    d->set_ratio(x, y);
}

void glaxnimate::gui::SpinBox2D::setSeparator(const QString& sep)
{
    d->separator = sep;
    d->update_lineedit();
}

void glaxnimate::gui::SpinBox2D::fixup(QString& input) const
{
    double x, y;

    if ( !d->value_from_text(input, x, y, Private::SectionNone))
    {
        x = d->x;
        y = d->y;
    }

    input = d->value_string(x) + d->separator + d->value_string(y);
}

QString glaxnimate::gui::SpinBox2D::suffix()
{
    return d->suffix;
}

void glaxnimate::gui::SpinBox2D::setSuffix(const QString& suffix)
{
    d->suffix = suffix;
    d->strings_dirty = true;
    d->update_lineedit();
}

void glaxnimate::gui::SpinBox2D::setSingleStep(double step)
{
    d->step = step;
}

double glaxnimate::gui::SpinBox2D::singleStep() const
{
    return d->step;
}

void glaxnimate::gui::SpinBox2D::stepBy(int steps)
{
    auto section = d->current_section();
    if ( section == Private::SectionX )
        d->set_x(d->x + steps * d->step);
    else
        d->set_y(d->y + steps * d->step);

    d->select_section(section);
}

SpinBox2D::StepEnabled glaxnimate::gui::SpinBox2D::stepEnabled() const
{
    double cur = d->current_section() == Private::SectionX ? d->x : d->y;
    StepEnabled result;
    if (cur > d->min) result |= StepDownEnabled;
    if (cur < d->max) result |= StepUpEnabled;
    return result;
}

QValidator::State glaxnimate::gui::SpinBox2D::validate(QString& input, int&) const
{
    // TODO add alternative separators like x ; : -
    const int idx = input.indexOf(d->separator);

    if ( idx < 0 )
        return QValidator::Invalid;

    return QValidator::Acceptable;
}

void SpinBox2D::focusInEvent(QFocusEvent *event)
{
    QAbstractSpinBox::focusInEvent(event);

    if (event->reason() == Qt::BacktabFocusReason)
        d->select_section(Private::SectionY);
    else //if (event->reason() == Qt::TabFocusReason)
        d->select_section(Private::SectionX);
}

void SpinBox2D::focusOutEvent(QFocusEvent *event)
{
    double x, y;
    if ( d->value_from_text(lineEdit()->text(), x, y, Private::SectionNone))
        setValue(x, y);
    else
        d->update_lineedit(); // revert to last valid value

    QAbstractSpinBox::focusOutEvent(event);
}


void SpinBox2D::keyPressEvent(QKeyEvent *event)
{
    // TODO figure out why it doesn't work
    /*
    switch (event->key())
    {
        case Qt::Key_Tab:
            if ( d->section == Private::SectionX )
            {
                d->select_section(Private::SectionY);
                event->accept();
                return;
            }
            break;
        case Qt::Key_Backtab:
            if ( d->section == Private::SectionY )
            {
                d->select_section(Private::SectionX);
                event->accept();
                return;
            }
            break;
        default:
            break;
    }

    */
    QAbstractSpinBox::keyPressEvent(event);
}

bool glaxnimate::gui::SpinBox2D::eventFilter(QObject* source, QEvent *event)
{
    if ( source == lineEdit() )
    {
        if ( event->type() == QEvent::MouseButtonPress )
        {
            auto btn_event = static_cast<QMouseEvent*>(event);
            if ( btn_event->button() == Qt::LeftButton )
            {
                const int pos = lineEdit()->cursorPositionAt(btn_event->position().toPoint());
                auto section = d->section_at(pos);
                if ( !d->section_is_selected(section) )
                    d->moused = true;
            }
        }
        else if ( event->type() == QEvent::MouseMove )
        {
            d->moused = false;
        }
        else if ( event->type() == QEvent::MouseButtonRelease )
        {
            auto btn_event = static_cast<QMouseEvent*>(event);
            if ( btn_event->button() == Qt::LeftButton && d->moused )
            {
                d->moused = false;
                const int pos = lineEdit()->cursorPosition();
                auto section = d->section_at(pos);
                d->select_section(section);
            }
        }
    }

    return QAbstractSpinBox::eventFilter(source, event);
}
