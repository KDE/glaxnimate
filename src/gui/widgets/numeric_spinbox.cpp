/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "numeric_spinbox.hpp"

#include <cstring>

namespace {

class SimpleExprParser
{
public:
    SimpleExprParser(QLocale locale) :
        number_extra(locale.zeroDigit() + locale.groupSeparator() + locale.decimalPoint()),
        locale(std::move(locale))
    {}

    qreal parse(QStringView string)
    {
        text = string;
        lex();
        return parse_expr();
    }

    bool is_empty(QStringView string)
    {
        text = string;
        return lex() == Eof;
    }

private:
    QStringView text;
    int pos = 0;
    enum Token {
        Operator,
        Number,
        Eof
    } lookahead;
    qreal token_number = 0;
    char token_op = 0;
    static constexpr const char* single_ops = "+-*/()";
    QString number_extra;
    QLocale locale;

    QChar get_ch()
    {
        auto ch = text[pos];
        pos++;
        return ch;
    }

    bool at_end()
    {
        return pos >= text.length();
    }

    bool is_num(const QChar& ch)
    {
        return ch.isDigit() || number_extra.contains(ch);
    }

    Token lex()
    {
        while ( true )
        {
            QChar qch;
            while ( true )
            {
                if ( at_end() )
                    return lookahead = Eof;
                qch = get_ch();
                if ( !qch.isSpace() )
                    break;
            }

            char ch = qch.toLatin1();

            if ( strchr(single_ops, ch) )
            {
                token_op = ch;
                return lookahead = Operator;
            }

            if ( is_num(qch) )
            {
                token_number = lex_number();
                return lookahead = Number;
            }

            // Skip unknown characters
        }
    }

    qreal lex_number()
    {
        const QChar* start = text.constData() + pos - 1;
        while ( !at_end() )
        {
            if ( !is_num(get_ch()) )
            {
                pos--;
                break;
            }
        }
        const QChar* end = text.constData() + pos;

        return locale.toDouble(QStringView(start, end));
    }

    qreal parse_expr()
    {
        return parse_add();
    }

    qreal parse_add()
    {
        qreal val = parse_mult();
        while ( lookahead == Operator )
        {
            if ( token_op == '+' )
            {
                lex();
                val += parse_mult();
            }
            else if ( token_op == '-' )
            {
                lex();
                val -= parse_mult();
            }
            else
            {
                break;
            }
        }
        return val;
    }

    qreal parse_mult()
    {
        qreal val = parse_unary();
        while ( lookahead == Operator )
        {
            if ( token_op == '*' )
            {
                lex();
                val *= parse_unary();
            }
            else if ( token_op == '/' )
            {
                lex();
                qreal mult = parse_unary();
                if ( !qFuzzyIsNull(mult) )
                    val /= mult;
            }
            else
            {
                break;
            }
        }
        return val;
    }

    qreal parse_unary()
    {
        if ( lookahead == Number )
        {
            auto val = token_number;
            lex();
            return val;
        }
        else if ( lookahead == Operator )
        {
            auto op = token_op;
            lex();
            switch ( op )
            {
                case '+':
                    return parse_unary();
                case '-':
                    return -parse_unary();
                case '(':
                {
                    qreal val = parse_expr();
                    if ( lookahead == Operator && token_op == ')' )
                        lex();
                    return val;
                }
            }
            return 0;
        }

        lex();
        return 0;
    }
};

} // namespace

glaxnimate::gui::NumericSpinBox::NumericSpinBox(QWidget* parent) : QDoubleSpinBox(parent)
{
    setMinimum(-999'999.99); // '); lupdate is sometimes weird
    setMaximum(+999'999.99); // '); lupdate is sometimes weird
    setValue(0);
    setDecimals(2);
}



double glaxnimate::gui::NumericSpinBox::valueFromText(const QString& text) const
{
    return SimpleExprParser(locale()).parse(text);
}

QValidator::State glaxnimate::gui::NumericSpinBox::validate(QString& input, int&) const
{
    if ( SimpleExprParser(locale()).is_empty(input) )
        return QValidator::Intermediate;
    return QValidator::Acceptable;
}

double glaxnimate::gui::NumericSpinBox::parse(QLocale locale, QStringView text)
{
    return SimpleExprParser(locale).parse(text);
}
