/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QIODevice>

#include "ps_value.hpp"

namespace glaxnimate::ps {

using namespace Qt::StringLiterals;

struct Token
{
    enum Type
    {
        Eof,
        Literal,
        Comment,
        Operator,
        Unrecoverable
    };

    static Token make_operator(String val)
    {
        Token op{Operator, val};
        op.value.set_attribute(Value::Executable, true);
        return op;
    }

    Type type;
    Value value;
};

class Lexer
{
public:
    Lexer(QIODevice* device) : device(device) {}

    void set_device(QIODevice* device, bool reset_row=false)
    {
        this->device = device;
        if ( reset_row )
            file_row = 0;
        file_row++;
        file_column = 1;
    }

    Token next_token()
    {
        int ch;

        while ( true )
        {
            ch = get_char();
            if ( ch == -1 )
                return {Token::Eof, {}};
            else if ( !std::isspace(ch) )
                break;
        }

        if ( ch == '%' )
            return lex_comment();

        if ( ch == '+' || ch == '-' || std::isdigit(ch) )
            return lex_number(ch);

        if ( ch == '.' )
            return lex_float(".", true);

        if ( ch == '(' )
            return lex_string();

        if ( ch == '<' )
            return lex_lt();

        if ( ch == '/' )
            return lex_name();

        if ( ch == '[' )
            return Token::make_operator("[");

        return lex_operator(ch);
    }

    int row() const { return file_row; }
    int column() const { return file_column; }

private:
    Token lex_comment()
    {
        QByteArray comment;

        while ( true )
        {
            auto ch = get_char();
            if ( ch == -1 || ch == '\n' )
                break;
            comment += char(ch);
        }
        return {Token::Comment, comment};
    }

    Token lex_unknown(QByteArray head, char next)
    {
        head += next;
        while ( true )
        {
            int ch = get_char();
            if ( token_end(ch) )
                break;
            head += char(ch);
        }

        return {Token::Operator, head};
    }

    Token lex_operator(char next)
    {
        if ( next == '>' && peek() == next )
        {
            get_char();
            return Token::make_operator(">>");
        }

        QByteArray op(1, next);
        if ( !is_delimiter(next) )
        {
            while ( true )
            {
                int ch = get_char();
                if ( token_end(ch) )
                    break;
                op += char(ch);
            }
        }

        return Token::make_operator(op);
    }

    Token lex_number(char start)
    {
        QByteArray numstr;
        numstr += start;
        while ( true )
        {
            int ch = get_char();
            if ( token_end(ch) )
                break;

            if ( ch == '#' )
            {
                int base = numstr.toInt();
                if ( base < 2 )
                    return lex_unknown(numstr, ch);
                return lex_num_radix(numstr + char(ch), base);
            }

            if ( ch == '.' || std::tolower(ch) == 'e' )
                return lex_float(numstr + char(ch), ch == '.');

            if ( !std::isdigit(ch) )
                return lex_unknown(numstr, ch);

            numstr += char(ch);
        }

        return {Token::Literal, Value(numstr.toInt())};
    }

    Token lex_num_radix(const QByteArray& prefix, int base)
    {
        QByteArray numstr;
        while ( true )
        {
            int ch = get_char();
            if ( token_end(ch) )
                break;

            int digit = -1;
            if ( std::isdigit(ch) )
                digit = ch - '0';
            else if ( std::isalpha(ch) )
                digit = std::tolower(ch) - 'a';
            else
                return lex_unknown(prefix + numstr, ch);

            if ( digit >= base )
                return lex_unknown(prefix + numstr, ch);

            numstr += char(ch);
        }

        return {Token::Literal, numstr.toInt(nullptr, base)};
    }

    Token lex_float(QByteArray numstr, bool before_exp)
    {
        bool has_exp = true;
        if ( before_exp )
        {
            has_exp = false;
            while ( true )
            {
                int ch = get_char();
                if ( token_end(ch) )
                    break;
                if ( std::isdigit(ch) )
                {
                    numstr += char(ch);
                }
                else if ( std::tolower(ch) == 'e' )
                {
                    has_exp = true;
                    numstr += 'e';
                    break;
                }
                else
                {
                    return lex_unknown(numstr, ch);
                }
            }
        }

        if ( has_exp )
        {
            int ch = peek();
            if ( ch == '+' || ch == '-' )
            {
                numstr += char(ch);
                get_char();
            }

            while ( true )
            {
                int ch = get_char();
                if ( token_end(ch) )
                    break;

                if ( std::isdigit(ch) )
                    numstr += char(ch);
                else
                    return lex_unknown(numstr, ch);

            }
        }

        return {Token::Literal, numstr.toFloat()};
    }

