/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ps_interpreter.hpp"
#include "glaxnimate/math/math.hpp"

using namespace glaxnimate::ps;
using namespace glaxnimate;
using namespace Qt::StringLiterals;


QString ArgumentType::to_string() const
{
    if ( overloads.empty() )
        return u"any"_s;
    QStringList str;
    for ( auto t : overloads )
        str.push_back(Value::type_name(t));
    return str.join(u" or "_s);
}


class Interpreter::Private
{
public:
    enum MetaSection
    {
        Initial,
        Document,
        Page,
        Other
    };

    Lexer lexer{nullptr};
    ExecutionMemory memory;
    bool halted = false;
    Level level = Level::PS3;
    QString current_command;
    MetaSection meta_section = Initial;
    std::map<QString, QString> document_metadata;
    std::map<QString, QString> page_metadata;
    bool auto_level = false;
    int auto_level_status = 0;
    int partial_level = 0;

    void handle_meta(QStringView comment)
    {
        // skip %
        comment.slice(1);

        if ( comment.startsWith("Page:"_L1) )
        {
            meta_section = Page;
        }
        else if ( comment.startsWith("EndComments"_L1) )
        {
            if ( auto_level && auto_level_status != 2 )
                auto_level_status = -1;
            meta_section = Other;
        }
        else if ( comment.startsWith("EndPageSetup"_L1) )
        {
            meta_section = Other;
        }

        if ( meta_section == Other )
            return;

        auto colon = comment.indexOf(':');
        if ( colon != -1 )
        {
            auto name = comment.left(colon);
            auto value = comment.sliced(colon+1).trimmed();
            std::map<QString, QString>* meta = meta_section == Page ? &page_metadata : &document_metadata;
            (*meta)[name.toString()] = value.toString();
            if ( auto_level_status == 1 && name == "LanguageLevel"_L1 )
            {
                bool ok = false;
                int language_level = value.toInt(&ok);

                if ( !ok )
                {
                    auto_level_status = -1;
                    return;
                }

                partial_level |= (1 << language_level) - 1;

                this->level = Level(partial_level);
                auto_level_status = 2;
            }
        }
    }

    void handle_intro(QStringView comment)
    {
        if ( !auto_level )
            return;

        auto version = comment.indexOf("PS-Adobe-3.0"_L1);
        if ( version == -1 )
        {
            // doesn't follow document structuring specs
            auto_level_status = -1;
            return;
        }

        auto_level_status = 1;

        if ( comment.contains("EPSF-"_L1) )
            partial_level = LevelBits::Encapsulated;
        else
            partial_level = LevelBits::NotEncapsulated;
    }
};

Value Interpreter::procedure_value()
{
    ValueArray proc;
    bool finished = false;
    while ( !finished && !d->halted )
    {
        auto token = d->lexer.next_token();
        switch ( token.type )
        {
            case Token::Operator:
            {
                auto op = token.value.cast<String>().bytes();
                if ( op == "{" )
                {
                    proc.emplace_back(procedure_value());
                }
                else if ( op == "}" )
                {
                    finished = true;
                    break;
                }
                else
                {
                    proc.emplace_back(std::move(token.value));
                }
                break;
            }
            case Token::Literal:
                proc.emplace_back(std::move(token.value));
                break;
            case Token::Eof:
                finished = true;
                break;
            case Token::Unrecoverable:
                error(u"Unkown token (maybe unterminated string?)"_s, true);
                finished = true;
                break;
            case Token::Comment:
                break;
        }
    }

    Value procval = Value::from<Value::Array>(std::move(proc));
    procval.set_attribute(Value::Execute, true);
    return procval;
}

Interpreter::Interpreter() : d(std::make_unique<Private>())
{

}

Interpreter::~Interpreter() = default;

glaxnimate::ps::ExecutionMemory &glaxnimate::ps::Interpreter::memory()
{
    return d->memory;
}
glaxnimate::ps::Stack &glaxnimate::ps::Interpreter::stack()
{
    return d->memory.operand_stack;
}

