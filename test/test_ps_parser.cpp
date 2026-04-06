/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QTest>
#include <QBuffer>

#include "glaxnimate/module/postscript/ps_lexer.hpp"
#include "glaxnimate/module/postscript/ps_interpreter.hpp"

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
    QString last_comment;

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

    void on_comment(const QString &text) override
    {
        last_comment = text;
    }
};

template<class T> Value stack_val(T arg) { return Value(arg); }

template<class... T>
std::vector<Value> stack_vals(T... args)
{
    return {stack_val(args)...};
}

QVariantList array_to_vals(const ValueArray& arr)
{
    QVariantList vals;
    vals.reserve(arr.size());
    for ( const auto& v : arr )
        vals.push_back(v.value());
    return vals;
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

class TestCase: public QObject
{
    Q_OBJECT

    Token lex(const char* text)
    {
        return StringLexer(text).next_token();
    }

private Q_SLOTS:
    void initTestCase()
    {
        qRegisterMetaType<Value>();
        qRegisterMetaType<ValueArray>();

        /*QMetaType::registerConverter<glaxnimate::ps::ValueArray, QString>(
                [](const glaxnimate::ps::ValueArray& arr) {
                return arr.to_pretty_string();
            }
        );*/
    }

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
        COMPARE_TOKEN(lex("10z"), Token::Operator, Value::String, "10z");
        COMPARE_TOKEN(lex("++10"), Token::Operator, Value::String, "++10");
    }

    void test_lex_radix()
    {
        StringLexer lexer("8#1777 16#fffe 2#1000 36#Glax");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 01777);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 0xfffe);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 0b1000);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 774105);

        COMPARE_TOKEN(lex("2#102"), Token::Operator, Value::String, "2#102");
        COMPARE_TOKEN(lex("2#10~"), Token::Operator, Value::String, "2#10~");
    }

    void test_lex_float()
    {
        StringLexer lexer("-0.25 34.5 -23.6e4 1.0E-5 1E6 -1. .5");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Real, -0.25);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Real, 34.5);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Real, -23.6e4);

        COMPARE_TOKEN(lex("0.2.3"), Token::Operator, Value::String, "0.2.3");
        COMPARE_TOKEN(lex("0.45f"), Token::Operator, Value::String, "0.45f");
        COMPARE_TOKEN(lex("e6"), Token::Operator, Value::String, "e6");
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
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, "<<");
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, ">>");
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
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, "abc");
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, "Offset");
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, "$$");
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, "23A");
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, "13-456");
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, "a.b");
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, "$MyDict");
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, "@pattern");
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
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, "[");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 123);
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "abc");
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, "]");

        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, "<<");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::String, "1");
        COMPARE_TOKEN(lexer.next_token(), Token::Literal, Value::Integer, 2);
        COMPARE_TOKEN(lexer.next_token(), Token::Operator, Value::String, ">>");
    }

    void test_parse_vals()
    {
        COMPARE_PARSE("1 /123 2", 1, "123", 2);
    }

    void test_parse_stack_ops()
    {
        COMPARE_PARSE("2 3 /4 clear");
        COMPARE_PARSE("1 2 3 dup", 1, 2, 3, 3);
        COMPARE_PARSE("1 2 3 exch", 1, 3, 2);
        COMPARE_PARSE("1 2 3 pop", 1, 2);
        COMPARE_PARSE("1 2 3 4 5 6  2 copy", 1, 2, 3, 4, 5, 6, 5, 6);
        COMPARE_PARSE("10 20 30 40 1 index", 10, 20, 30, 40, 30);
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
        QCOMPARE(Value::from<Value::Mark>().to_string(), "");
        QCOMPARE(Value::from<Value::Null>().to_string(), "");
        QCOMPARE(Value::from<Value::Array>({1, 2, "abc"}).to_string(), "");

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
        QCOMPARE(interp.stack_values(), stack_vals(1, 2, 3, Value(), 4, 5, 6));
        QCOMPARE(interp.stack()[3].type(), Value::Mark);

        interp.exec_string("counttomark");
        QCOMPARE(interp.stack_values(), stack_vals(1, 2, 3, Value(), 4, 5, 6, 3));

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
        QCOMPARE(interp.stack()[2].value(), 1);
        QCOMPARE(interp.stack()[1].value(), 2);
        QCOMPARE(interp.stack()[0].type(), Value::Array);
        QCOMPARE(interp.stack()[0].attributes(), Value::Executable);
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
        using MetaMap = std::map<QString, QString>;
        MetaMap docs = {
            {u"Creator"_s, u"Creator Value"_s},
            {u"CreationDate"_s, u"Sat Apr  4 08:10:50 2026"_s},
            {u"Pages"_s, u"1"_s},
            {u"DocumentData"_s, u"Clean7Bit"_s},
            {u"LanguageLevel"_s, u"2"_s},
            {u"BoundingBox"_s, u"112 199 212 279"_s},
        };
        QCOMPARE(interp.document_metadata(), docs);
        MetaMap page = {
            {u"Page"_s, u"1 1"_s},
            {u"PageBoundingBox"_s, u"112 199 212 279"_s},
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
};

QTEST_GUILESS_MAIN(TestCase)
#include "test_ps_parser.moc"

