/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ps_interpreter.hpp"

#include <QBuffer>

#include "glaxnimate/app_info.hpp"
#include "glaxnimate/math/math.hpp"
#include "ps_lexer.hpp"

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
    bool break_loop = false;
    int max_loop_iter = 1000;
    bool stopped = false;
    int exec_stack_size = 0;

    Level level = Level::PS3;
    QByteArray current_command;
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

Interpreter::Interpreter() : d(std::make_unique<Private>())
{
    d->memory.builtins = &CommandSet::builtins();
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

void glaxnimate::ps::Interpreter::execute(QIODevice *device, bool reset_pos)
{
    d->lexer.set_device(device, reset_pos);
    while ( !d->halted && !d->stopped )
    {
        auto token = d->lexer.next_token();
        switch ( token.type )
        {
            case Token::Literal:
                execute(std::move(token.value));
                if ( d->break_loop )
                    error(u"Invalid exit"_s);
                break;
            case Token::Eof:
                return;
            case Token::Unrecoverable:
                error(u"Unkown token (maybe unterminated string?)"_s);
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
                    on_warning(u"Could not determine language level"_s);
                break;
            }
        }
    }
}

void Interpreter::execute(Value proc)
{
    if ( !proc.has_attribute(Value::Executable) || proc.has_attribute(Value::Deferred) )
    {
        proc.set_attribute(Value::Deferred, false);
        stack().push(std::move(proc));
        return;
    }

    if ( proc.type() == Value::Array )
    {
        for ( const auto& v : proc.cast<ValueArray>() )
        {
            execute(v);
            if ( procedure_must_exit() )
                break;
        }
    }
    else if ( proc.type() == Value::String )
    {
        execute_command(proc);
    }
    else
    {
        stack().push(std::move(proc));
    }

}

void glaxnimate::ps::Interpreter::print(const QString &text)
{
    on_print(text);
}

void glaxnimate::ps::Interpreter::error(const QString &error)
{
    d->stopped = true;
    on_error(error);
}

void Interpreter::warning(const QString &error)
{
    on_warning(error);
}

bool Command::collect_arguments(Stack &stack, std::vector<std::pair<int, int>> &errors, ValueArray& args) const
{
    int count = arg_types.size();
    errors.clear();

    if ( stack.size() < count )
        return false;

    for ( int i = 0; i < count; i++ )
    {
        int stack_ind = count - 1 - i;
        if ( !arg_types[i].matches(stack[stack_ind]) )
        {
            errors.push_back({i, stack_ind});
        }
    }

    if ( !errors.empty() )
        return false;

    args.resize(count);
    for ( int i = count - 1; i >= 0; i-- )
    {
        args[i] = stack.pop();
    }
    return true;
}