    Token lex_string()
    {
        QByteArray str;
        int paren_count = 0;

        while ( true )
        {
            int ch = get_char();
            if ( ch == -1 )
                break;

            if ( ch == ')' )
            {
                paren_count--;
                if ( paren_count < 0 )
                    break;
            }

            if ( ch == '\\' )
            {
                str += lex_escape();
                continue;
            }

            str += char(ch);

            if ( ch == '(' )
            {
                paren_count++;
            }
        }

        return {Token::Literal, str};
    }

    QByteArray lex_escape()
    {
        int ch = get_char();
        switch ( ch )
        {
            case -1: return {};
            case 'n': return "\n";
            case 'r': return "\r";
            case 'b': return "\b";
            case 't': return "\t";
            case 'f': return "\f";
            case '\\': return "\\";
            case '(': return "(";
            case ')': return ")";
            case '\n': return {};
        }

        // \ddd
        if ( std::isdigit(ch) && ch < '8' )
        {
            QByteArray num(1, ch);
            for ( int i = 1; i < 3; i++ )
            {
                ch = get_char();
                if ( ch == -1 )
                    break;
                if ( !std::isdigit(ch) || ch >= '8' )
                {
                    unget(ch);
                    break;
                }
                num += char(ch);
            }
            return QByteArray(1, num.toInt(nullptr, 8));
        }

        // ignore \r and \r\n
        if ( ch == '\r' )
        {
            ch = get_char();
            if ( ch != -1 && ch != '\n' )
                unget(ch);
            return {};
        }

        return QByteArray(1, ch);
    }

    Token lex_lt()
    {
        char next = peek();
        if ( next == '~' )
        {
            get_char();
            return lex_base85();
        }

        if ( next == '<' )
        {
            get_char();
            return Token::make_operator("<<");
        }

        return lex_hex_string();
    }

    Token lex_hex_string()
    {
        QByteArray data;
        while ( true )
        {
            int ch = get_char();

            if ( ch == '>' )
                break;

            if ( token_end(ch) )
                break;

            if ( !std::isxdigit(ch) )
            {
                return {Token::Unrecoverable, {}};
            }

            data += ch;
        }

        if ( data.size() % 2 )
            data += '0';

        return {Token::Literal, QByteArray::fromHex(data)};
    }

    Token lex_name()
    {
        QByteArray name;
        while ( true )
        {
            int ch = get_char();
            if ( token_end(ch) )
                break;
            name += char(ch);
        }

        return {Token::Literal, name};
    }

    Token lex_base85()
    {
        QByteArray data;
        quint32 accum = 0;
        int count = 0;

        while ( true )
        {
            int ch = get_char();
            if ( ch == -1 || ch == '~' )
                break;
            if ( std::isspace(ch) )
                continue;

            if ( ch == 'z' )
            {
                data.append(4, 0);
                continue;
            }

            if ( ch < '!' || ch > 'u' )
                return {Token::Unrecoverable, {}};

            accum *= 85;
            accum += ch - '!';
            count++;
            if ( count == 5 )
            {
                data.push_back((accum >> 24) & 0xff);
                data.push_back((accum >> 16) & 0xff);
                data.push_back((accum >> 8) & 0xff);
                data.push_back(accum & 0xff);
                count = 0;
            }
        }

        if ( count )
        {
            for( ; count % 5; count++ )
            {
                accum *= 85;
                accum += 84;
            }
            data.push_back((accum >> 24) & 0xff);
            data.push_back((accum >> 16) & 0xff);
            data.push_back((accum >> 8) & 0xff);
            data.push_back(accum & 0xff);
        }

        if ( get_char() != '>' )
            return {Token::Unrecoverable, {}};

        return {Token::Literal, data};
    }

    int get_char()
    {
        if ( device->atEnd() )
            return -1;

        char ch;
        if ( !device->getChar(&ch) )
            return -1;

        if ( ch == '\n' )
        {
            file_row++;
            file_column = 0;
        }
        else
        {
            file_column++;
        }

        return ch;
    }

    void unget(char ch)
    {
        device->ungetChar(ch);
        file_column--;
    }

    int peek()
    {
        char ch = 0;
        if ( !device->peek(&ch, 1) )
            return -1;
        return ch;
    }

    bool is_delimiter(char ch)
    {
        return
            ch == '(' || ch == ')' ||
            ch == '<' || ch == '>' ||
            ch == '[' || ch == ']' ||
            ch == '{' || ch == '}' ||
            ch == '/' || ch == '%'
        ;
    }

    bool token_end(int ch)
    {
        if ( ch == -1 || std::isspace(ch) )
            return true;

        if ( !is_delimiter(ch) )
            return false;

        unget(ch);
        return true;
    }

    QIODevice* device;
    int file_row = 1;
    int file_column = 0;
};

} // namespace glaxnimate::ps