void glaxnimate::ps::Interpreter::execute(QIODevice *device)
{
    d->lexer.set_device(device);
    while ( !d->halted )
    {
        auto token = d->lexer.next_token();
        switch ( token.type )
        {
            case Token::Operator:
            {
                auto op = token.value.cast<String>().bytes();
                if ( op == "{" )
                {
                    stack().push(procedure_value());
                }
                else
                {
                    execute_command(op);
                }
                break;
            }
            case Token::Literal:
                d->memory.operand_stack.push(token.value);
                break;
            case Token::Eof:
                return;
            case Token::Unrecoverable:
                error(u"Unkown token (maybe unterminated string?)"_s, true);
                break;
            case Token::Comment:
            {
                auto comment = token.value.to_string();
                if ( d->meta_section == Private::Initial )
                {
                    if ( comment.startsWith('!') )
                    {
                        d->handle_intro(comment);
                        d->meta_section = Private::Document;
                    }
                    else
                    {
                        d->meta_section = Private::Other;
                    }
                }
                if ( comment.startsWith('%') )
                    d->handle_meta(comment);
                on_comment(comment);
                if ( d->auto_level_status < 0 )
                    error(u"Could not determine language level"_s, false);
                break;
            }
        }
    }
}

void Interpreter::execute(const Value &proc)
{
    if ( !proc.has_attribute(Value::Execute) )
    {
        // error(u"Value is not executable"_s, false);
        stack().push(proc);
        return;
    }

    if ( proc.type() == Value::Array )
    {
        for ( const auto& v : proc.cast<ValueArray>() )
        {
            execute(v);
            if ( d->halted )
                break;
        }
    }
    else if ( proc.type() == Value::String )
    {
        execute_command(proc.cast<String>().bytes());
    }
    else
    {
        stack().push(proc);
    }

}

void glaxnimate::ps::Interpreter::print(const QString &text)
{
    on_print(text);
}

void glaxnimate::ps::Interpreter::error(const QString &error, bool critical)
{
    if ( critical )
        d->halted = true;
    on_error(error);
}

void glaxnimate::ps::Interpreter::execute_command(const QByteArray &name)
{
    auto cmd = command_from_name(name);
    if ( !cmd )
    {
        error(u"Unknown command '%1'"_s.arg(name), false);
    }
    else if ( !level_is_compatible(d->level, cmd->level) )
    {
        error(u"Command '%1' requires %2"_s.arg(name).arg(level_string(cmd->level)), true);
    }
    else
    {
        if ( int(cmd->args.size()) > d->memory.operand_stack.size() )
        {
            error(u"Command '%1' requires %2 arguments on the stack"_s.arg(name, cmd->args.size()), false);
        }

        ValueArray args;
        args.resize(cmd->args.size());
        for ( int i = cmd->args.size() - 1; i >= 0; i-- )
        {
            args[i] = d->memory.operand_stack.pop();
            const auto& expected_type = cmd->args[i];

            if ( !expected_type.matches(args[i].type()) )
            {
                error(u"Argument %4 command '%2' has an invalid value of %1 (Expected %3)"_s
                    .arg(args[i].to_pretty_string(), name, expected_type.to_string(), QString::number(i)), false);
                return;
            }
        }

        cmd->func(std::move(args), *this);
    }
}

Level Interpreter::level() const
{
    return d->level;
}

void Interpreter::set_level(Level level)
{
    d->level = level;
}

void Interpreter::set_level_autodetect(bool enable)
{
    d->auto_level = enable;
}

std::map<QString, QString> &Interpreter::document_metadata()
{
    return d->document_metadata;
}

std::map<QString, QString> &Interpreter::page_metadata()
{
    return d->page_metadata;
}

QString glaxnimate::ps::level_string(Level level)
{
    return u"%1 %2"_s.arg(level_is_encapsulated(level) ? "EPS" : "PS").arg(level_number(level));
}

static QString to_ugly_string(const Value& val)
{
    QString string = val.to_string();
    if ( string.isEmpty() && val.type() != Value::String )
        return u"-nostring-"_s;
    return string;
}