void glaxnimate::ps::Interpreter::execute_command(const Value& nameval)
{
    Value cmdval;
    if ( d->memory.load(nameval.hash_key(), cmdval, false) )
    {
        if ( cmdval == nameval )
            error(u"Command executing itself"_s);
        else
            execute(cmdval);
        return;
    }

    QByteArray name = nameval.cast<String>().bytes();
    d->current_command = name;

    auto cmdrange = CommandSet::builtins().find(name);

    if ( cmdrange.first == cmdrange.second )
    {
        error(u"Unknown command '%1'"_s.arg(name));
        return;
    }


    ValueArray args;
    const Command* best = nullptr;
    std::vector<std::pair<int, int>> errors;

    for ( auto it = cmdrange.first; it != cmdrange.second; ++it )
    {
        const Command* cmd = &it->second;
        std::vector<std::pair<int, int>> cmderr;
        if ( cmd->collect_arguments(stack(), cmderr, args) )
        {
            cmd->func(std::move(args), *this);
            d->current_command.clear();
            return;
        }
        if ( !best || cmderr.size() < errors.size() )
        {
            errors = std::move(cmderr);
            best = cmd;
        }
    }

    if ( errors.empty() )
    {
        error(u"Not enough arguments on the stack for '%1'"_s.arg(name));
    }
    else
    {
        for ( auto p : errors )
        {
            error(u"Argument %1 of command '%2' has an invalid value of %3 (Expected %4)"_s.arg(
                QString::number(p.first),
                name,
                stack()[p.second].to_pretty_string(),
                best->arg_types[p.first].to_string()
            ));
        }
    }

    d->current_command.clear();
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

bool Interpreter::is_halted() const
{
    return d->halted;
}

void Interpreter::halt()
{
    d->halted = true;
}

bool Interpreter::is_stopped() const
{
    return d->stopped;
}

void Interpreter::set_stopped(bool stopped)
{
    d->stopped = stopped;
}

bool Interpreter::procedure_must_exit() const
{
    return d->halted || d->stopped || d->break_loop;
}

bool Interpreter::loop_must_exit(int count)
{
    if ( d->halted || d->stopped )
        return true;

    auto broken = d->break_loop;
    d->break_loop = false;
    if ( count > d->max_loop_iter )
    {
        error(u"Runaway loop"_s);
        return true;
    }
    return broken;
}

void Interpreter::loop_exit()
{
    d->break_loop = true;
}

int Interpreter::file_row() const
{
    return d->lexer.row();
}

int Interpreter::file_column() const
{
    return d->lexer.column();
}

const QByteArray &Interpreter::current_command()
{
    return d->current_command;
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

QDebug operator<<(QDebug d, Interpreter &interp)
{
    d << "Operand stack:\n";
    for ( const auto& v : interp.stack() )
    {
        d << "    " << v.to_pretty_string() << "\n";
    }

    if ( !interp.current_command().isEmpty() )
        d << "Last command: " << interp.current_command() << "\n";

    d << "At " << interp.file_row() << ":" << interp.file_column() << "\n";

    return d;
}

ValueDict &ExecutionMemory::loaded_systemdict()
{
    if ( systemdict.empty() )
    {
        builtins->populate_dict(systemdict);

        /*
         * NOTE:
         *  $error errordict not included because we don't want error handling on scripts
         *  statusdict omitted as it is not required by the specs
         */
        systemdict["userdict"] = userdict;
        systemdict["globaldict"] = globaldict;
        systemdict["systemdict"] = systemdict;

        systemdict["currentstrokeadjust"] = false;
    }

    return systemdict;
}

bool ExecutionMemory::load(const ValueDict::key_type &key, Value &out, bool search_system)
{
    for ( auto dit = dict_stack.rbegin(); dit != dict_stack.rend(); ++dit )
    {
        if ( dit->load_into(key, out) )
            return true;
    }

    if ( userdict.load_into(key, out) || globaldict.load_into(key, out) )
        return true;

    if ( search_system )
        return loaded_systemdict().load_into(key, out);

    return false;
}

bool ExecutionMemory::store(const ValueDict::key_type &key, const Value &val)
{

    for ( auto dit = dict_stack.rbegin(); dit != dict_stack.rend(); ++dit )
    {
        if ( dit->store(key, val) )
            return true;
    }

    if ( userdict.store(key, val) || globaldict.store(key, val) )
        return true;

    if ( loaded_systemdict().contains(key) )
        return false;

    (*current_dict())[key] = val;
    return true;
}

ValueDict *ExecutionMemory::where(const ValueDict::key_type &key)
{
    for ( auto dit = dict_stack.rbegin(); dit != dict_stack.rend(); ++dit )
    {
        if ( dit->contains(key) )
            return &*dit;
    }

    if ( userdict.contains(key) )
        return &userdict;

    if ( globaldict.contains(key) )
        return &globaldict;

    if ( loaded_systemdict().contains(key) )
        return &systemdict;

    return nullptr;
}

/*

Not encapsulated:

    banddevice
    exitserver
    initmatrix
    setshared
  - clear
    framedevice
  - quit
    startjob
  - cleardictstack
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

void CommandSet::def(QByteArray key, Command cmd)
{
    commands.emplace(std::move(key), std::move(cmd));
}

void CommandSet::alias(QByteArray key, QByteArray other)
{
    auto range = find(other);
    assert(range.first != range.second);
    for ( auto it = range.first; it != range.second; ++it )
        commands.emplace(key, it->second);
}

const CommandSet &CommandSet::builtins()
{
    static CommandSet builtins;
    if ( builtins.commands.empty() )
        populate_builtins(builtins);
    return builtins;
}

void CommandSet::populate_dict(ValueDict &value) const
{
    for ( const auto& p : commands )
    {
        Value key = p.first;
        Value val = p.first;
        val.set_attributes(Value::Executable|Value::Name|Value::Readable);
        value[p.first] = val;
    }
}

namespace {

template<class NumT>
void for_impl(const ValueArray& args, Interpreter& interpreter)
{
    auto counter = args[0].cast<NumT>();
    auto increment = args[1].cast<NumT>();
    auto limit = args[2].cast<NumT>();

    if ( increment < 0 )
    {
        for ( int i = 0; counter >= limit && !interpreter.loop_must_exit(i++); counter += increment )
        {
            interpreter.stack().push(counter);
            interpreter.execute(args[3]);
        }
    }
    else
    {
        for ( int i = 0; counter <= limit && !interpreter.loop_must_exit(i++); counter += increment )
        {
            interpreter.stack().push(counter);
            interpreter.execute(args[3]);
        }
    }
}

bool to_float_array(const ValueArray& vals, std::vector<float>& out, bool allow_negative)
{
    out.reserve(vals.size());
    for ( const auto& v : vals )
    {
        if ( v.type() == Value::Real || v.type() == Value::Integer )
        {
            auto d = v.cast<float>();
            if ( allow_negative || d >= 0 )
            {
                out.push_back(d);
                continue;
            }
        }

        return false;
    }

    return true;
}

void current_color_impl(ColorSpaceType type, Interpreter& interpreter)
{
    std::vector<float> comps;
    if ( interpreter.memory().gstate.color_space.type() == type )
        comps = interpreter.memory().gstate.color_space.components(interpreter.memory().gstate.color);
    else
        comps = ColorSpace(type).components(interpreter.memory().gstate.color);

    for ( float comp : comps )
        interpreter.stack().push(comp);
}

void set_color_impl(ColorSpaceType type, const ValueArray& args, Interpreter& interpreter)
{
    interpreter.memory().gstate.color_space = type;
    std::vector<float> comps;
    if ( !to_float_array(args, comps, false) )
    {
        interpreter.error(u"Invalid component"_s);
        return;
    }

    if ( !interpreter.memory().gstate.color_space.make_color(comps, &interpreter.memory().gstate.color) )
    {
        interpreter.error(u"Invalid color"_s);
    }
}

} // namespace

void CommandSet::populate_builtins(CommandSet& builtins)
{
    using Arg = ArgumentType;

// Stack ops
{
    builtins.def("pop", {Level::EPS1, {Arg::any()}, [](ValueArray, Interpreter&){}});
    builtins.def("exch", {Level::EPS1, {Arg::any(), Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(std::move(args[1]));
        interpreter.stack().push(std::move(args[0]));
    }});
    builtins.def("dup", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        if ( interpreter.stack().empty() )
            interpreter.error(u"Empty stack"_s);
        else
            interpreter.stack().push(interpreter.stack().top());
    }});
    builtins.def("copy", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        int count = args[0].cast<int>();
        if ( count ==  0 )
            return;
        if ( count < 0 )
        {
            interpreter.error(u"Negative index"_s);
            return;
        }
        if ( interpreter.stack().size() < count )
        {
            interpreter.error(u"Not enough elements on the stack"_s);
            return;
        }
        ValueArray copies;
        copies.reserve(count);
        for ( auto it = interpreter.stack().rend() - count; it != interpreter.stack().rend(); ++it )
            copies.emplace_back(*it);
        for ( auto& v : copies )
            interpreter.stack().push(std::move(v));
    }});
    builtins.def("index", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        int index = args[0].cast<int>();
        if ( index < 0 )
        {
            interpreter.error(u"Negative index"_s);
            return;
        }
        if ( interpreter.stack().size() < index )
        {
            interpreter.error(u"Not enough elements on the stack"_s);
            return;
        }
        interpreter.stack().push(interpreter.stack()[index]);
    }});
    builtins.def("roll", {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        int count = args[0].cast<int>();
        int roll = args[1].cast<int>();
        if ( count < 0 )
        {
            interpreter.error(u"Rolling by a negative amount"_s);
            return;
        }

        if ( count > interpreter.stack().size() )
        {
            interpreter.error(u"Not enough items in the stack"_s);
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
    }});
    builtins.def("clear", {Level::PS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().clear();
    }});
    builtins.def("mark", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(Value::from<Value::Mark>());
    }});
    builtins.def("cleartomark", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        while ( !interpreter.stack().empty() )
        {
            if ( interpreter.stack().pop().type() == Value::Mark )
                return;
        }

        interpreter.error(u"No mark found"_s);
    }});
    builtins.def("counttomark", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
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
        interpreter.error(u"No mark found"_s);
        interpreter.stack().push(count);
    }});
    builtins.def("count", {Level::PS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(interpreter.stack().size());
    }});
}
// Math functions
{
    builtins.def("add", {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer && args[1].type() == args[0].type() )
            interpreter.stack().push(args[0].cast<int>() + args[1].cast<int>());
        else
            interpreter.stack().push(args[0].cast<float>() + args[1].cast<float>());
    }});
    builtins.def("div", {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<float>();
        auto den = args[1].cast<float>();
        if ( den == 0 )
        {
            interpreter.error(u"Division by 0"_s);
            return;
        }

        interpreter.stack().push(num / den);
    }});
    builtins.def("idiv", {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<int>();
        auto den = args[1].cast<int>();
        if ( den == 0 )
        {
            interpreter.error(u"Division by 0"_s);
            return;
        }

        interpreter.stack().push(num / den);
    }});
    builtins.def("mod", {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<int>();
        auto den = args[1].cast<int>();
        if ( den == 0 )
        {
            interpreter.error(u"Division by 0"_s);
            return;
        }

        interpreter.stack().push(num % den);
    }});
    builtins.def("mul", {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer && args[1].type() == args[0].type() )
            interpreter.stack().push(args[0].cast<int>() * args[1].cast<int>());
        else
            interpreter.stack().push(args[0].cast<float>() * args[1].cast<float>());
    }});
    builtins.def("sub", {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer && args[1].type() == args[0].type() )
            interpreter.stack().push(args[0].cast<int>() - args[1].cast<int>());
        else
            interpreter.stack().push(args[0].cast<float>() - args[1].cast<float>());
    }});
    builtins.def("abs", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(math::abs(args[0].cast<int>()));
        else
            interpreter.stack().push(math::abs(args[0].cast<float>()));
    }});
    builtins.def("neg", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(-args[0].cast<int>());
        else
            interpreter.stack().push(-args[0].cast<float>());
    }});
    builtins.def("ceiling", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(args[0]);
        else
            interpreter.stack().push(qCeil(args[0].cast<float>()));
    }});
    builtins.def("round", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(args[0]);
        else
            interpreter.stack().push(qRound(args[0].cast<float>()));
    }});
    builtins.def("floor", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
            interpreter.stack().push(args[0]);
        else
            interpreter.stack().push(qFloor(args[0].cast<float>()));
    }});
    builtins.def("truncate", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer )
        {
            interpreter.stack().push(args[0]);
        }
        else
        {
            auto val = args[0].cast<float>();
            interpreter.stack().push(val < 0 ? qCeil(val) : qFloor(val));
        }
    }});
    builtins.def("sqrt", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(math::sqrt(args[0].cast<float>()));
    }});
    builtins.def("atan", {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<float>();
        auto den = args[1].cast<float>();
        interpreter.stack().push(float(math::rad2deg(math::atan2(num, den))));
    }});
    builtins.def("cos", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(float(math::cos(math::deg2rad(args[0].cast<float>()))));
    }});
    builtins.def("sin", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(float(math::sin(math::deg2rad(args[0].cast<float>()))));
    }});
    builtins.def("exp", {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        auto base = args[0].cast<float>();
        auto exp = args[1].cast<float>();
        interpreter.stack().push(std::pow(base, exp));
    }});
    builtins.def("ln", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        auto val = args[0].cast<float>();
        interpreter.stack().push(std::log(val));
    }});
    builtins.def("log", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        auto val = args[0].cast<float>();
        interpreter.stack().push(std::log10(val));
    }});
    builtins.def("rand", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(int(interpreter.memory().prng()));
    }});
    builtins.def("srand", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        interpreter.memory().prng.seed(args[0].cast<int>());
    }});
    builtins.def("rrand", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        auto seed = interpreter.memory().prng();
        interpreter.memory().prng.seed(seed);
        interpreter.stack().push(int(seed));
    }});
}
// Interactive
{
    builtins.def("=", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.print(to_ugly_string(args[0])+ '\n');
    }});
    builtins.def("==", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.print(args[0].to_pretty_string() + '\n');
    }});
    builtins.def("stack", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        for ( const auto& v : interpreter.stack() )
            interpreter.print(to_ugly_string(v) + '\n');
    }});
    builtins.def("pstack", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        for ( const auto& v : interpreter.stack() )
            interpreter.print(v.to_pretty_string() + '\n');
    }});
}
// Array
{
    builtins.alias("[", "mark");
    builtins.def("]", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter) {
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
            interpreter.error(u"No mark found"_s);

        std::reverse(arr.begin(), arr.end());
        interpreter.stack().push(Value(std::move(arr)));
    }});
    builtins.def("array", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        int count = args[0].cast<int>();
        if ( count < 0 )
        {
            interpreter.error(u"Negative size"_s);
            return;
        }
        ValueArray arr;
        arr.resize(count);
        interpreter.stack().push(Value(std::move(arr)));
    }});
    builtins.def("length", {Level::EPS1, {Value::Array}, [](ValueArray args, Interpreter& interpreter){
        auto arr = args[0].cast<ValueArray>();
        interpreter.stack().push(arr.size());
    }});
    builtins.def("get", {Level::EPS1, {Value::Array, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        auto arr = args[0].cast<ValueArray>();
        int index = args[1].cast<int>();
        if ( index < 0 || index >= arr.size() )
        {
            interpreter.error(u"Index out of range"_s);
            return;
        }
        interpreter.stack().push(std::move(arr[index]));
    }});
    builtins.def("put", {Level::EPS1, {Value::Array, Value::Integer, Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        auto arr = args[0].cast<ValueArray>();
        int index = args[1].cast<int>();
        if ( index < 0 || index >= arr.size() )
        {
            interpreter.error(u"Index out of range"_s);
            return;
        }
        arr[index] = std::move(args[2]);
    }});
    builtins.def("getinterval", {Level::EPS1, {Value::Array, Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        auto arr = args[0].cast<ValueArray>();
        int index = args[1].cast<int>();
        int count = args[2].cast<int>();
        if ( index < 0 || index >= arr.size() )
        {
            interpreter.error(u"Index out of range"_s);
            return;
        }
        if ( count < 0 || index + count >= arr.size() )
        {
            interpreter.error(u"Index out of range"_s);
            return;
        }

        ValueArray result;
        result.reserve(count);
        for ( int i = 0; i < count; i++ )
            result.emplace_back(std::move(arr[i + index]));
        interpreter.stack().push(std::move(result));
    }});
    builtins.def("putinterval", {Level::EPS1, {Value::Array, Value::Integer, Value::Array}, [](ValueArray args, Interpreter& interpreter){
        int index = args[1].cast<int>();
        auto arr = args[0].cast<ValueArray>();
        auto interval = args[2].cast<ValueArray>();
        if ( index < 0 || index >= arr.size() )
        {
            interpreter.error(u"Index out of range"_s);
            return;
        }
        if ( index + interval.size() >= arr.size() )
        {
            interpreter.error(u"Index out of range"_s);
            return;
        }

        for ( int i = 0; i < interval.size(); i++ )
            arr[index + i] = interval[i];
    }});
    builtins.def("astore", {Level::EPS1, {Value::Array}, [](ValueArray args, Interpreter& interpreter){
        auto arr = args[0].cast<ValueArray>();
        if ( interpreter.stack().size() < arr.size() )
        {
            interpreter.error(u"Not enough elements on the stack"_s);
            return;
        }

        for ( int i = 0; i < arr.size(); i++ )
        {
            arr[arr.size() - 1 - i] = interpreter.stack().pop();
        }
    }});
    builtins.def("aload", {Level::EPS1, {Value::Array}, [](ValueArray args, Interpreter& interpreter){
        auto arr = args[0].cast<ValueArray>();

        for ( int i = 0; i < arr.size(); i++ )
            interpreter.stack().push(arr[i]);

        interpreter.stack().push(std::move(arr));
    }});
    builtins.def("copy", {Level::EPS1, {Value::Array, Value::Array}, [](ValueArray args, Interpreter& interpreter){
        auto src = args[0].cast<ValueArray>();
        auto dest = args[1].cast<ValueArray>();
        if ( src.size() > dest.size() )
        {
            interpreter.error(u"Not enough elements on the destination array"_s);
            return;
        }

        for ( int i = 0; i < src.size(); i++ )
            dest[i] = src[i];

        /*
         * NOTE: The description of arr1 arr2 copy subarr2 is quite confusing
         * I'm returning the source array becayse it's what ghostscript seems to be doing
         */
        interpreter.stack().push(std::move(src));
    }});
    builtins.def("forall", {Level::EPS1, {Value::Array, Value::Array}, [](ValueArray args, Interpreter& interpreter){
        if ( !args[1].has_attribute(Value::Executable) )
            return interpreter.error(u"Not a procedure"_s);

        auto arr = args[0].cast<ValueArray>();

        for ( int i = 0; i < arr.size() && !interpreter.loop_must_exit(i); i++ )
        {
            interpreter.stack().push(std::move(arr[i]));
            interpreter.execute(args[1]);
        }
    }});
