/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QTest>
#include <QBuffer>

#include "glaxnimate/module/postscript/ps_lexer.hpp"

using namespace glaxnimate::ps;


class StringLexer : public Lexer
{
public:
    StringLexer(const char* text)
        : Lexer(nullptr),
          arr(text),
          buf(&arr)
    {
        buf.open(QIODeviceBase::ReadOnly);
        set_device(&buf);
    }

    QByteArray arr;
    QBuffer buf;
};

#define COMPARE_VALUE(val, typenum, expect) \
    QCOMPARE(val.type(), typenum); \
    QCOMPARE(val.value(), QVariant::fromValue(expect))

#define COMPARE_TOKEN(val, toktype, typenum, expect) \
    do { auto token_ = val; \
    QCOMPARE(token_.type, toktype); \
    COMPARE_VALUE(token_.value, typenum, expect); \
    } while(false)


class TestCase: public QObject
{
    Q_OBJECT

    Token lex(const char* text)
    {
        return StringLexer(text).next_token();
    }

private Q_SLOTS:
    void test_lex_comment()
    {
        auto token = lex("% foo bar\nbaz");
        COMPARE_TOKEN(token, Token::Comment, Value::String, QVariant(u" foo bar"_s));
    }

    void test_lex_int()
    {
        StringLexer lexer("123 -98 43445 0 +17");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 123);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, -98);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 43445);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 0);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 17);
        COMPARE_TOKEN(lex("10z"), Token::Operator, Value::String, u"10z"_s);
        COMPARE_TOKEN(lex("++10"), Token::Operator, Value::String, u"++10"_s);
    }

    void test_lex_radix()
    {
        StringLexer lexer("8#1777 16#fffe 2#1000 36#Glax");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 01777);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 0xfffe);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 0b1000);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 774105);

        COMPARE_TOKEN(lex("2#102"), Token::Operator, Value::String, u"2#102"_s);
        COMPARE_TOKEN(lex("2#10~"), Token::Operator, Value::String, u"2#10~"_s);
    }

    void test_float()
    {
        StringLexer lexer("-0.25 34.5 -23.6e4 1.0E-5 1E6 -1. .5");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Real, -0.25);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Real, 34.5);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Real, -23.6e4);

        COMPARE_TOKEN(lex("0.2.3"), Token::Operator, Value::String, u"0.2.3"_s);
        COMPARE_TOKEN(lex("0.45f"), Token::Operator, Value::String, u"0.45f"_s);
        COMPARE_TOKEN(lex("e6"), Token::Operator, Value::String, u"e6"_s);
    }

    void test_string()
    {
        StringLexer lexer("(Hello World) (Hello (World)) (Hello\nWorld) (Hello\\nWorld) (Hello\\\nWorld)");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, u"Hello World"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, u"Hello (World)"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, u"Hello\nWorld"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, u"Hello\nWorld"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, u"HelloWorld"_s);
    }

    void test_hex_string()
    {
        StringLexer lexer("<48656c6c6f> <48656c7>");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Hello"_ba);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Help"_ba);
        QCOMPARE(lex("<48656z7>").type, Token::Unrecoverable);
    }

    void test_double_angle()
    {
        StringLexer lexer("<< >>");
        QCOMPARE(lexer.next_token().value.type(), Value::Mark);
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, u">>"_s);
    }

    void test_base85()
    {
        StringLexer lexer("<~9jqo^~> <~9j qo^~> <~zz~> <~/c~>");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Man "_ba);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Man "_ba);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "\0\0\0\0\0\0\0\0"_ba);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "\56\3\31\264"_ba);
    }

    void test_operators()
    {
        StringLexer lexer("abc Offset $$ 23A 13-456 a.b $MyDict @pattern");
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, u"abc"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, u"Offset"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, u"$$"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, u"23A"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, u"13-456"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, u"a.b"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, u"$MyDict"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, u"@pattern"_s);
    }

    void test_name()
    {
        StringLexer lexer("/foo /bar/baz");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, u"foo"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, u"bar"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, u"baz"_s);
    }

    void test_mark()
    {
        StringLexer lexer("[123 /abc]<</1 2>>");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Mark, QVariant());
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, u"123"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, u"abc"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, u"]"_s);

        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Mark, QVariant());
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, u"1"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, u"2"_s);
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, u">>"_s);
    }

};

QTEST_GUILESS_MAIN(TestCase)
#include "test_ps_parser.moc"