/*

Not encapsulated:

    banddevice
    exitserver
    initmatrix
    setshared
  - clear
    framedevice
    quit
    startjob
    cleardictstack
    grestoreall
    renderbands
    copypage
    initclip
    setglobal
    erasepage
    initgraphics
    setpagedevice

Level 2

    <<
    >>
    arct
    colorimage
    cshow
    currentblackgeneration
    currentcacheparams
    currentcmykcolor
    currentcolor
    currentcolorrendering
    currentcolorscreen
    currentcolorspace
    currentcolortransfer
    currentdevparams
    currentglobal
    currentgstate
    currenthalftone
    currentobjectformat
    currentoverprint
    currentpacking
    currentpagedevice
    currentshared
    currentstrokeadjust
    currentsystemparams
    currentundercolorremoval
    currentuserparams
    defineresource
    defineuserobject
    deletefile
    execform
    execuserobject
    filenameforall
    fileposition
    filter
    findencoding
    findresource
    gcheck
    globaldict
    GlobalFontDirectory
    glyphshow
    gstate
    ineofill
    infill
    instroke
    inueofill
    inufill
    inustroke
    ISOLatin1Encoding
    languagelevel
    makepattern
    packedarray
    printobject
    product
    realtime
    rectclip
    rectfill
    rectstroke
    renamefile
    resourceforall
    resourcestatus
    revision
    rootfont
    scheck
    selectfont
    serialnumber
    setbbox
    setblackgeneration
    setcachedevice2
    setcacheparams
    setcmykcolor
    setcolor
    setcolorrendering
    setcolorscreen
    setcolorspace
    setcolortranfer
    setdevparams
    setfileposition
    setglobal
    setgstate
    sethalftone
    setobjectformat
    setoverprint
    setpacking
    setpagedevice
    setpattern
    setshared
    setstrokeadjust
    setsystemparams
    setucacheparams
    setundercolorremoval
    setuserparams
    setvmthreshold
    shareddict
    SharedFontDirectory
    startjob
    uappend
    ucache
    ucachestatus
    ueofill
    ufill
    undef
    undefinefont
    undefineresource
    undefineuserobject
    upath
    UserObject
    ustroke
    ustrokepath
    vmreclaim
    writeobject
    xshow
    xyshow
    yshow


CYMK

    colorimage
    currentblackgeneration
    currentcmykcolor
    currentcolorscreen
    currentcolortransfer
    currentundercolorremoval
    setblackgeneration
    setcmykcolor
    setcolorscreen
    setcolortransfer
    setundercolorremoval

Composite Font

    cshow
    findencoding
    rootfont
    setcachedevice2

Filesystem

    deletefile
    filenameforall
    fileposition
    renamefile
    setfileposition

Version 25.0

    //
    currentcacheparams
    currentpacking
    packedarray
    setcacheparams
    setpacking

Misc

    ISOLatin1Encoding
    product
    realtime
    revision
    serialnumber

Level 3

    cliprestore
    findcolorrendering
    clipsave
    setsmoothness
    composefont
    shfill
    currentsmoothness

BitmapFontInit
    addglyph
    removeall
    removeglyphs

CIDInit
    beginbfchar
    endbfchar
    StartData
    beginbfrange
    endbfrange
    usecmap
    begincidchar
    endcidchar
    usefont
    begincidrange
    endcidrange
    begincmap
    endcmap
    begincodespacerange
    endcodespacerange
    beginnotdefchar
    endnotdefchar
    beginnotdefrange
    endnotdefrange
    beginrearrangedfont
    endrearrangedfont
    beginusematrix
    endusematrix
ColorRendering
    GetHalftoneName
    GetPageDeviceName
    GetSubstituteCRD
FontSetInit
    StartData
Trapping
    currenttrapparams
    settrapparams
    settrapzone
*/