// Packed array
    builtins.alias("packedarray", "array");
    builtins.def("packedarray", {Level::EPS2, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        auto length = args[0].cast<int>();
        if ( length < 0 || length > interpreter.stack().size() )
        {
            interpreter.error(u"Invalid size"_s);
            return;
        }


        ValueArray arr;
        arr.resize(length);

        for ( int i = 0; i < arr.size(); i++ )
        {
            arr[arr.size() - 1 - i] = interpreter.stack().pop();
        }

        interpreter.stack().push(std::move(arr));
    }});
    builtins.def("setpacking", {Level::EPS2, {Value::Boolean}, [](ValueArray, Interpreter&){}});
    builtins.def("currentpacking", {Level::EPS2, {}, [](ValueArray, Interpreter& interp){ interp.stack().push(false); }});
}
// Dictionary
{
    builtins.def("dict", {Level::EPS2, {Value::Integer}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(ValueDict());
    }});
    builtins.alias("<<", "mark");
    builtins.def(">>", {Level::EPS2, {}, [](ValueArray, Interpreter& interpreter) {
        bool fail = false;
        ValueDict out;
        while ( !interpreter.stack().empty() )
        {
            auto value = interpreter.stack().pop();
            if ( value.type() == Value::Mark )
                break;

            if ( interpreter.stack().empty() )
            {
                fail = true;
                break;
            }

            auto key = interpreter.stack().pop();
            if ( value.type() == Value::Mark )
            {
                fail = true;
                break;
            }
            out[key.hash_key()] = std::move(value);

            if ( interpreter.stack().empty() )
                fail = true;
        }

        if ( fail )
            interpreter.error(u"Odd number of items in the dict"_s);

        interpreter.stack().push(std::move(out));
    }});
    builtins.def("length", {Level::EPS1, {Value::Dict}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(args[0].cast<ValueDict>().size());
    }});
    builtins.def("maxlength", {Level::EPS1, {Value::Dict}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(args[0].cast<ValueDict>().size());
    }});
    builtins.def("begin", {Level::EPS1, {Value::Dict}, [](ValueArray args, Interpreter& interpreter){
        interpreter.memory().dict_stack.push_back(args[0].cast<ValueDict>());
    }});
    builtins.def("end", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        if ( interpreter.memory().dict_stack.empty() )
        {
            interpreter.error(u"Empty dict stack"_s);
            return;
        }
        interpreter.memory().dict_stack.pop_back();
    }});
    builtins.def("def", {Level::EPS1, {Arg::any(), Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        (*interpreter.memory().current_dict()).put(args[0].hash_key(), args[1]);
    }});
    builtins.def("load", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        Value val;
        if ( !interpreter.memory().load(args[0].hash_key(), val, true) )
            interpreter.error(u"Value %s not found in the dict stack"_s.arg(args[0].to_pretty_string()));
        else
            interpreter.stack().push(std::move(val));
    }});
    builtins.def("store", {Level::EPS1, {Arg::any(), Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        if ( !interpreter.memory().store(args[0].hash_key(), args[1]) )
            interpreter.error(u"Could not store %1"_s.arg(args[0].to_pretty_string()));
    }});
    builtins.def("get", {Level::EPS1, {Value::Dict, Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        auto dict = args[0].cast<ValueDict>();
        auto it = dict.find(args[1].hash_key());
        if ( it == dict.end() )
        {
            interpreter.error(u"Key %1 not found"_s.arg(args[1].to_pretty_string()));
            return;
        }
        interpreter.stack().push(it->second);
    }});
    builtins.def("put", {Level::EPS1, {Value::Dict, Arg::any(), Arg::any()}, [](ValueArray args, Interpreter&){
        auto dict = args[0].cast<ValueDict>();
        dict[args[1].hash_key()] = std::move(args[2]);
    }});
    builtins.def("undef", {Level::EPS2, {Value::Dict, Arg::any()}, [](ValueArray args, Interpreter&){
        auto dict = args[0].cast<ValueDict>();
        dict.erase(args[1].hash_key());
    }});
    builtins.def("known", {Level::EPS1, {Value::Dict, Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        auto dict = args[0].cast<ValueDict>();
        interpreter.stack().push(dict.contains(args[1].hash_key()));
    }});
    builtins.def("where", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        auto dict = interpreter.memory().where(args[0].hash_key());
        if ( dict )
            interpreter.stack().push(*dict);
        interpreter.stack().push(bool(dict));
    }});
    builtins.def("copy", {Level::EPS1, {Value::Dict, Value::Dict}, [](ValueArray args, Interpreter& interpreter){
        auto src = args[0].cast<ValueDict>();
        auto dest = args[1].cast<ValueDict>();
        for ( auto& p : src )
            dest[p.first] = std::move(p.second);
        interpreter.stack().push(std::move(dest));
    }});
    builtins.def("forall", {Level::EPS1, {Value::Dict, Value::Array}, [](ValueArray args, Interpreter& interpreter){
        if ( !args[1].has_attribute(Value::Executable) )
            return interpreter.error(u"Not a procedure"_s);

        auto val = args[0].cast<ValueDict>();

        int i = 0;
        for ( auto& p : val )
        {
            if ( interpreter.loop_must_exit(i++) )
                break;
            interpreter.stack().push(p.first);
            interpreter.stack().push(p.second);
            interpreter.execute(args[1]);
        }
    }});
    builtins.def("dictstack", {Level::EPS1, {Value::Array}, [](ValueArray args, Interpreter& interpreter){
        auto dest = args[0].cast<ValueArray>();
        if ( int(interpreter.memory().dict_stack.size() + 3) > dest.size() )
        {
            interpreter.error(u"Not enough elements on the destination array"_s);
            return;
        }

        dest[0] = interpreter.memory().loaded_systemdict();
        dest[1] = interpreter.memory().globaldict;
        dest[2] = interpreter.memory().userdict;
        int i = 3;
        for ( const auto& dict : interpreter.memory().dict_stack )
            dest[i++] = dict;

        interpreter.stack().push(std::move(dest));
    }});
    builtins.def("countdictstack", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        // Include the 3 standard ones
        interpreter.stack().push(int(interpreter.memory().dict_stack.size() + 3));
    }});
    builtins.def("currentdict", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(*interpreter.memory().current_dict());
    }});
    builtins.def("cleardictstack", {Level::PS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.memory().dict_stack.clear();
    }});
}
// String
{
    builtins.def("string", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        int count = args[0].cast<int>();
        if ( count < 0 )
        {
            interpreter.error(u"Negative size"_s);
            return;
        }
        interpreter.stack().push(String(QByteArray(count, 0)));
    }});
    builtins.def("get", {Level::EPS1, {Value::String, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        auto arr = args[0].cast<String>();
        int index = args[1].cast<int>();
        if ( index < 0 || index >= arr.size() )
        {
            interpreter.error(u"Index out of range"_s);
            return;
        }
        interpreter.stack().push(int(arr[index]));
    }});
    builtins.def("put", {Level::EPS1, {Value::String, Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        auto arr = args[0].cast<String>();
        int index = args[1].cast<int>();
        if ( index < 0 || index >= arr.size() )
        {
            interpreter.error(u"Index out of range"_s);
            return;
        }
        arr[index] = args[2].cast<int>();
    }});
    builtins.def("getinterval", {Level::EPS1, {Value::String, Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        int index = args[1].cast<int>();
        int count = args[2].cast<int>();
        auto arr = args[0].cast<String>();
        if ( index < 0 || index >= arr.size() )
        {
            interpreter.error(u"Index out of range"_s);
            return;
        }
        if ( count < 0 || index + count >= arr.size() )
        {
            interpreter.error(u"Index out of range"_s);
            return;
        }

        interpreter.stack().push(arr.bytes().sliced(index, count));
    }});
    builtins.def("putinterval", {Level::EPS1, {Value::String, Value::Integer, Value::String}, [](ValueArray args, Interpreter& interpreter){
        auto arr = args[0].cast<String>();
        int index = args[1].cast<int>();
        auto interval = args[2].cast<String>();
        if ( index < 0 || index >= arr.size() )
        {
            interpreter.error(u"Index out of range"_s);
            return;
        }
        if ( index + interval.size() >= arr.size() )
        {
            interpreter.error(u"Index out of range"_s);
            return;
        }

        for ( int i = 0; i < interval.size(); i++ )
            arr[index + i] = interval[i];
    }});
    builtins.def("copy", {Level::EPS1, {Value::String, Value::String}, [](ValueArray args, Interpreter& interpreter){
        auto src = args[0].cast<String>();
        auto dest = args[1].cast<String>();
        if ( src.size() > dest.size() )
        {
            interpreter.error(u"Not enough elements on the destination string"_s);
            return;
        }

        for ( int i = 0; i < src.size(); i++ )
            dest[i] = src[i];

        interpreter.stack().push(std::move(src));
    }});
    builtins.def("forall", {Level::EPS1, {Value::String, Value::Array}, [](ValueArray args, Interpreter& interpreter){
        if ( !args[1].has_attribute(Value::Executable) )
            return interpreter.error(u"Not a procedure"_s);

        auto arr = args[0].cast<String>();

        for ( int i = 0; i < arr.size() && !interpreter.loop_must_exit(i); i++ )
        {
            interpreter.stack().push(int(arr[i]));
            interpreter.execute(args[1]);
        }
    }});
    builtins.def("anchorsearch", {Level::EPS1, {Value::String, Value::String}, [](ValueArray args, Interpreter& interpreter){
        auto string = args[0].cast<String>();
        auto seek = args[1].cast<String>();

        if ( string.starts_with(seek) )
        {
            interpreter.stack().push(string.bytes().sliced(seek.size()));
            interpreter.stack().push(seek);
            interpreter.stack().push(true);
        }
        else
        {
            interpreter.stack().push(string);
            interpreter.stack().push(false);
        }
    }});
    builtins.def("search", {Level::EPS1, {Value::String, Value::String}, [](ValueArray args, Interpreter& interpreter){
        auto string = args[0].cast<String>();
        auto seek = args[1].cast<String>();
        int pos = string.bytes().indexOf(seek.bytes());

        if ( pos != -1 )
        {
            interpreter.stack().push(string.bytes().sliced(pos + seek.size()));
            interpreter.stack().push(seek);
            interpreter.stack().push(string.bytes().sliced(0, pos));
            interpreter.stack().push(true);
        }
        else
        {
            interpreter.stack().push(string);
            interpreter.stack().push(false);
        }
    }});
    builtins.def("token", {Level::EPS1, {Value::String}, [](ValueArray args, Interpreter& interpreter){
        auto string = args[0].cast<String>();
        QBuffer buf(const_cast<QByteArray*>(&string.bytes()));
        buf.open(QIODeviceBase::ReadOnly);
        auto tok = Lexer(&buf).next_token_nocomment();
        if ( tok.type == Token::Literal )
        {
            interpreter.stack().push(buf.readAll());
            interpreter.stack().push(tok.value);
            interpreter.stack().push(true);
        }
        else
        {
            interpreter.stack().push(false);
        }
    }});
}
// Boolean
{
    builtins.def("true", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(true);
    }});
    builtins.def("false", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(false);
    }});


    builtins.def("eq", {Level::EPS1, {Arg::any(), Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(args[0].shallow_equal(args[1]));
    }});
    builtins.def("ne", {Level::EPS1, {Arg::any(), Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(!args[0].shallow_equal(args[1]));
    }});
