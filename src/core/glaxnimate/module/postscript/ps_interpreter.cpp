/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ps_interpreter.hpp"
#include "glaxnimate/math/math.hpp"

using namespace glaxnimate::ps;
using namespace glaxnimate;

Value Interpreter::procedure_value()
{
    ValueArray proc;
    bool finished = false;
    while ( !finished && !halted )
    {
        auto token = lexer.next_token();
        switch ( token.type )
        {
            case Token::Operator:
                if ( token.value.value() == u"{"_s )
                {
                    proc.push_back(procedure_value());
                }
                else if ( token.value.value() == u"}"_s )
                {
                    finished = true;
                    break;
                }
                else
                {
                    proc.emplace_back(std::move(token.value));
                }
                break;
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

void glaxnimate::ps::Interpreter::execute(QIODevice *device)
{
    lexer.set_device(device);
    while ( !halted )
    {
        auto token = lexer.next_token();
        switch ( token.type )
        {
            case Token::Operator:
                if ( token.value.value() == u"{"_s )
                {
                    stack().push(procedure_value());
                }
                else
                {
                    execute(token.value);
                }
                break;
            case Token::Literal:
                memory_.operand_stack.push(token.value);
                break;
            case Token::Eof:
                return;
            case Token::Unrecoverable:
                error(u"Unkown token (maybe unterminated string?)"_s, true);
                break;
            case Token::Comment:
                on_comment(token.value.to_string());
                break;
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
            if ( halted )
                break;
        }
    }
    else if ( proc.type() == Value::String )
    {
        execute_command(proc.to_string());
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
        halted = true;
    on_error(error);
}

void glaxnimate::ps::Interpreter::execute_command(const QString &name)
{
    auto cmd = command_from_name(name);
    if ( !cmd )
    {
        error(u"Unknown command '%1'"_s.arg(name), false);
    }
    else if ( !level_is_compatible(level_, cmd->level) )
    {
        error(u"Command '%1' requires %2"_s.arg(name).arg(level_string(cmd->level)), true);
    }
    else
    {
        if ( int(cmd->args.size()) > memory_.operand_stack.size() )
        {
            error(u"Command '%1' requires %2 arguments on the stack"_s.arg(name, cmd->args.size()), false);
        }

        ValueArray args;
        args.resize(cmd->args.size());
        for ( int i = cmd->args.size() - 1; i >= 0; i-- )
        {
            args[i] = memory_.operand_stack.pop();
            auto expected_type = cmd->args[cmd->args.size() - 1 - i];
            if ( expected_type != Value::Null && !args[i].can_convert(expected_type) )
            {
                error(u"Wrong value %1 passed to command '%2'"_s.arg(args[i].to_string(), name), false);
                return;
            }
        }

        cmd->func(std::move(args), *this);
    }
}

Level Interpreter::level() const
{
    return level_;
}

QString glaxnimate::ps::level_string(Level level)
{
    if ( level_is_encapsulated(level) )
        return level_string(Level(int(level) & int(Level::LevelMask))) + u" EPSF-%1.0"_s.arg(level_number(level));
    return u"PS-Adobe-%1.0"_s.arg(level_number(level));
}

static QString to_ugly_string(const Value& val)
{
    QString string = val.to_string();
    if ( string.isEmpty() && val.type() != Value::String )
        return u"-nostring-"_s;
    return string;
}

static std::unordered_map<QString, glaxnimate::ps::Command> builtins = {
// Stack ops
    {u"pop"_s, {Level::EPS1, {Value::Null}, [](ValueArray, Interpreter&){}}},
    {u"exch"_s, {Level::EPS1, {Value::Null, Value::Null}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(std::move(args[1]));
        interpreter.stack().push(std::move(args[0]));
    }}},
    {u"dup"_s, {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        if ( interpreter.stack().empty() )
            interpreter.error(u"Empty stack"_s, false);
        else
            interpreter.stack().push(interpreter.stack().top());
    }}},
    {u"copy"_s, {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
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
            copies.push_back(*it);
        for ( auto& v : copies )
            interpreter.stack().push(std::move(v));
    }}},
    {u"index"_s, {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
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
    {u"roll"_s, {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
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
    {u"clear"_s, {Level::PS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().clear();
    }}},
    {u"mark"_s, {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(Value::from<Value::Mark>());
    }}},
    {u"["_s, {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(Value::from<Value::Mark>());
    }}},
    {u"<<"_s, {Level::EPS2, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(Value::from<Value::Mark>());
    }}},
    {u"cleartomark"_s, {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        while ( !interpreter.stack().empty() )
        {
            if ( interpreter.stack().pop().type() == Value::Mark )
                return;
        }

        interpreter.error(u"No mark found"_s, false);
    }}},
    {u"counttomark"_s, {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
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
    {u"add"_s, {Level::EPS1, {Value::Real, Value::Real}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer && args[1].type() == args[0].type() )
            interpreter.stack().push(args[0].cast<int>() + args[1].cast<int>());
        else
            interpreter.stack().push(args[0].cast<float>() + args[1].cast<float>());
    }}},
    {u"div"_s, {Level::EPS1, {Value::Real, Value::Real}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<float>();
        auto den = args[1].cast<float>();
        if ( den == 0 )
        {
            interpreter.error(u"Division by 0"_s, false);
            return;
        }

        interpreter.stack().push(num / den);
    }}},
    {u"idiv"_s, {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<int>();
        auto den = args[1].cast<int>();
        if ( den == 0 )
        {
            interpreter.error(u"Division by 0"_s, false);
            return;
        }

        interpreter.stack().push(num / den);
    }}},
    {u"mod"_s, {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<int>();
        auto den = args[1].cast<int>();
        if ( den == 0 )
        {
            interpreter.error(u"Division by 0"_s, false);
            return;
        }

        interpreter.stack().push(num % den);
    }}},
    {u"mul"_s, {Level::EPS1, {Value::Real, Value::Real}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer && args[1].type() == args[0].type() )
            interpreter.stack().push(args[0].cast<int>() * args[1].cast<int>());
        else
            interpreter.stack().push(args[0].cast<float>() * args[1].cast<float>());
    }}},
    {u"sub"_s, {Level::EPS1, {Value::Real, Value::Real}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer && args[1].type() == args[0].type() )
            interpreter.stack().push(args[0].cast<int>() - args[1].cast<int>());
        else
            interpreter.stack().push(args[0].cast<float>() - args[1].cast<float>());
    }}},
    {u"abs"_s, {Level::EPS1, {Value::Real}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(math::abs(args[0].cast<int>()));
        else
            interpreter.stack().push(math::abs(args[0].cast<float>()));
    }}},
    {u"neg"_s, {Level::EPS1, {Value::Real}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(-args[0].cast<int>());
        else
            interpreter.stack().push(-args[0].cast<float>());
    }}},
    {u"ceiling"_s, {Level::EPS1, {Value::Real}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(args[0]);
        else
            interpreter.stack().push(qCeil(args[0].cast<float>()));
    }}},
    {u"round"_s, {Level::EPS1, {Value::Real}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(args[0]);
        else
            interpreter.stack().push(qRound(args[0].cast<float>()));
    }}},
    {u"floor"_s, {Level::EPS1, {Value::Real}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(args[0]);
        else
            interpreter.stack().push(qFloor(args[0].cast<float>()));
    }}},
    {u"truncate"_s, {Level::EPS1, {Value::Real}, [](ValueArray args, Interpreter& interpreter){
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
    {u"sqrt"_s, {Level::EPS1, {Value::Real}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(math::sqrt(args[0].cast<float>()));
    }}},
    {u"atan"_s, {Level::EPS1, {Value::Real, Value::Real}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<float>();
        auto den = args[1].cast<float>();
        interpreter.stack().push(float(math::rad2deg(math::atan2(num, den))));
    }}},
    {u"cos"_s, {Level::EPS1, {Value::Real}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(float(math::cos(math::deg2rad(args[0].cast<float>()))));
    }}},
    {u"sin"_s, {Level::EPS1, {Value::Real}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(float(math::sin(math::deg2rad(args[0].cast<float>()))));
    }}},
    {u"exp"_s, {Level::EPS1, {Value::Real, Value::Real}, [](ValueArray args, Interpreter& interpreter){
        auto base = args[0].cast<float>();
        auto exp = args[1].cast<float>();
        interpreter.stack().push(std::pow(base, exp));
    }}},
    {u"ln"_s, {Level::EPS1, {Value::Real}, [](ValueArray args, Interpreter& interpreter){
        auto val = args[0].cast<float>();
        interpreter.stack().push(std::log(val));
    }}},
    {u"log"_s, {Level::EPS1, {Value::Real}, [](ValueArray args, Interpreter& interpreter){
        auto val = args[0].cast<float>();
        interpreter.stack().push(std::log10(val));
    }}},
    {u"rand"_s, {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(int(interpreter.memory().prng()));
    }}},
    {u"srand"_s, {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        interpreter.memory().prng.seed(args[0].cast<int>());
    }}},
    {u"rrand"_s, {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        auto seed = interpreter.memory().prng();
        interpreter.memory().prng.seed(seed);
        interpreter.stack().push(int(seed));
    }}},
// Interactive
    {u"="_s, {Level::EPS1, {Value::Null}, [](ValueArray args, Interpreter& interpreter){
        interpreter.print(to_ugly_string(args[0])+ '\n');
    }}},
    {u"=="_s, {Level::EPS1, {Value::Null}, [](ValueArray args, Interpreter& interpreter){
        interpreter.print(args[0].to_pretty_string() + '\n');
    }}},
    {u"stack"_s, {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        for ( const auto& v : interpreter.stack() )
            interpreter.print(to_ugly_string(v) + '\n');
    }}},
    {u"pstack"_s, {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        for ( const auto& v : interpreter.stack() )
            interpreter.print(v.to_pretty_string() + '\n');
    }}},
// Control
    {u"exec"_s, {Level::EPS1, {Value::Null}, [](ValueArray args, Interpreter& interpreter){
        interpreter.execute(args[0]);
    }}},
};


Command* Interpreter::command_from_name(const QString &name)
{
    auto it = builtins.find(name);
    if ( it == builtins.end() )
        return nullptr;
    return &it->second;
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
