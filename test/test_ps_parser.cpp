/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QTest>
#include <QBuffer>

#include "bezier_test.hpp"
#include "glaxnimate/module/postscript/ps_lexer.hpp"
#include "glaxnimate/module/postscript/ps_interpreter.hpp"

using namespace glaxnimate::ps;
using namespace glaxnimate;


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


class TestInterpreter : public Interpreter
{
public:

    auto exec_string(const char* text)
    {
        QByteArray arr(text);
        QBuffer buf(&arr);
        buf.open(QIODeviceBase::ReadOnly);
        execute(&buf);
        return stack_values();
    }

    QString consume_output()
    {
        QString out = output;
        output = {};
        return out;
    }

    QString output;
    QString last_error;
    QByteArray last_comment;
    GraphicsState last_fill;
    GraphicsState last_stroke;

    std::vector<Value> stack_values()
    {
        std::vector<Value> vals;
        vals.reserve(memory().operand_stack.size());
        for ( const auto& v : memory().operand_stack )
            vals.push_back(v);
        std::reverse(vals.begin(), vals.end());
        return vals;
    }

protected:
    void on_print(const QString &text) override
    {
        output += text;
    }

    void on_error(const QString &text) override
    {
        last_error = text;
    }

    void on_warning(const QString &text) override
    {
        last_error = text;
    }

    void on_comment(const QByteArray &text) override
    {
        last_comment = text;
    }

    void on_meta_comment(const QByteArray &, const QByteArray &) override
    {
    }

    void on_fill(const GraphicsState &gstate) override
    {
        last_fill = gstate;
    }
};

template<class T> Value stack_val(T arg) { return Value(arg); }

template<class... T>
std::vector<Value> stack_vals(T... args)
{
    return {stack_val(args)...};
}


#define COMPARE_VALUE(val, typenum, expect) \
    QCOMPARE(val.type(), typenum); \
    QCOMPARE(val, Value(expect))

#define COMPARE_TOKEN(val, toktype, typenum, expect) \
    do { auto token_ = val; \
    QCOMPARE(token_.type, toktype); \
    COMPARE_VALUE(token_.value, typenum, expect); \
    } while(false)