#define CMPOP(name, op) \
    builtins.def(name, {Level::EPS1, {Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){ \
        if ( args[0].type() == Value::Integer && args[1].type() == args[0].type() ) \
            interpreter.stack().push(args[0].cast<int>() op args[1].cast<int>()); \
        else \
            interpreter.stack().push(args[0].cast<float>() op args[1].cast<float>()); \
    }}); \
    builtins.def(name, {Level::EPS1, {Value::String, Value::String}, [](ValueArray args, Interpreter& interpreter){ \
        interpreter.stack().push(args[0].cast<String>().bytes() op args[1].cast<String>().bytes()); \
    }});
    CMPOP("ge", >=)
    CMPOP("gt", >)
    CMPOP("le", <=)
    CMPOP("lt", <)
#undef CMPOP

    builtins.def("and", {Level::EPS1, {Value::Boolean, Value::Boolean}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(args[0].cast<bool>() && args[1].cast<bool>());
    }});
    builtins.def("or", {Level::EPS1, {Value::Boolean, Value::Boolean}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(args[0].cast<bool>() || args[1].cast<bool>());
    }});
    builtins.def("xor", {Level::EPS1, {Value::Boolean, Value::Boolean}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(bool(args[0].cast<bool>() ^ args[1].cast<bool>()));
    }});
    builtins.def("not", {Level::EPS1, {Value::Boolean}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(!args[0].cast<bool>());
    }});


    builtins.def("and", {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(args[0].cast<int>() & args[1].cast<int>());
    }});
    builtins.def("or", {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(args[0].cast<int>() | args[1].cast<int>());
    }});
    builtins.def("xor", {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(args[0].cast<int>() ^ args[1].cast<int>());
    }});
    builtins.def("not", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(~args[0].cast<int>());
    }});
    builtins.def("bitshift", {Level::EPS1, {Value::Integer, Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        auto num = args[0].cast<int>();
        auto shift = args[1].cast<int>();
        interpreter.stack().push(shift < 0 ? num >> -shift : num << shift);
    }});
}
// Control
{
    builtins.def("exec", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.execute(args[0]);
    }});
    builtins.def("if", {Level::EPS1, {Value::Boolean, Arg::proc()}, [](ValueArray args, Interpreter& interpreter){
        bool cond = args[0].cast<bool>();
        if ( cond )
            interpreter.execute(args[1]);
    }});
    builtins.def("ifelse", {Level::EPS1, {Value::Boolean, Arg::proc(), Arg::proc()}, [](ValueArray args, Interpreter& interpreter){
        bool cond = args[0].cast<bool>();
        if ( cond )
            interpreter.execute(args[1]);
        else
            interpreter.execute(args[2]);
    }});
    builtins.def("repeat", {Level::EPS1, {Value::Integer, Arg::proc()}, [](ValueArray args, Interpreter& interpreter){
        int count = args[0].cast<int>();
        if ( count < 0 )
        {
            interpreter.warning(u"Negative loop count"_s);
        }

        for ( int i = 0; i < count && !interpreter.loop_must_exit(i); i++ )
            interpreter.execute(args[1]);
    }});
    builtins.def("for", {Level::EPS1, {Arg::number(), Arg::number(), Arg::number(), Arg::proc()}, [](ValueArray args, Interpreter& interpreter){
        if ( args[0].type() == Value::Integer && args[1].type() == Value::Integer && args[2].type() == Value::Integer )
            for_impl<int>(args, interpreter);
        else
            for_impl<float>(args, interpreter);
    }});
    builtins.def("exit", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.loop_exit();
    }});
    builtins.def("loop", {Level::EPS1, {Arg::proc()}, [](ValueArray args, Interpreter& interpreter){
        for ( int i = 0; !interpreter.loop_must_exit(i); i++ )
            interpreter.execute(args[0]);
    }});
    builtins.def("stop", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.set_stopped(true);
    }});
    builtins.def("stopped", {Level::EPS1, {Arg::proc()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.execute(args[0]);
        interpreter.stack().push(interpreter.is_stopped());
        interpreter.set_stopped(false);
    }});
    builtins.def("start", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.set_stopped(true);
    }});
    builtins.def("quit", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.halt();
    }});
}
// Type etc
{
    builtins.def("type", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(args[0].value_type_name());
    }});
    builtins.def("cvlit", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        args[0].set_attribute(Value::Executable, false);
        interpreter.stack().push(std::move(args[0]));
    }});
    builtins.def("cvx", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        args[0].set_attribute(Value::Executable, true);
        interpreter.stack().push(std::move(args[0]));
    }});
    builtins.def("xcheck", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(args[0].has_attribute(Value::Executable));
    }});
    builtins.def("rcheck", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(args[0].has_attribute(Value::Readable));
    }});
    builtins.def("wcheck", {Level::EPS1, {Arg::any()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(args[0].has_attribute(Value::Writable));
    }});
    Arg mutable_type{Value::Array, Value::Dict, Value::File, Value::String};
    builtins.def("executeonly", {Level::EPS1, {mutable_type}, [](ValueArray args, Interpreter& interpreter){
        args[0].set_attributes(Value::Executable);
        interpreter.stack().push(std::move(args[0]));
    }});
    builtins.def("noaccess", {Level::EPS1, {mutable_type}, [](ValueArray args, Interpreter& interpreter){
        args[0].set_attributes(0);
        interpreter.stack().push(std::move(args[0]));
    }});
    builtins.def("readonly", {Level::EPS1, {mutable_type}, [](ValueArray args, Interpreter& interpreter){
        args[0].set_attribute(Value::Writable, false);
        interpreter.stack().push(std::move(args[0]));
    }});
    builtins.def("cvi", {Level::EPS1, {Arg{Value::String,Value::Real,Value::Integer}}, [](ValueArray args, Interpreter& interpreter){
        Value val = std::move(args[0]);
        if ( val.type() == Value::String )
        {
            auto string = val.cast<String>();
            QBuffer buf(const_cast<QByteArray*>(&string.bytes()));
            buf.open(QIODeviceBase::ReadOnly);
            auto tok = Lexer(&buf).next_token_nocomment();
            if ( tok.type != Token::Literal || (tok.value.type() != Value::Integer && tok.value.type() != Value::Real) )
            {
                interpreter.error(u"Invalid integer"_s);
                return;
            }
            val = std::move(tok.value);
        }

        if ( val.type() == Value::Integer )
            interpreter.stack().push(std::move(val));
        else
            interpreter.stack().push(int(val.cast<float>()));
    }});
    builtins.def("cvr", {Level::EPS1, {Arg{Value::String,Value::Real,Value::Integer}}, [](ValueArray args, Interpreter& interpreter){
        Value val = std::move(args[0]);
        if ( val.type() == Value::String )
        {
            auto string = val.cast<String>();
            QBuffer buf(const_cast<QByteArray*>(&string.bytes()));
            buf.open(QIODeviceBase::ReadOnly);
            auto tok = Lexer(&buf).next_token_nocomment();
            if ( tok.type != Token::Literal || (tok.value.type() != Value::Integer && tok.value.type() != Value::Real) )
            {
                interpreter.error(u"Invalid real"_s);
                return;
            }
            val = std::move(tok.value);
        }

        if ( val.type() == Value::Real )
            interpreter.stack().push(std::move(val));
        else
            interpreter.stack().push(float(val.cast<int>()));
    }});
    builtins.def("cvn", {Level::EPS1, {Value::String}, [](ValueArray args, Interpreter& interpreter){
        args[0].set_attribute(Value::Name, true);
        interpreter.stack().push(std::move(args[0]));
    }});
    builtins.def("cvs", {Level::EPS1, {Arg::any(), Value::String}, [](ValueArray args, Interpreter& interpreter){
        String dest = args[1].cast<String>();
        QByteArray src;
        switch ( args[0].type() )
        {
            case Value::Integer:
                src = QByteArray::number(args[0].cast<int>());
                break;
            case Value::Real:
                src = QByteArray::number(args[0].cast<float>());
                break;
            case Value::Boolean:
                src = args[0].cast<bool>() ? "true" : "false";
                break;
            case Value::String:
                src = args[0].cast<String>().bytes();
                break;
            case Value::Array:
            case Value::Dict:
            case Value::File:
            case Value::Mark:
            case Value::Null:
            case Value::Save:
                src = "--nostringval--";
                break;
        }

        if ( src.size() > dest.size() )
        {
            interpreter.error(u"Destination string too small"_s);
            return;
        }
        for ( int i = 0; i < src.size(); i++ )
            dest[i] = src[i];

        interpreter.stack().push(src);
    }});

    builtins.def("cvrs", {Level::EPS1, {Arg::number(), Value::Integer, Value::String}, [](ValueArray args, Interpreter& interpreter){
        int radix = args[1].cast<int>();
        if ( radix < 2 || radix > 36 )
        {
            interpreter.error(u"Invalid radix"_s);
            return;
        }

        String dest = args[2].cast<String>();
        QByteArray src;

        if ( radix == 10 )
        {
            if ( args[0].type() == Value::Integer )
                src = QByteArray::number(args[0].cast<int>());
            else
                src = QByteArray::number(args[0].cast<float>());
        }
        else
        {
            int val = args[0].cast<int>();
            src = QByteArray::number((unsigned int)(val), radix).toUpper();
        }

        if ( src.size() > dest.size() )
        {
            interpreter.error(u"Destination string too small"_s);
            return;
        }

        for ( int i = 0; i < src.size(); i++ )
            dest[i] = src[i];

        interpreter.stack().push(src);
    }});
}
// Misc
{
    builtins.def("version", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(AppInfo::instance().version().toLatin1());
    }});
    builtins.def("product", {Level::EPS2, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(AppInfo::instance().name().toUtf8());
    }});
    builtins.def("revision", {Level::EPS2, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(0);
    }});
    builtins.def("serialnumber", {Level::EPS2, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(0);
    }});
    builtins.def("languagelevel", {Level::EPS2, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(level_number(interpreter.level()));
    }});

    builtins.def("null", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(Value());
    }});

    builtins.def("bind", {Level::EPS1, {Arg::proc()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.stack().push(std::move(args[0]));
    }});

    builtins.def("realtime", {Level::EPS2, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(0);
    }});
    builtins.def("usertime", {Level::EPS2, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(0);
    }});
}
// Graphics
{
    builtins.def("gsave", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.memory().gstate_stack.push_back(interpreter.memory().gstate);
        interpreter.memory().gstate.clip_stack.clear();
    }});
    builtins.def("grestore", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        if ( !interpreter.memory().gstate_stack.empty() )
        {
            interpreter.memory().gstate = interpreter.memory().gstate_stack.back();
            interpreter.memory().gstate_stack.pop_back();
        }
    }});
    builtins.def("grestoreall", {Level::PS1, {}, [](ValueArray, Interpreter& interpreter){
        if ( !interpreter.memory().gstate_stack.empty() )
        {
            interpreter.memory().gstate = interpreter.memory().gstate_stack.front();
            interpreter.memory().gstate_stack.clear();
        }
    }});
    builtins.def("initgraphics", {Level::PS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.memory().gstate = {};
    }});
    builtins.def("clipsave", {Level::EPS3, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.memory().gstate.clip_stack.push_back(interpreter.memory().gstate.clip);
    }});
    builtins.def("cliprestore", {Level::EPS3, {}, [](ValueArray, Interpreter& interpreter){

        if ( !interpreter.memory().gstate.clip_stack.empty() )
        {
            interpreter.memory().gstate.clip = interpreter.memory().gstate.clip_stack.back();
            interpreter.memory().gstate.clip_stack.pop_back();
        }
    }});
    builtins.def("setlinewidth", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.memory().gstate.line_width = math::max(0.f, args[0].cast<float>());
    }});
    builtins.def("currentlinewidth", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(interpreter.memory().gstate.line_width);
    }});
    builtins.def("setlinecap", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        interpreter.memory().gstate.line_cap = GraphicsState::convert_cap(args[0].cast<int>());
    }});
    builtins.def("currentlinecap", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(GraphicsState::convert_cap(interpreter.memory().gstate.line_cap));
    }});
    builtins.def("setlinejoin", {Level::EPS1, {Value::Integer}, [](ValueArray args, Interpreter& interpreter){
        interpreter.memory().gstate.line_join = GraphicsState::convert_join(args[0].cast<int>());
    }});
    builtins.def("currentlinejoin", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(GraphicsState::convert_join(interpreter.memory().gstate.line_join));
    }});
    builtins.def("setmiterlimit", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.memory().gstate.miter_limit = args[0].cast<float>();
    }});
    builtins.def("currentmiterlimit", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(interpreter.memory().gstate.miter_limit);
    }});
    builtins.def("setstrokeadjust", {Level::EPS2, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        interpreter.memory().loaded_systemdict().put("currentstrokeadjust", args[0].cast<bool>());
    }});
    builtins.def("setdash", {Level::EPS1, {Value::Array, Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        std::vector<float> dashes;
        float offset = args[1].cast<float>();
        auto argdashes = args[0].cast<ValueArray>();
        if ( !to_float_array(argdashes, dashes, false) )
        {
            interpreter.error(u"Invalid dash length"_s);
            return;
        }

        interpreter.memory().gstate.dash_pattern = std::move(dashes);
        interpreter.memory().gstate.dash_offset = offset;

    }});
    builtins.def("currentsetdash", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        ValueArray dashes;
        dashes.reserve(interpreter.memory().gstate.dash_pattern.size());
        for ( auto d : interpreter.memory().gstate.dash_pattern )
            dashes.emplace_back(d);
        interpreter.stack().push(std::move(dashes));
        interpreter.stack().push(interpreter.memory().gstate.dash_offset);
    }});
    builtins.def("setcolorspace", {Level::EPS2, {Arg{Value::String, Value::Array}}, [](ValueArray args, Interpreter& interpreter){
        ValueArray params;
        if ( args[0].type() == Value::String )
            params.emplace_back(std::move(args[0]));
        else
            params = args[0].cast<ValueArray>();

        if ( params.empty() || params[0].type() != Value::String )
        {
            interpreter.error(u"Missing color space"_s);
            return;
        }

        auto space_name = params[0].cast<String>();
        if ( !interpreter.memory().gstate.color_space.from_name(space_name.bytes()) )
        {
            interpreter.error(u"Unknown color space %1"_s.arg(space_name.bytes()));
        }
    }});
    builtins.def("currentcolorspace", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        interpreter.stack().push(ValueArray{interpreter.memory().gstate.color_space.name()});
    }});
    builtins.def("setcolor", {Level::EPS2, {}, [](ValueArray, Interpreter& interpreter){
        int comp_count = interpreter.memory().gstate.color_space.component_count();
        if ( interpreter.stack().size() < comp_count )
        {
            interpreter.error(u"Not enough components"_s);
            return;
        }

        std::vector<float> comps;
        comps.resize(comp_count);
        for ( int i = comp_count - 1; i >= 0; --i )
        {
            auto v = interpreter.stack().pop();
            if ( v.type() != Value::Real && v.type() != Value::Integer )
            {
                interpreter.error(u"Invalid component type"_s);
                return;
            }
            comps[i] = v.cast<float>();
        }

        if ( !interpreter.memory().gstate.color_space.make_color(comps, &interpreter.memory().gstate.color) )
            interpreter.error(u"Invalid color"_s);
    }});
    builtins.def("currentcolor", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        current_color_impl(interpreter.memory().gstate.color_space.type(), interpreter);
    }});
    builtins.def("setgray", {Level::EPS1, {Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        set_color_impl(ColorSpaceType::DeviceGrey, args, interpreter);
    }});
    builtins.def("currentgray", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        current_color_impl(ColorSpaceType::DeviceGrey, interpreter);
    }});
    builtins.def("setrgbcolor", {Level::EPS1, {Arg::number(), Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        set_color_impl(ColorSpaceType::DeviceRGB, args, interpreter);
    }});
    builtins.def("currentrgbcolor", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        current_color_impl(ColorSpaceType::DeviceRGB, interpreter);
    }});
    builtins.def("setcmykcolor", {Level::EPS1, {Arg::number(), Arg::number(), Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        set_color_impl(ColorSpaceType::DeviceCYMK, args, interpreter);
    }});
    builtins.def("currentcmykcolor", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        current_color_impl(ColorSpaceType::DeviceRGB, interpreter);
    }});
    builtins.def("sethsbcolor", {Level::EPS1, {Arg::number(), Arg::number(), Arg::number()}, [](ValueArray args, Interpreter& interpreter){
        set_color_impl(ColorSpaceType::CustomHSV, args, interpreter);
        interpreter.memory().gstate.color_space = ColorSpaceType::DeviceRGB;
    }});
    builtins.def("currenthsbcolor", {Level::EPS1, {}, [](ValueArray, Interpreter& interpreter){
        current_color_impl(ColorSpaceType::CustomHSV, interpreter);
    }});
}

    /*
        Unimplemented:
            countexecstack
            execstack
            gstate / setgstate / currentgstate
            file / resource operators

        Not compliant
            access control changes don't apply to the shared value for dicts
            access control not checked for put/get etc
            bind is a noop

        Not defined (allowed by the specs):
            executive / echo / prompt
    */
}