using Arg = ArgumentType;
static std::unordered_map<QByteArray, glaxnimate::ps::Command> builtins = {
// Stack ops
    {"pop", {Level::EPS1, {Arg::any()}, [](ValueArray, Interpreter&){}}},
    {"exch", {Level::EPS1, {Arg::any(), Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(std::move(args[1]));
        interpreter.stack().push(std::move(args[0]));
    }}},
    {"dup", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        if ( interpreter.stack().empty() )
            interpreter.error(u"Empty stack"_s, false);
        else
            interpreter.stack().push(interpreter.stack().top());
    }}},
    {"copy", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        int count = args[0].cast<int>();
        if ( count ==  0 )
            return;
        if ( count < 0 )
        {
            interpreter.error(u"Negative index"_s, false);
            return;
        }
        if ( interpreter.stack().size() < count )
        {
            interpreter.error(u"Not enough elements on the stack"_s, false);
            return;
        }
        ValueArray copies;
        copies.reserve(count);
        for ( auto it = interpreter.stack().rend() - count; it != interpreter.stack().rend(); ++it )
            copies.emplace_back(*it);
        for ( auto& v : copies )
            interpreter.stack().push(std::move(v));
    }}},
    {"index", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        int index = args[0].cast<int>();
        if ( index < 0 )
        {
            interpreter.error(u"Negative index"_s, false);
            return;
        }
        if ( interpreter.stack().size() < index )
        {
            interpreter.error(u"Not enough elements on the stack"_s, false);
            return;
        }
        interpreter.stack().push(interpreter.stack()[index]);
    }}},
    {"roll", {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        int count = args[0].cast<int>();
        int roll = args[1].cast<int>();
        if ( count < 0 )
        {
            interpreter.error(u"Rolling by a negative amount"_s, false);
            return;
        }

        if ( count > interpreter.stack().size() )
        {
            interpreter.error(u"Not enough items in the stack"_s, false);
            return;
        }
        if ( std::abs(roll) >= count )
        {
            roll = roll % count;
        }

        auto begin = interpreter.stack().rend() - count;
        auto end = interpreter.stack().rend();
        auto middle = roll >= 0 ? end - roll : begin - roll;
        std::rotate(begin, middle, end);
    }}},
    {"clear", {Level::PS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().clear();
    }}},
    {"mark", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(Value::from<Value::Mark>());
    }}},
    {"<<", {Level::EPS2, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(Value::from<Value::Mark>());
    }}},
    {"cleartomark", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        while ( !interpreter.stack().empty() )
        {
            if ( interpreter.stack().pop().type() == Value::Mark )
                return;
        }

        interpreter.error(u"No mark found"_s, false);
    }}},
    {"counttomark", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        int count = 0;
        for ( const auto& v : interpreter.stack() )
        {
            if ( v.type() == Value::Mark )
            {
                interpreter.stack().push(count);
                return;
            }
            count++;
        }
        interpreter.error(u"No mark found"_s, false);
        interpreter.stack().push(count);
    }}},
// Math functions
    {"add", {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer && args[1].type() == args[0].type() )
            interpreter.stack().push(args[0].cast<int>() + args[1].cast<int>());
        else
            interpreter.stack().push(args[0].cast<float>() + args[1].cast<float>());
    }}},
    {"div", {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<float>();
        auto den = args[1].cast<float>();
        if ( den == 0 )
        {
            interpreter.error(u"Division by 0"_s, false);
            return;
        }

        interpreter.stack().push(num / den);
    }}},
    {"idiv", {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<int>();
        auto den = args[1].cast<int>();
        if ( den == 0 )
        {
            interpreter.error(u"Division by 0"_s, false);
            return;
        }

        interpreter.stack().push(num / den);
    }}},
    {"mod", {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<int>();
        auto den = args[1].cast<int>();
        if ( den == 0 )
        {
            interpreter.error(u"Division by 0"_s, false);
            return;
        }

        interpreter.stack().push(num % den);
    }}},
    {"mul", {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer && args[1].type() == args[0].type() )
            interpreter.stack().push(args[0].cast<int>() * args[1].cast<int>());
        else
            interpreter.stack().push(args[0].cast<float>() * args[1].cast<float>());
    }}},
    {"sub", {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer && args[1].type() == args[0].type() )
            interpreter.stack().push(args[0].cast<int>() - args[1].cast<int>());
        else
            interpreter.stack().push(args[0].cast<float>() - args[1].cast<float>());
    }}},
    {"abs", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(math::abs(args[0].cast<int>()));
        else
            interpreter.stack().push(math::abs(args[0].cast<float>()));
    }}},
    {"neg", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(-args[0].cast<int>());
        else
            interpreter.stack().push(-args[0].cast<float>());
    }}},
    {"ceiling", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(args[0]);
        else
            interpreter.stack().push(qCeil(args[0].cast<float>()));
    }}},
    {"round", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(args[0]);
        else
            interpreter.stack().push(qRound(args[0].cast<float>()));
    }}},
    {"floor", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(args[0]);
        else
            interpreter.stack().push(qFloor(args[0].cast<float>()));
    }}},
    {"truncate", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
        {
            interpreter.stack().push(args[0]);
        }
        else
        {
            auto val = args[0].cast<float>();
            interpreter.stack().push(val < 0 ? qCeil(val) : qFloor(val));
        }
    }}},
    {"sqrt", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(math::sqrt(args[0].cast<float>()));
    }}},
    {"atan", {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<float>();
        auto den = args[1].cast<float>();
        interpreter.stack().push(float(math::rad2deg(math::atan2(num, den))));
    }}},
    {"cos", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(float(math::cos(math::deg2rad(args[0].cast<float>()))));
    }}},
    {"sin", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(float(math::sin(math::deg2rad(args[0].cast<float>()))));
    }}},
    {"exp", {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        auto base = args[0].cast<float>();
        auto exp = args[1].cast<float>();
        interpreter.stack().push(std::pow(base, exp));
    }}},
    {"ln", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        auto val = args[0].cast<float>();
        interpreter.stack().push(std::log(val));
    }}},
    {"log", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        auto val = args[0].cast<float>();
        interpreter.stack().push(std::log10(val));
    }}},
    {"rand", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(int(interpreter.memory().prng()));
    }}},
    {"srand", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        interpreter.memory().prng.seed(args[0].cast<int>());
    }}},
    {"rrand", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        auto seed = interpreter.memory().prng();
        interpreter.memory().prng.seed(seed);
        interpreter.stack().push(int(seed));
    }}},