#define COMPARE_CUSTOM(actual, expected, act_txt, exp_txt) \
    if (!QTest::qCompare(actual, expected, #act_txt, #exp_txt, __FILE__, __LINE__)) \
        QTEST_FAIL_ACTION;

#define COMPARE_PARSE(code, ...) \
    do { \
        TestInterpreter interp_; \
        interp_.exec_string(code); \
        QCOMPARE(interp_.last_error, QString()); \
        COMPARE_CUSTOM(interp_.stack_values(), stack_vals(__VA_ARGS__), code, (__VA_ARGS__)); \
    } while (false)

#define COMPARE_SHAPE(code, bez) \
    do { \
            TestInterpreter interp_; \
            interp_.exec_string(code); \
            QCOMPARE(interp_.last_error, QString()); \
            COMPARE_MULTIBEZIER(interp_.memory().gstate.path, bez); \
    } while (false)

class TestCase: public QObject, public BezierTestBase
{
    Q_OBJECT

    Token lex(const char* text)
    {
        return StringLexer(text).next_token();
    }


    GraphicsState exec_gstate(const char* text)
    {
        TestInterpreter interp;
        interp.exec_string(text);
        return interp.memory().gstate;
    }

private Q_SLOTS:
    void test_lex_comment()
    {
        auto token = lex("% foo bar\nbaz");
        COMPARE_TOKEN(token, Token::Comment, Value::String, " foo bar");
    }

    void test_lex_int()
    {
        StringLexer lexer("123 -98 43445 0 +17");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 123);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, -98);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 43445);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 0);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 17);
        COMPARE_TOKEN(lex("10z"), Token::Literal, Value::String, "10z");
        COMPARE_TOKEN(lex("++10"), Token::Literal, Value::String, "++10");
    }

    void test_lex_radix()
    {
        StringLexer lexer("8#1777 16#fffe 2#1000 36#Glax");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 01777);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 0xfffe);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 0b1000);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 774105);

        COMPARE_TOKEN(lex("2#102"), Token::Literal, Value::String, "2#102");
        COMPARE_TOKEN(lex("2#10~"), Token::Literal, Value::String, "2#10~");
    }

    void test_lex_float()
    {
        StringLexer lexer("-0.25 34.5 -23.6e4 1.0E-5 1E6 -1. .5");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Real, -0.25);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Real, 34.5);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Real, -23.6e4);

        COMPARE_TOKEN(lex("0.2.3"), Token::Literal, Value::String, "0.2.3");
        COMPARE_TOKEN(lex("0.45f"), Token::Literal, Value::String, "0.45f");
        COMPARE_TOKEN(lex("e6"), Token::Literal, Value::String, "e6");
    }

    void test_lex_string()
    {
        StringLexer lexer("(Hello World) (Hello (World)) (Hello\nWorld) (Hello\\nWorld) (Hello\\\nWorld)");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Hello World");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Hello (World)");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Hello\nWorld");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Hello\nWorld");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "HelloWorld");
    }

    void test_lex_hex_string()
    {
        StringLexer lexer("<48656c6c6f> <48656c7>");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Hello"_ba);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Help"_ba);
        QCOMPARE(lex("<48656z7>").type, Token::Unrecoverable);
    }

    void test_lex_double_angle()
    {
        StringLexer lexer("<< >>");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "<<");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, ">>");
    }

    void test_lex_base85()
    {
        StringLexer lexer("<~9jqo^~> <~9j qo^~> <~zz~> <~/c~>");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Man "_ba);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Man "_ba);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "\0\0\0\0\0\0\0\0"_ba);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "\56\3\31\264"_ba);
    }

    void test_lex_operators()
    {
        StringLexer lexer("abc Offset $$ 23A 13-456 a.b $MyDict @pattern");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "abc");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "Offset");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "$$");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "23A");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "13-456");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "a.b");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "$MyDict");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "@pattern");
    }

    void test_lex_name()
    {
        StringLexer lexer("/foo /bar/baz");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "foo");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "bar");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "baz");
    }

    void test_lex_mark()
    {
        StringLexer lexer("[123 /abc]<</1 2>>");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "[");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 123);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "abc");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "]");

        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "<<");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "1");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 2);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, ">>");
    }

    void test_parse_vals()
    {
        COMPARE_PARSE("1 /123 2", 1, "123", 2);
    }

    void test_parse_stack_ops()
    {
        COMPARE_PARSE("2 3 /4 clear",);
        COMPARE_PARSE("1 2 3 dup", 1, 2, 3, 3);
        COMPARE_PARSE("1 2 3 exch", 1, 3, 2);
        COMPARE_PARSE("1 2 3 pop", 1, 2);
        COMPARE_PARSE("1 2 3 4 5 6  2 copy", 1, 2, 3, 4, 5, 6, 5, 6);
        COMPARE_PARSE("10 20 30 40 1 index", 10, 20, 30, 40, 30);
        COMPARE_PARSE("1 2 3 (a) count", 1, 2, 3, "a", 4);
    }

    void test_parse_roll()
    {

        COMPARE_PARSE("1 2 3 4 5 6  5 0 roll", 1, 2, 3, 4, 5, 6);
        COMPARE_PARSE("1 2 3 4 5 6  5 1 roll", 1, 6, 2, 3, 4, 5);
        COMPARE_PARSE("1 2 3 4 5 6  5 2 roll", 1, 5, 6, 2, 3, 4);
        COMPARE_PARSE("1 2 3 4 5 6  5 -1 roll", 1, 3, 4, 5, 6, 2);
        COMPARE_PARSE("1 2 3 4 5 6  5 -2 roll", 1, 4, 5, 6, 2, 3);

        COMPARE_PARSE("1 2 3 4 5 6  5 -5 roll", 1, 2, 3, 4, 5, 6);
        COMPARE_PARSE("1 2 3 4 5 6  5 5 roll", 1, 2, 3, 4, 5, 6);
        COMPARE_PARSE("1 2 3 4 5 6  5 6 roll", 1, 6, 2, 3, 4, 5);
        COMPARE_PARSE("1 2 3 4 5 6  5 -6 roll", 1, 3, 4, 5, 6, 2);
    }

    void test_parse_arithmetics()
    {
        COMPARE_PARSE("2 3 add", 5);
        COMPARE_PARSE("2 3.5 add", 5.5);
        COMPARE_PARSE("8 2 div", 4);
        COMPARE_PARSE("9 2 div", 4.5);
        COMPARE_PARSE("9 2 idiv", 4);
        COMPARE_PARSE("9 2 mod", 1);
        COMPARE_PARSE("9 2 mul", 18);
        COMPARE_PARSE("9 2.5 mul", 22.5);
        COMPARE_PARSE("9.5 2 mul", 19);
        COMPARE_PARSE("2 3 sub", -1);
        COMPARE_PARSE("2 3.5 sub", -1.5);

        COMPARE_PARSE("2 neg 2.5 neg", -2, -2.5);
        COMPARE_PARSE("2 abs -2 abs 2.5 abs -2.5 abs", 2, 2, 2.5, 2.5);
    }

    void test_parse_round()
    {
        COMPARE_PARSE("2.2 ceiling -2.2 ceiling 2.8 ceiling -2.8 ceiling", 3, -2, 3, -2);
        COMPARE_PARSE("2.2 floor -2.2 floor 2.8 floor -2.8 floor", 2, -3, 2, -3);
        COMPARE_PARSE("2.2 round -2.2 round 2.8 round -2.8 round", 2, -2, 3, -3);
        COMPARE_PARSE("2.2 truncate -2.2 truncate 2.8 truncate -2.8 truncate", 2, -2, 2, -2);
    }


    void test_value_to_string()
    {
        QCOMPARE(Value(1).to_string(), "1");
        QCOMPARE(Value("abc").to_string(), "abc");
        QCOMPARE(Value(2.5f).to_string(), "2.5");
        QCOMPARE(Value("abc"_ba).to_string(), "abc");
        QCOMPARE(Value::from<Value::Mark>().to_string(), "--nostringval--");
        QCOMPARE(Value::from<Value::Null>().to_string(), "--nostringval--");
        QCOMPARE(Value::from<Value::Array>({1, 2, "abc"}).to_string(), "--nostringval--");

        QCOMPARE(Value(2.5f).to_pretty_string(), "2.5");
        QCOMPARE(Value::from<Value::Mark>().to_pretty_string(), "mark");
        QCOMPARE(Value::from<Value::Null>().to_pretty_string(), "null");
        QCOMPARE(Value::from<Value::Array>({1, 2, "abc"}).to_pretty_string(), "[1 2 (abc)]");
    }

    void test_parse_interactive()
    {
        TestInterpreter interp;
        QCOMPARE(interp.exec_string("(foo) 1 2"), stack_vals("foo", 1, 2));

        QCOMPARE(interp.exec_string("(bar)"), stack_vals("foo", 1, 2, "bar"));
        QCOMPARE(interp.exec_string("=="), stack_vals("foo", 1, 2));
        QCOMPARE(interp.consume_output(), "(bar)\n");

        QCOMPARE(interp.exec_string("(bar) ="), stack_vals("foo", 1, 2));
        QCOMPARE(interp.consume_output(), "bar\n");

        QCOMPARE(interp.exec_string("pstack"), stack_vals("foo", 1, 2));
        QCOMPARE(interp.consume_output(), "2\n1\n(foo)\n");
        QCOMPARE(interp.exec_string("stack"), stack_vals("foo", 1, 2));
        QCOMPARE(interp.consume_output(), "2\n1\nfoo\n");
    }

    void test_parse_mark()
    {
        TestInterpreter interp;
        interp.exec_string("1 2 3 mark 4 5 6");
        QCOMPARE(interp.stack_values(), stack_vals(1, 2, 3, Value::from<Value::Mark>(), 4, 5, 6));
        QCOMPARE(interp.stack()[3].type(), Value::Mark);

        interp.exec_string("counttomark");
        QCOMPARE(interp.stack_values(), stack_vals(1, 2, 3, Value::from<Value::Mark>(), 4, 5, 6, 3));

        interp.exec_string("cleartomark");
        QCOMPARE(interp.stack_values(), stack_vals(1, 2, 3));

        interp.exec_string("[ <<");
        QCOMPARE(interp.stack()[0].type(), Value::Mark);
        QCOMPARE(interp.stack()[1].type(), Value::Mark);
    }
    void test_parse_pow()
    {
        COMPARE_PARSE("6.25 sqrt", 2.5);
        COMPARE_PARSE("2 4 exp", 16);
        COMPARE_PARSE("5 ln", std::log(5.f));
        COMPARE_PARSE("100 log", 2);
    }

    void test_parse_trig()
    {
        COMPARE_PARSE("2 -2 atan", 135);
        QCOMPARE(TestInterpreter().exec_string("90 cos")[0].cast<float>(), 0.f);
        COMPARE_PARSE("90 sin", 1);
    }

    void test_exec()
    {
        TestInterpreter interp;
        interp.exec_string("1 2 {3 add}");
        QCOMPARE(interp.stack()[2], Value(1));
        QCOMPARE(interp.stack()[1], Value(2));
        QCOMPARE(interp.stack()[0].type(), Value::Array);
        QCOMPARE(interp.stack()[0].attributes(), Value::Executable|Value::Writable|Value::Readable);
        QCOMPARE(interp.stack()[0].cast<ValueArray>(), ValueArray({3, "add"}));
        interp.exec_string("exec");
        QCOMPARE(interp.stack_values(), stack_vals(1, 5));
    }

    void test_comments()
    {
        TestInterpreter interp;
        interp.set_level_autodetect(true);
        interp.exec_string(R"(%!PS-Adobe-3.0
%%Creator: Creator Value
%%CreationDate: Sat Apr  4 08:10:50 2026
%%Pages: 1
%%DocumentData: Clean7Bit
%%LanguageLevel: 2
%%BoundingBox: 112 199 212 279
%%EndComments
%%BeginProlog
%%EndProlog
%%BeginSetup
%%EndSetup
%%NotAHeader: foobar
%%Page: 1 1
%%BeginPageSetup
%%PageBoundingBox: 112 199 212 279
%%EndPageSetup

%%Trailer
%%EOF
)");
        ValueDict docs = {
            {"Creator"_ba, "Creator Value"_ba},
            {"CreationDate"_ba, "Sat Apr  4 08:10:50 2026"_ba},
            {"Pages"_ba, "1"_ba},
            {"DocumentData"_ba, "Clean7Bit"_ba},
            {"LanguageLevel"_ba, "2"_ba},
            {"BoundingBox"_ba, "112 199 212 279"_ba},
        };
        QCOMPARE(interp.document_metadata(), docs);
        ValueDict page = {
            {"Page"_ba, "1 1"_ba},
            {"PageBoundingBox"_ba, "112 199 212 279"_ba},
            {"PageSize"_ba, ValueArray({595, 841})},
        };
        QCOMPARE(interp.page_metadata(), page);
        QCOMPARE(interp.level(), Level::PS2);

    }

    void test_type_name()
    {
        QCOMPARE(Value::type_name(Value::Integer), "integertype");
        QCOMPARE(Value::type_name(Value::Array), "arraytype");
    }

    void test_string()
    {
        COMPARE_PARSE("(Rawr)", "Rawr");
        COMPARE_PARSE("3 string", "\0\0\0"_ba);
        COMPARE_PARSE("(Hello) 1 get", 101);
        COMPARE_PARSE("(Hello) dup 1 97 put", "Hallo");
        COMPARE_PARSE("(Hello) 1 3 getinterval", "ell");
        COMPARE_PARSE("(Hello) dup 2 (ww) putinterval", "Hewwo");
        COMPARE_PARSE("(hello world) dup (foo) exch copy", "foolo world", "foo");

        COMPARE_PARSE("(Hello) (Hell) anchorsearch", "o", "Hell", true);
        COMPARE_PARSE("(Hello) (ell) anchorsearch", "Hello", false);
        COMPARE_PARSE("(Hello) (ell) search", "o", "ell", "H", true);
        COMPARE_PARSE("(Hello) (Hell) search", "o", "Hell", "", true);
        COMPARE_PARSE("(Hello) (llo) search", "", "llo", "He", true);
        COMPARE_PARSE("(Hello) (foo) search", "Hello", false);
    }

    void test_token()
    {
        COMPARE_PARSE("(123) token", "", 123, true);
        COMPARE_PARSE("(123 foo) token", "foo", 123, true);
        COMPARE_PARSE("(%foo 123) token", false);
        COMPARE_PARSE("(%foo\n123) token", "", 123, true);
        COMPARE_PARSE("({123) token", false);

        TestInterpreter interp;
        interp.exec_string("({ 123 }345) token");
        QCOMPARE(interp.last_error, "");
        QCOMPARE(interp.stack_values(), stack_vals("345", ValueArray({123}), true));
        QVERIFY(interp.stack()[1].has_attribute(Value::Executable));
    }

    void test_bool()
    {
        COMPARE_PARSE("true false", true, false);
    }

    void test_array()
    {
        COMPARE_PARSE("[1 2 3]", ValueArray({1, 2, 3}));
        COMPARE_PARSE("3 array", ValueArray({Value(), Value(), Value()}));
        COMPARE_PARSE("[1 2 3] length", 3);
        COMPARE_PARSE("[10 20 30] 1 get", 20);
        COMPARE_PARSE("[1 2 3] length", 3);
        COMPARE_PARSE("[1 2 3] dup 1 99 put", ValueArray({1, 99, 3}));
        COMPARE_PARSE("[9 8 7 6 5] 1 3 getinterval", ValueArray({8, 7, 6}));

        COMPARE_PARSE("[1 2 3 4 5 6 7] dup 2 [(a) (b) (c)] putinterval", ValueArray({1, 2, "a", "b", "c", 6, 7}));
        COMPARE_PARSE("1 [2 3 4] aload", 1, 2, 3, 4, ValueArray({2, 3, 4}));
        COMPARE_PARSE("[1 2 3 4 5 6 7] dup [(a) (b) (c)] exch copy", ValueArray({"a", "b", "c", 4, 5, 6, 7}), ValueArray({"a", "b", "c"}));

        COMPARE_PARSE("1 2 3 3 packedarray", ValueArray({1, 2, 3}));
        COMPARE_PARSE("true setpacking",);
        COMPARE_PARSE("currentpacking", false);
    }

    void test_dict()
    {
        // basic
        COMPARE_PARSE("<<(foo) 1 (bar) 2>>", ValueDict({{"foo", 1}, {"bar", 2}}));
        COMPARE_PARSE("5 dict", ValueDict());
        COMPARE_PARSE("<<(foo) 1 (bar) 2>> length", 2);
        COMPARE_PARSE("<<(foo) 1 (bar) 2>> maxlength", 2);
        COMPARE_PARSE("<<(foo) 1 (bar) 2>> (bar) get", 2);
        COMPARE_PARSE("<</foo 1>> dup /foo known exch /bar known", true, false);

        // modify
        COMPARE_PARSE("<<>> dup /foo 123 put", ValueDict({{"foo", 123}}));
        COMPARE_PARSE("<</bar 3>> dup /foo undef", ValueDict({{"bar", 3}}));
        COMPARE_PARSE("<</foo 1 /bar 3>> dup /foo undef", ValueDict({{"bar", 3}}));
        COMPARE_PARSE("<</bar 2 /baz 3>> dup <</foo 1 /baz 4>> exch copy", ValueDict({{"foo", 1}, {"bar", 2}, {"baz", 4}}), ValueDict({{"foo", 1}, {"bar", 2}, {"baz", 4}}));

        // def
        COMPARE_PARSE("/foo 123 def /foo load", 123);
        COMPARE_PARSE("/foo 123 def foo", 123);
        COMPARE_PARSE("/foo 123 def foo", 123);
        COMPARE_PARSE("/foo {1 2 add} def foo", 3);

        // begin / end
        COMPARE_PARSE("<<>> dup begin /foo 123 def",  ValueDict({{"foo", 123}}));
        COMPARE_PARSE("<<>> dup begin /foo 123 def end /bar 456 def",  ValueDict({{"foo", 123}}));
        COMPARE_PARSE("<</foo 123>> begin /foo load", 123);
        COMPARE_PARSE("<</foo 123>> begin <</bar 123>> begin /foo where", ValueDict({{"foo", 123}}), true);
        COMPARE_PARSE("<</food 123>> begin <</bar 123>> begin /foo where", false);

        // store
        COMPARE_PARSE("<<>> dup begin <<>> dup begin /foo 123 def", ValueDict(), ValueDict({{"foo", 123}}));
        COMPARE_PARSE("<<>> dup begin <<>> dup begin /foo 123 store", ValueDict(), ValueDict({{"foo", 123}}));
        COMPARE_PARSE("<</foo 4>> dup begin <<>> dup begin /foo 123 def", ValueDict({{"foo", 4}}), ValueDict({{"foo", 123}}));
        COMPARE_PARSE("<</foo 4>> dup begin <<>> dup begin /foo 123 store", ValueDict({{"foo", 123}}), ValueDict());
    }

    void test_forall()
    {
        TestInterpreter interp;
        interp.exec_string("(Hello) {=} forall");
        QCOMPARE(interp.last_error, "");
        QCOMPARE(interp.stack().size(), 0);
        QCOMPARE(interp.consume_output(), "72\n101\n108\n108\n111\n");

        interp.exec_string("[1 2 3 4] {=} forall");
        QCOMPARE(interp.last_error, "");
        QCOMPARE(interp.stack().size(), 0);
        QCOMPARE(interp.consume_output(), "1\n2\n3\n4\n");


        interp.exec_string("<</foo 1 /bar 2>> {= =} forall");
        QCOMPARE(interp.last_error, "");
        QCOMPARE(interp.stack().size(), 0);
        QString out = interp.consume_output();
        QVERIFY(out == "1\nfoo\n2\nbar\n" || out == "2\nbar\n1\nfoo\n");
    }

    void test_cmp()
    {
        COMPARE_PARSE("1 1 eq", true);
        COMPARE_PARSE("1 1.0 eq", true);
        COMPARE_PARSE("1 (1) eq", false);
        COMPARE_PARSE("1 2 eq", false);
        COMPARE_PARSE("[ << eq", true);
        COMPARE_PARSE("null null eq", true);
        COMPARE_PARSE("[ null eq", false);
        COMPARE_PARSE("0 false eq", false);
        COMPARE_PARSE("0 false eq", false);
        COMPARE_PARSE("(foo) (foo) eq", true);
        COMPARE_PARSE("(foo) (food) eq", false);
        COMPARE_PARSE("[1] [1] eq", false);
        COMPARE_PARSE("[1] dup eq", true);
        COMPARE_PARSE("<</foo 1>> <</foo 1>> eq", false);
        COMPARE_PARSE("<</foo 1>> dup eq", true);

        COMPARE_PARSE("1 1 ne", false);
        COMPARE_PARSE("1 1.0 ne", false);
        COMPARE_PARSE("1 (1) ne", true);
        COMPARE_PARSE("1 2 ne", true);
        COMPARE_PARSE("[ << ne", false);
        COMPARE_PARSE("null null ne", false);
        COMPARE_PARSE("[ null ne", true);
        COMPARE_PARSE("0 false ne", true);
        COMPARE_PARSE("0 false ne", true);
        COMPARE_PARSE("(foo) (foo) ne", false);
        COMPARE_PARSE("(foo) (food) ne", true);
        COMPARE_PARSE("[1] [1] ne", true);
        COMPARE_PARSE("[1] dup ne", false);
        COMPARE_PARSE("<</foo 1>> <</foo 1>> ne", true);
        COMPARE_PARSE("<</foo 1>> dup ne", false);

        COMPARE_PARSE("1 2 lt", true);
        COMPARE_PARSE("1 2.3 lt", true);
        COMPARE_PARSE("1.2 2 lt", true);
        COMPARE_PARSE("1.2 2.3 lt", true);
        COMPARE_PARSE("(abc) (abcd) lt", true);
        COMPARE_PARSE("(abc) (zzz) lt", true);
        COMPARE_PARSE("1 1 lt", false);
        COMPARE_PARSE("(abc) (abc) lt", false);
        COMPARE_PARSE("2 1 lt", false);

        COMPARE_PARSE("1 2 le", true);
        COMPARE_PARSE("1 2.3 le", true);
        COMPARE_PARSE("1.2 2 le", true);
        COMPARE_PARSE("1.2 2.3 le", true);
        COMPARE_PARSE("(abc) (abcd) le", true);
        COMPARE_PARSE("(abc) (zzz) le", true);
        COMPARE_PARSE("1 1 le", true);
        COMPARE_PARSE("(abc) (abc) le", true);
        COMPARE_PARSE("2 1 le", false);

        COMPARE_PARSE("2 1 gt", true);
        COMPARE_PARSE("2 1.3 gt", true);
        COMPARE_PARSE("2.2 1 gt", true);
        COMPARE_PARSE("2.2 1.3 gt", true);
        COMPARE_PARSE("(abcd) (abc) gt", true);
        COMPARE_PARSE("(zab) (fab) gt", true);
        COMPARE_PARSE("1 1 gt", false);
        COMPARE_PARSE("(abc) (abc) gt", false);
        COMPARE_PARSE("1 2 gt", false);

        COMPARE_PARSE("2 1 ge", true);
        COMPARE_PARSE("2 1.3 ge", true);
        COMPARE_PARSE("2.2 1 ge", true);
        COMPARE_PARSE("2.2 1.3 ge", true);
        COMPARE_PARSE("(abcd) (abc) ge", true);
        COMPARE_PARSE("(zab) (fab) ge", true);
        COMPARE_PARSE("1 1 ge", true);
        COMPARE_PARSE("(abc) (abc) ge", true);
        COMPARE_PARSE("1 2 ge", false);
    }

    void test_boolops()
    {
        COMPARE_PARSE("true true and", true);
        COMPARE_PARSE("true false and", false);
        COMPARE_PARSE("false true and", false);
        COMPARE_PARSE("false false and", false);

        COMPARE_PARSE("true true or", true);
        COMPARE_PARSE("true false or", true);
        COMPARE_PARSE("false true or", true);
        COMPARE_PARSE("false false or", false);

        COMPARE_PARSE("true true xor", false);
        COMPARE_PARSE("true false xor", true);
        COMPARE_PARSE("false true xor", true);
        COMPARE_PARSE("false false xor", false);

        COMPARE_PARSE("true not", false);
        COMPARE_PARSE("false not", true);
    }

    void test_bitops()
    {
        COMPARE_PARSE("2#1100 2#1010 and", 0b1000);
        COMPARE_PARSE("2#1100 2#1010 or",  0b1110);
        COMPARE_PARSE("2#1100 2#1010 xor", 0b0110);
        COMPARE_PARSE("52 not", -53);
        COMPARE_PARSE("2#11010 3 bitshift", 0b11010000);
        COMPARE_PARSE("2#11010 -3 bitshift", 0b11);
    }

    void test_if()
    {
        COMPARE_PARSE("false {123} if",);
        COMPARE_PARSE("true {123} if", 123);
        COMPARE_PARSE("false {123} {456} ifelse", 456);
        COMPARE_PARSE("true {123} {456} ifelse", 123);
    }

    void test_bounded_loop()
    {
        COMPARE_PARSE("4 {123} repeat", 123, 123, 123, 123);
        COMPARE_PARSE("0 {123} repeat",);

        COMPARE_PARSE("0 1 4 {} for", 0, 1, 2, 3, 4);
        COMPARE_PARSE("1 2 6 {} for", 1, 3, 5);
        COMPARE_PARSE("100 1 1 4 {add} for", 110);
        COMPARE_PARSE("3 -.5 1 {} for", 3.0, 2.5, 2.0, 1.5, 1.0);
    }

    void test_loop_exit()
    {
        TestInterpreter interp;
        interp.exec_string("6 7 (foo) {count 0 eq {exit} if =} loop");
        QCOMPARE(interp.stack_values(), std::vector<Value>());
        QCOMPARE(interp.consume_output(), "foo\n7\n6\n");
        QCOMPARE(interp.last_error, "");

        COMPARE_PARSE("(foobar) { dup 98 eq {exit} if (a) } forall", 102, "a", 111, "a", 111, "a", 98);
        COMPARE_PARSE("[1 2 3 4 5] { dup 3 eq {exit} if (a) } forall", 1, "a", 2, "a", 3);
        COMPARE_PARSE("1 100 {1 add dup 10 eq {exit} if} repeat", 10);
        COMPARE_PARSE("1 {1 add dup 10 eq {exit} if} loop", 10);
    }

    void test_stop()
    {
        COMPARE_PARSE("{1 2 stop 3} stopped (a)", 1, 2, true, "a");
        COMPARE_PARSE("{1 2 3} stopped (a)", 1, 2, 3, false, "a");
        QCOMPARE(TestInterpreter().exec_string("{1 2 get 3} stopped (a)"), stack_vals(1, 2, true, "a"));

        COMPARE_PARSE("{1 2 quit 3} stopped (a)", 1, 2, false);
    }

    void test_attributes()
    {
        COMPARE_PARSE("[1] type", "arraytype");
        COMPARE_PARSE("{1} type", "arraytype");
        COMPARE_PARSE("true type", "booleantype");
        COMPARE_PARSE("<</foo 1>> type", "dicttype");
        COMPARE_PARSE("1 type", "integertype");
        COMPARE_PARSE("[ type", "marktype");
        COMPARE_PARSE("/foo type", "nametype");
        COMPARE_PARSE("null type", "nulltype");
        COMPARE_PARSE("1.0 type", "realtype");
        COMPARE_PARSE("(foo) type", "stringtype");

        QCOMPARE(TestInterpreter().exec_string("1")[0].attributes(), Value::None);
        QCOMPARE(TestInterpreter().exec_string("1 cvx")[0].attributes(), Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("1 cvx cvlit")[0].attributes(), Value::None);
        COMPARE_PARSE("1 xcheck", false);
        COMPARE_PARSE("1 cvx xcheck", true);

        QCOMPARE(TestInterpreter().exec_string("1 array")[0].attributes(), Value::Readable|Value::Writable);
        QCOMPARE(TestInterpreter().exec_string("1 array cvx")[0].attributes(), Value::Readable|Value::Writable|Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("1 array cvx cvlit")[0].attributes(), Value::Readable|Value::Writable);
        QCOMPARE(TestInterpreter().exec_string("1 array executeonly")[0].attributes(), Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("1 array readonly")[0].attributes(), Value::Readable);
        QCOMPARE(TestInterpreter().exec_string("1 array noaccess")[0].attributes(), 0);
        COMPARE_PARSE("1 array xcheck", false);
        COMPARE_PARSE("1 array wcheck", true);
        COMPARE_PARSE("1 array rcheck", true);

        QCOMPARE(TestInterpreter().exec_string("[]")[0].attributes(), Value::Readable|Value::Writable);
        QCOMPARE(TestInterpreter().exec_string("[] cvx")[0].attributes(), Value::Readable|Value::Writable|Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("[] cvx cvlit")[0].attributes(), Value::Readable|Value::Writable);
        QCOMPARE(TestInterpreter().exec_string("[] executeonly")[0].attributes(), Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("[] readonly")[0].attributes(), Value::Readable);
        QCOMPARE(TestInterpreter().exec_string("[] noaccess")[0].attributes(), 0);
        COMPARE_PARSE("[] xcheck", false);
        COMPARE_PARSE("[] wcheck", true);
        COMPARE_PARSE("[] rcheck", true);

        QCOMPARE(TestInterpreter().exec_string("{}")[0].attributes(), Value::Readable|Value::Writable|Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("{} cvx")[0].attributes(), Value::Readable|Value::Writable|Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("{} cvlit")[0].attributes(), Value::Readable|Value::Writable);
        QCOMPARE(TestInterpreter().exec_string("{} executeonly")[0].attributes(), Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("{} readonly")[0].attributes(), Value::Readable|Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("{} noaccess")[0].attributes(), 0);
        COMPARE_PARSE("{} xcheck", true);
        COMPARE_PARSE("{} wcheck", true);
        COMPARE_PARSE("{} rcheck", true);

        QCOMPARE(TestInterpreter().exec_string("(a)")[0].attributes(), Value::Readable|Value::Writable);
        QCOMPARE(TestInterpreter().exec_string("(a) cvx")[0].attributes(), Value::Readable|Value::Writable|Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("(a) cvx cvlit")[0].attributes(), Value::Readable|Value::Writable);
        QCOMPARE(TestInterpreter().exec_string("(a) executeonly")[0].attributes(), Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("(a) readonly")[0].attributes(), Value::Readable);
        QCOMPARE(TestInterpreter().exec_string("(a) noaccess")[0].attributes(), 0);
        COMPARE_PARSE("(a) xcheck", false);
        COMPARE_PARSE("(a) wcheck", true);
        COMPARE_PARSE("(a) rcheck", true);


        QCOMPARE(TestInterpreter().exec_string("1 dict")[0].attributes(), Value::Readable|Value::Writable);
        QCOMPARE(TestInterpreter().exec_string("1 dict cvx")[0].attributes(), Value::Readable|Value::Writable|Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("1 dict cvx cvlit")[0].attributes(), Value::Readable|Value::Writable);
        QCOMPARE(TestInterpreter().exec_string("1 dict executeonly")[0].attributes(), Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("1 dict readonly")[0].attributes(), Value::Readable);
        QCOMPARE(TestInterpreter().exec_string("1 dict noaccess")[0].attributes(), 0);
        COMPARE_PARSE("1 dict xcheck", false);
        COMPARE_PARSE("1 dict wcheck", true);
        COMPARE_PARSE("1 dict rcheck", true);

        QCOMPARE(TestInterpreter().exec_string("/foo")[0].attributes(), Value::Readable|Value::Name);
        QCOMPARE(TestInterpreter().exec_string("/foo cvx")[0].attributes(), Value::Readable|Value::Executable|Value::Name);
        QCOMPARE(TestInterpreter().exec_string("/foo cvx cvlit")[0].attributes(), Value::Readable|Value::Name);
        QCOMPARE(TestInterpreter().exec_string("/foo executeonly")[0].attributes(), Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("/foo readonly")[0].attributes(), Value::Readable|Value::Name);
        QCOMPARE(TestInterpreter().exec_string("/foo noaccess")[0].attributes(), 0);
        COMPARE_PARSE("/foo xcheck", false);
        COMPARE_PARSE("/foo wcheck", false);
        COMPARE_PARSE("/foo rcheck", true);

        QCOMPARE(TestInterpreter().exec_string("{foo} 0 get")[0].attributes(), Value::Readable|Value::Executable|Value::Name);
        QCOMPARE(TestInterpreter().exec_string("{foo} 0 get cvx")[0].attributes(), Value::Readable|Value::Executable|Value::Name);
        QCOMPARE(TestInterpreter().exec_string("{foo} 0 get cvlit")[0].attributes(), Value::Readable|Value::Name);
        QCOMPARE(TestInterpreter().exec_string("{foo} 0 get executeonly")[0].attributes(), Value::Executable);
        QCOMPARE(TestInterpreter().exec_string("{foo} 0 get readonly")[0].attributes(), Value::Readable|Value::Executable|Value::Name);
        QCOMPARE(TestInterpreter().exec_string("{foo} 0 get noaccess")[0].attributes(), 0);
        COMPARE_PARSE("{foo} 0 get xcheck", true);
        COMPARE_PARSE("{foo} 0 get wcheck", false);
        COMPARE_PARSE("{foo} 0 get rcheck", true);
    }

    void test_conversions()
    {
        COMPARE_PARSE("1 cvi", 1);
        COMPARE_PARSE("10.9 cvi", 10);
        COMPARE_PARSE("-10.9 cvi", -10);
        COMPARE_PARSE("(10.9) cvi", 10);
        COMPARE_PARSE("(10) cvi", 10);
        COMPARE_PARSE("(2#100) cvi", 4);


        COMPARE_PARSE("(string) cvn", "string");
        QCOMPARE(TestInterpreter().exec_string("(string) cvn")[0].attributes(), Value::Readable|Value::Writable|Value::Name);

        COMPARE_PARSE("1 cvr", 1.0);
        COMPARE_PARSE("10.5 cvr", 10.5);
        COMPARE_PARSE("(10.5) cvr", 10.5);
        COMPARE_PARSE("(2#100) cvr", 4.0);

        COMPARE_PARSE("1  10 string cvs", "1");
        COMPARE_PARSE("10.5 10 string cvs", "10.5");
        COMPARE_PARSE("true 10 string cvs", "true");
        COMPARE_PARSE("false 10 string cvs", "false");
        COMPARE_PARSE("(foo) 10 string cvs", "foo");
        COMPARE_PARSE("/foo 10 string cvs", "foo");
        COMPARE_PARSE("[] 16 string cvs", "--nostringval--");
        COMPARE_PARSE("<<>> 16 string cvs", "--nostringval--");
        COMPARE_PARSE("mark 16 string cvs", "--nostringval--");
        COMPARE_PARSE("null 16 string cvs", "--nostringval--");


        COMPARE_PARSE("  10   10 10 string cvrs", "10");
        COMPARE_PARSE("  10.5 10 10 string cvrs", "10.5");
        COMPARE_PARSE(" 123   10 10 string cvrs", "123");
        COMPARE_PARSE("-123   10 10 string cvrs", "-123");
        COMPARE_PARSE(" 123.4 10 10 string cvrs", "123.4");
        COMPARE_PARSE(" 123   16 10 string cvrs", "7B");
        COMPARE_PARSE("-123   16 10 string cvrs", "FFFFFF85");
        COMPARE_PARSE(" 123.4 16 10 string cvrs", "7B");
    }

    void test_gstate_basics()
    {
        QCOMPARE(exec_gstate("").line_width, 1);
        QCOMPARE(exec_gstate("20 setlinewidth").line_width, 20);
        COMPARE_PARSE("currentlinewidth 20 setlinewidth currentlinewidth", 1, 20);

        QCOMPARE(exec_gstate("").line_cap, glaxnimate::model::Stroke::ButtCap);
        QCOMPARE(exec_gstate("1 setlinecap").line_cap, glaxnimate::model::Stroke::RoundCap);
        QCOMPARE(exec_gstate("2 setlinecap").line_cap, glaxnimate::model::Stroke::SquareCap);
        COMPARE_PARSE("currentlinecap 2 setlinecap currentlinecap", 0, 2);

        QCOMPARE(exec_gstate("").line_join, glaxnimate::model::Stroke::MiterJoin);
        QCOMPARE(exec_gstate("1 setlinejoin").line_join, glaxnimate::model::Stroke::RoundJoin);
        QCOMPARE(exec_gstate("2 setlinejoin").line_join, glaxnimate::model::Stroke::BevelJoin);
        COMPARE_PARSE("currentlinejoin 2 setlinejoin currentlinejoin", 0, 2);

        QCOMPARE(exec_gstate("").miter_limit, 10);
        QCOMPARE(exec_gstate("1 setmiterlimit").miter_limit, 1);
        COMPARE_PARSE("currentmiterlimit 2 setmiterlimit currentmiterlimit", 10, 2);

        QCOMPARE(exec_gstate("20 setlinewidth gsave").line_width, 20);
        QCOMPARE(exec_gstate("20 setlinewidth gsave 10 setlinewidth").line_width, 10);
        QCOMPARE(exec_gstate("20 setlinewidth gsave 10 setlinewidth grestore").line_width, 20);

    }

    void test_gstate_colors()
    {
        QCOMPARE(exec_gstate(".5 setgray").color, QColor::fromRgbF(0.5, 0.5, 0.5));
        QCOMPARE(exec_gstate("1 .5 0.25 setrgbcolor").color, QColor::fromRgbF(1, 0.5, 0.25));
        QCOMPARE(exec_gstate("1 .5 0.25 sethsbcolor").color, QColor::fromHsvF(1, 0.5, 0.25));
        QCOMPARE(exec_gstate("1 .5 0.25 0.125 setcmykcolor").color, QColor::fromCmykF(1, 0.5, 0.25, 0.125));
        QCOMPARE(exec_gstate("/DeviceRGB setcolorspace 1 .5 0.25 setcolor").color, QColor::fromRgbF(1, 0.5, 0.25));

        COMPARE_PARSE("currentgray .5 setgray currentgray", 0, 0.5);
        COMPARE_PARSE("1 .5 0.25 setrgbcolor currentrgbcolor", 1, .5, 0.25);
        COMPARE_PARSE("1 .5 0.25 sethsbcolor currenthsbcolor", 1, .5, 0.25);
    }

    void test_matrix_stack()
    {
        COMPARE_PARSE("matrix", ValueArray({1, 0, 0, 1, 0, 0}));
        COMPARE_PARSE("matrix 10 20 2 index translate", ValueArray({1, 0, 0, 1, 10, 20}), ValueArray({1, 0, 0, 1, 10, 20}));
        COMPARE_PARSE("matrix 10 20 2 index translate identmatrix", ValueArray({1, 0, 0, 1, 0, 0}), ValueArray({1, 0, 0, 1, 0, 0}));
        COMPARE_PARSE("10 20 matrix scale", ValueArray({10, 0, 0, 20, 0, 0}));
        COMPARE_PARSE("90 matrix rotate", ValueArray({0, 1, -1, 0, 0, 0}));
        COMPARE_PARSE("180 matrix rotate", ValueArray({-1, 0, 0, -1, 0, 0}));
        COMPARE_PARSE("10 20 matrix translate 2 3 matrix scale matrix concatmatrix 90 matrix rotate matrix concatmatrix",
            ValueArray({0, 2, -3, 0, -60, 20}));
        COMPARE_PARSE("[0.0 2.0 -3.0 0.0 -60.0 20.0] matrix matrix concatmatrix",
            ValueArray({0, 2, -3, 0, -60, 20}));
        COMPARE_PARSE("12 34 [0.0 2.0 -3.0 0.0 -60.0 20.0] transform", -162, 44);
        COMPARE_PARSE("12 34 [0.0 2.0 -3.0 0.0 -60.0 20.0] itransform", 7, -24);
        COMPARE_PARSE("12 34 [0.0 2.0 -3.0 0.0 -60.0 20.0] dtransform", -102, 24);
        COMPARE_PARSE("12 34 [0.0 2.0 -3.0 0.0 -60.0 20.0] idtransform", 17, -4);
        COMPARE_PARSE("[0.0 2.0 -3.0 0.0 -60.0 20.0] matrix invertmatrix", ValueArray({0, -1./3., .5, 0, -10, -20}));
    }

    void test_matrix_gstate()
    {
        TestInterpreter interp;
        QTransform default_tf = interp.memory().gstate.transform;
        auto dtfe = matrix_elements(default_tf);
        ValueArray default_matrix;
        for ( float e : dtfe )
            default_matrix.emplace_back(e);

        QCOMPARE(default_matrix, ValueArray({1, 0, 0, 1, 0, 0}));
        COMPARE_PARSE("matrix currentmatrix", default_matrix);
        COMPARE_PARSE("matrix defaultmatrix", default_matrix);
        COMPARE_PARSE("10 20 translate matrix currentmatrix", ValueArray({1, 0, 0, 1, 10, 20}));
        COMPARE_PARSE("10 20 scale matrix currentmatrix", ValueArray({10, 0, 0, 20, 0, 0}));
        COMPARE_PARSE("90 rotate matrix currentmatrix", ValueArray({0, 1, -1, 0, 0, 0}));
        COMPARE_PARSE("90 rotate 2 3 scale 10 20 translate matrix currentmatrix",
            ValueArray({0, 2, -3, 0, -60, 20}));

        interp.exec_string("90 rotate 2 3 scale 10 20 translate");
        QCOMPARE(interp.memory().gstate.transform, matrix_from_elements({0, 2, -3, 0, -60, 20}));
        interp.exec_string("[1 2 3 4 5 6] setmatrix");
        QCOMPARE(interp.memory().gstate.transform, matrix_from_elements({1, 2, 3, 4, 5, 6}));
        interp.exec_string("initmatrix");
        QCOMPARE(interp.memory().gstate.transform, default_tf);

        interp.stack().clear();
        interp.exec_string("[0.0 2.0 -3.0 0.0 -60.0 20.0] setmatrix");
        QCOMPARE(interp.exec_string("12 34 transform"), stack_vals(-162, 44));
        interp.stack().clear();
        QCOMPARE(interp.exec_string("12 34 itransform"), stack_vals(7, -24));
        interp.stack().clear();
        QCOMPARE(interp.exec_string("12 34 dtransform"), stack_vals(-102, 24));
        interp.stack().clear();
        QCOMPARE(interp.exec_string("12 34 idtransform"), stack_vals(17, -4));
        interp.stack().clear();
    }

    void test_path()
    {
        COMPARE_PARSE("10 20 moveto currentpoint", 10, 20);
        COMPARE_PARSE("10 20 moveto 30 40 lineto currentpoint", 30, 40);
        COMPARE_PARSE("10 20 moveto 30 40 rlineto currentpoint", 40, 60);

        COMPARE_SHAPE(R"(
            100 200 moveto
            200 300 lineto
            -100 0 rlineto
            )",
            math::bezier::Bezier(false, {
                {{100, 200}, {100, 200}, {100, 200}},
                {{200, 300}, {200, 300}, {200, 300}},
                {{100, 300}, {100, 300}, {100, 300}},
            })
        );
        COMPARE_SHAPE(R"(
            100 200 moveto
            100 0 rlineto
            0 -200 rlineto
            -100 0 rlineto
            closepath
            )",
            math::bezier::Bezier(true, {
                {{100, 200}, {100, 200}, {100, 200}},
                {{200, 200}, {200, 200}, {200, 200}},
                {{200, 0}, {200, 0}, {200, 0}},
                {{100, 0}, {100, 0}, {100, 0}},
            })
        );
        COMPARE_SHAPE(R"(
            200 200 moveto
            100 0 rlineto
            0 100 rlineto
            )",
            math::bezier::Bezier(false, {
                {{200, 200}, {200, 200}, {200, 200}},
                {{300, 200}, {300, 200}, {300, 200}},
                {{300, 300}, {300, 300}, {300, 300}},
            })
        );
        COMPARE_SHAPE(R"(
            100 200 moveto
            100 200 100 0 45 arc
            closepath
            )",
            math::bezier::Bezier(true, {
               {{100, 200}, {100, 200}, {100, 200}},
               {{200, 200}, {200, 200}, {200, 226.511}},
               {{170.711, 270.711}, {189.457, 251.964}, {170.711, 270.711}},
            })
        );
        COMPARE_SHAPE(R"(
            300 600 moveto
            300 600 100 0 45 arcn
            closepath
            )",
            math::bezier::Bezier(true, {
               {{300, 600}, {300, 600}, {300, 600}},
               {{400, 600}, {400, 600}, {400, 545.142}},
               {{300, 500}, {354.858, 500}, {245.142, 500}},
               {{200, 600}, {200, 545.142}, {200, 654.858}},
               {{300, 700}, {245.142, 700}, {326.511, 700}},
               {{370.711, 670.711}, {351.964, 689.457}, {370.711, 670.711}},
           })
            );
        COMPARE_SHAPE(R"(
            300 100 moveto
            300 200 400 100 20 arct
            400 100 lineto
            )",
            math::bezier::Bezier(false, {
                {{300, 100}, {300, 100}, {300, 100}},
                {{300, 151.716}, {300, 151.716}, {300, 162.687}},
                {{320, 171.716}, {309.028, 171.716}, {325.302, 171.716}},
                {{334.142, 165.858}, {330.393, 169.607}, {334.142, 165.858}},
                {{400, 100}, {400, 100}, {400, 100}},
            })
            );
        COMPARE_SHAPE(R"(
            400 100 moveto
            400 200 500 100 20 arcto
            pop pop lineto
            )",
            math::bezier::Bezier(false, {
                {{400, 100}, {400, 100}, {400, 100}},
                {{400, 151.716}, {400, 151.716}, {400, 162.687}},
                {{420, 171.716}, {409.028, 171.716}, {425.302, 171.716}},
                {{434.142, 165.858}, {430.393, 169.607}, {434.142, 165.858}},
                {{400, 151.716}, {400, 151.716}, {400, 151.716}},
            })
        );
        COMPARE_SHAPE(R"(
            300 300 moveto
            350 400
            450 400
            500 500 curveto
            500 300 lineto
            )",
            math::bezier::Bezier(false, {
                {{300, 300}, {300, 300}, {350, 400}},
                {{500, 500}, {450, 400}, {500, 500}},
                {{500, 300}, {500, 300}, {500, 300}},
            })
        );
        COMPARE_SHAPE(R"(
            300 200 moveto
            50 100
            150 100
            200 200 rcurveto
            0 -200 rlineto
            )",
            math::bezier::Bezier(false, {
                {{300, 200}, {300, 200}, {350, 300}},
                {{500, 400}, {450, 300}, {500, 400}},
                {{500, 200}, {500, 200}, {500, 200}},
            })
            );
    }
};

QTEST_GUILESS_MAIN(TestCase)
#include "test_ps_parser.moc"