// Interactive
    {"=", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.print(to_ugly_string(args[0])+ '\n');
    }}},
    {"==", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.print(args[0].to_pretty_string() + '\n');
    }}},
    {"stack", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        for ( const auto& v : interpreter.stack() )
            interpreter.print(to_ugly_string(v) + '\n');
    }}},
    {"pstack", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        for ( const auto& v : interpreter.stack() )
            interpreter.print(v.to_pretty_string() + '\n');
    }}},
// Control
    {"exec", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.execute(args[0]);
    }}},
// Array
    {"[", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(Value::from<Value::Mark>());
    }}},
    {"]", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter) {
        bool mark_found = false;
        ValueArray arr;
        while ( !interpreter.stack().empty() )
        {
            auto item = interpreter.stack().pop();
            if ( item.type() == Value::Mark )
            {
                mark_found = true;
                break;
            }
            arr.emplace_back(std::move(item));
        }

        if ( !mark_found )
            interpreter.error(u"No mark found"_s, false);

        std::reverse(arr.begin(), arr.end());
        interpreter.stack().push(Value::from<Value::Array>(std::move(arr)));
    }}},
    {"array", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        int count = args[0].cast<int>();
        if ( count < 0 )
        {
            interpreter.error(u"Negative size"_s, false);
            return;
        }
        ValueArray arr;
        arr.resize(count);
        interpreter.stack().push(Value::from<Value::Array>(std::move(arr)));
    }}},
    {"length", {Level::EPS1, {Value::Array}, [](ValueArray args, Interpreter& interpreter){
        auto arr = args[0].cast<ValueArray>();
        interpreter.stack().push(arr.size());
    }}},
    {"get", {Level::EPS1, {Arg{Value::Array, Value::String}, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Array )
        {
            auto arr = args[0].cast<ValueArray>();
            int index = args[1].cast<int>();
            if ( index < 0 || index >= arr.size() )
            {
                interpreter.error(u"Index out of range"_s, false);
                return;
            }
            interpreter.stack().push(std::move(arr[index]));
        }
        else if ( args[0].type() == Value::String )
        {
            auto arr = args[0].cast<String>();
            int index = args[1].cast<int>();
            if ( index < 0 || index >= arr.size() )
            {
                interpreter.error(u"Index out of range"_s, false);
                return;
            }
            interpreter.stack().push(int(arr[index]));
        }
    }}},
    {"put", {Level::EPS1, {Arg{Value::Array, Value::String, Value::Dict}, Arg::any(), Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Array )
        {
            if ( args[1].type() != Value::Integer )
            {
                interpreter.error(u"Invalid argument"_s, false);
                return;
            }
            auto arr = args[0].cast<ValueArray>();
            int index = args[1].cast<int>();
            if ( index < 0 || index >= arr.size() )
            {
                interpreter.error(u"Index out of range"_s, false);
                return;
            }
            arr[index] = std::move(args[2]);
        }
        else if ( args[0].type() == Value::String )
        {
            if ( args[1].type() != Value::Integer || args[2].type() != Value::Integer )
            {
                interpreter.error(u"Invalid argument"_s, false);
                return;
            }
            auto arr = args[0].cast<String>();
            int index = args[1].cast<int>();
            if ( index < 0 || index >= arr.size() )
            {
                interpreter.error(u"Index out of range"_s, false);
                return;
            }
            arr[index] = args[2].cast<int>();
        }
        else
        {
            interpreter.error(u"Invalid argument"_s, false);
            return;
        }
    }}},
    {"getinterval", {Level::EPS1, {Arg{Value::Array, Value::String}, Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        int index = args[1].cast<int>();
        int count = args[2].cast<int>();

        if ( args[0].type() == glaxnimate::ps::Value::Array )
        {
            auto arr = args[0].cast<ValueArray>();
            if ( index < 0 || index >= arr.size() )
            {
                interpreter.error(u"Index out of range"_s, false);
                return;
            }
            if ( count < 0 || index + count >= arr.size() )
            {
                interpreter.error(u"Index out of range"_s, false);
                return;
            }

            ValueArray result;
            result.reserve(count);
            for ( int i = 0; i < count; i++ )
                result.emplace_back(std::move(arr[i + index]));
            interpreter.stack().push(std::move(result));
        }
        else if ( args[0].type() == glaxnimate::ps::Value::String )
        {
            auto arr = args[0].cast<String>();
            if ( index < 0 || index >= arr.size() )
            {
                interpreter.error(u"Index out of range"_s, false);
                return;
            }
            if ( count < 0 || index + count >= arr.size() )
            {
                interpreter.error(u"Index out of range"_s, false);
                return;
            }

            interpreter.stack().push(arr.bytes().sliced(index, count));
        }

    }}},
    {"putinterval", {Level::EPS1, {Arg{Value::Array, Value::String}, Value::Integer, Arg{Value::Array, Value::String}}, [](ValueArray args, Interpreter& interpreter){
        int index = args[1].cast<int>();
        if ( args[0].type() != args[2].type() )
        {
            interpreter.error(u"Invalid types"_s, false);
            return;
        }

        if ( args[0].type() == glaxnimate::ps::Value::Array )
        {
            auto arr = args[0].cast<ValueArray>();
            auto interval = args[2].cast<ValueArray>();
            if ( index < 0 || index >= arr.size() )
            {
                interpreter.error(u"Index out of range"_s, false);
                return;
            }
            if ( index + interval.size() >= arr.size() )
            {
                interpreter.error(u"Index out of range"_s, false);
                return;
            }

            for ( int i = 0; i < interval.size(); i++ )
                arr[index + i] = interval[i];
        }
        else if ( args[0].type() == glaxnimate::ps::Value::String )
        {
            auto arr = args[0].cast<String>();
            auto interval = args[2].cast<String>();
            if ( index < 0 || index >= arr.size() )
            {
                interpreter.error(u"Index out of range"_s, false);
                return;
            }
            if ( index + interval.size() >= arr.size() )
            {
                interpreter.error(u"Index out of range"_s, false);
                return;
            }

            for ( int i = 0; i < interval.size(); i++ )
                arr[index + i] = interval[i];
        }

    }}},
// String
    {"string", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        int count = args[0].cast<int>();
        if ( count < 0 )
        {
            interpreter.error(u"Negative size"_s, false);
            return;
        }
        interpreter.stack().push(String(QByteArray(count, 0)));
    }}},

};


Command* Interpreter::command_from_name(const QByteArray &name)
{
    auto it = builtins.find(name);
    if ( it == builtins.end() )
        return nullptr;
    return &it->second;
}
