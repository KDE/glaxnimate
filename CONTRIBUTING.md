
Contributing To Glaxnimate
=======================================


Donating
---------------------------------------

If you don't have any technical skill you can still support Glaxnimate by
[donating](https://www.kde.org/community/donations/?app=glaxnimate&source=appdata).


Reporting Issues
---------------------------------------

You can report issues on [KDE's Bugtracker](https://bugs.kde.org/enter_bug.cgi?product=glaxnimate).

Bug reports and feature requests are welcome.

### Bugs

When reporting bugs please be as detailed as possible, include steps to reproduce
the issue and describe what happens.

If relevant, attach the file you were editing on the issue.

You can also add the system information gathered by Glaxnimate itself:
Go to *Help > About... > System* and click *Copy*,
then you can paste on the issue the system information.


Packaging
---------------------------------------

You can create packages to port Glaxnimate to a specific system,
setting up an automatable process to create said package so it can be
integrated with continuous integration.

Currently all the supported packages are built using KDE Craft.

### More packages

If you want to port Glaxnimate to a different system of package manager,
feel free to do so!

If the process can be automated the script can be added to continuous integration
so it gets built automatically.


Documentation
---------------------------------------

You can add to the documentation by adding tutorials, missing information,
correcting typos, etc...

On the [Documentation Website](https://docs.glaxnimate.org/) each page
has a link to its source file on GitLab, you can use that page to edit it and
create a pull request.

Details on how to work with the documentation are at [Documentation](https://glaxnimate.org/contributing/documentation/).


Translations
---------------------------------------

Translations are handled by [KDE Localization](https://l10n.kde.org/).

You can check out the translation file of a specific language with subversion:

```sh
TRANSLATION_LANGUAGE=it
svn co svn://anonsvn.kde.org/home/kde/trunk/l10n-kf6/$TRANSLATION_LANGUAGE/messages/glaxnimate
```

You can then use [Lokalize](https://apps.kde.org/lokalize/) to help with the translations.

Code
---------------------------------------

See the [README](https://invent.kde.org/graphics/glaxnimate/-/blob/master/README.md) for build instructions.

You can open [merge requests on GitLab](https://invent.kde.org/graphics/glaxnimate/-/merge_requests)
to get your changes merged into Glaxnimate.

### License

Glaxnimate is licensed under the [GNU GPLv3+](http://www.gnu.org/licenses/gpl-3.0.html),
so your contributions must be under the same license.


### How to find things

All the code is in one of the subdirectories within `./src`.

#### Core

Glaxnimate core is resposible for the object model, undo commands, file formats, etc.

* File format support: `src/core/glaxnimate/io`
* Undo commands: `src/core/glaxnimate/command`
* Object model: `src/core/glaxnimate/model/`
* Static properties for the object model: `src/core/glaxnimate/model/property`
* Animations for the object model: `src/core/glaxnimate/model/animation`

#### Core Modules

The core contains optional modules, that might be excluded from a build
for smaller package size or to avoid extra dependencies.

To define a new module called `MyModule` you need the following steps:

Register in the core CMakeLists.txt:

```cmake
glaxnimate_module("MyModule" "Description" ON/OFF)
```

Create at least the file `src/core/glaxnimate/module/mymodule/mymodule_module.hpp`.

With contents similar to these:

```c++
#include "glaxnimate/module/module.hpp"

namespace glaxnimate::mymodule {

class Module : public module::Module
{
public:
    Module() : module::Module(i18n("Description")) {}
    std::vector<module::ExternalComponent> components() const override;

protected:
    void initialize() override;
};

} // namespace glaxnimate::mymodule
```

`Module::initialize()` should register any class that needs registering (file formats, renderers, etc).
`Module::components()` should return info about any external libraries used by the module to display
them in the GUI.

And the cmake file for the module `src/core/glaxnimate/module/mymodule/CMakeLists.txt`:

```cmake
set(SOURCES
    # ...
)

add_library(GlaxnimateMyModule OBJECT ${SOURCES})
kde_target_enable_exceptions(GlaxnimateMyModule PUBLIC)
```

it's important namings are kept as in these examples as the modules are loaded automatically.


#### GUI

Glaxnimate gui contains the code for widgets and the app itself.

* Timeline: `src/gui/widgets/timeline`
* Adding settings:
    * `src/gui/glaxnimate_settings.kcfg` for the settings definitions
    * `src/gui/widgets/settings/settings_dialog.cpp` to add them to the settings dialog
* Menu/toolbar actions: `src/gui/glaxnimateui.rc`

### Style Guide

#### Indentation and Spacing

Use 4 spaces per indentation level.

Normally the opening brace goes on the next line from its declaration.
The main exceptions are namespaces and value initializiers.

There is no fixed line length, if something feels too long split it up.

#### Namespaces

Use `snake_case` names, avoid multiple word names if possible.

Keep the opening brace in the same line.

Add a comment on the line with the closing brace.

Use the compact syntax when defining an inner namespace.

Do not indent namespace members.

```c++
namespace my_namespace::inner_namespace {

class MyClass
{
    // ...
};

} // my_namespace::inner_namespace
```

#### Variables

Use `snake_case`.

Keep one variable declaration per line, with pointer/reference attached to the type.

For complex initializer, keep the opening on the same line but inner values
on indented lines.

Always initialize pointers with a proper value or `nullptr`.

Specify the `const` qualifier before the type.

Example:

```c++
const int number = 2;
int x = 0;
int y = 1;
int xy_sum = x + y;

// short enough, can go on a single line
QPointF position { x, y };
// too complex, split among line
std::vector<int> integer_items = {
    some_function(x, y, position),
    some_other_function(),
    67
};

QObject* selected_object = nullptr;
```

#### Functions

Use `snake_case`.

Braces for functions go into their own lines.
If a function is empty or very short, you can keep it all in one line.

If a function has a very long argument list, split the arguments one per line

Example:

```c++
void do_something(int my_argument)
{
    // do the thing
}

void do_nothing() {} // you can also split across line

void very_complex_function(
    std::map<std::string, int>::iterator iterator,
    QObject* owner = nullptr,
    int iterations = 123,
    SomeClass::SomeEnum type = SomeClass::SomeEnum::None
);
```

#### Classes and Other Types

For classes, enums, and enum values use `UpperCamelCase`,
for internal types that follow stl conventions use `snake_case`.

Braces go on their own lines.

Align access modifiers with the class.

Keep templates on one line, without additional indentation.

Example:

```c++
namespace my_namespace {

template<class T>
class MyClass
{
public:
    using value_type = T; // stl-like type

    enum Type
    {
        FirstType,
        SecondType
    };

    void do_something(int my_argument);
};

} // my_namespace
```

#### Class Members

Class members follow the same guidelines as regular functions and variables.

The exception is for class members of a Qt-like interface
where you should use `camelCase` instead. This exception must be used sparingly.

Getter should use the name of the thing they are getting without prefixes.
Setters use the `set_` prefix.

All overridden function must be marked `override`.

Use direct members on value classes, or classes that are simple and not used in a lot of places;
otherwise use a private pointer.

For classes with data members, add a trailing underscore if they clash with public functions.

Example:

```c++
class MySimpleClass
{
public:
    int foo() const { return foo_; }
    void set_foo(int foo) { foo_ = foo; }

private:
    int foo_; // underscore to avoid clashing with foo()
    int no_clash;
};

// This uses a private pointer
class MyComplexClass
{
public:
    MyComplexClass();
    ~MyComplexClass();

    int foo() const;
    void set_foo(int foo);

private:
    class Private;
    std::unique_ptr<Private> d;
};

// in the implementation file
class MyComplexClass::Private
{
    int foo;
};

MyComplexClass::MyComplexClass()
    : d(std::make_unique<Private>())
{}

// The destructor needs to be defined as Private is not known on declaration
MyComplexClass::~MyComplexClass() = default;

int MyComplexClass::foo() const 
{
    return d->foo;
}
```

#### Control Flow Statements

Add spaces around the brackets on control statement conditions.

You can avoid braces for single-line blocks but if-else chains need to all be
with or without brackets.

Example:

```c++
if ( a )
    foo(a);
else
    bar();


// else needs braces so we add it to if as well
if ( a )
{
    foo(a);
}
else
{
    int b = bar();
    b.foobar();
    foo(b);
}

for ( int i = 0; i < 10; i++ )
    foo(i);
```

#### Macros and Preprocessor Directives

Use `UPPER_SNAKE_CASE` for macros.

Do not indent before the `#` (but you can indent after it).

Avoid macros unless strictly necessary for code generation. 
If a macro has limited scope, undefine it once used.

For widely used macros add the `GLAXNIMATE_` prefix

Example:

```c++
#define GLAXNIMATE_REUSABLE_CODE_GENERATION(x)

#if Q_OS_HURD
#   define LOCAL_CODE_GENERATOR(x) // ...
#else
#   define LOCAL_CODE_GENERATOR(x) // ...
#endif

LOCAL_CODE_GENERATOR(int)
LOCAL_CODE_GENERATOR(double)

#undef LOCAL_CODE_GENERATOR
```

#### Files

All source files must start with a SPDX license block.

For headers use `#pragma once` to avoid multiple inclusion issues and wrap
everything in namespaces within the `glaxnimate` namespace.

Use `.cpp` extension for source files and `.hpp` for header files. 
File names use `snake_case`.

Try to keep includes in this order, separating each group with a blank line:
* c++ standard library
* Qt
* Other libraries
* Glaxnimate headers

Example:

```c++
/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <map>
#include <memory>

#include <QObject>
#include <QWidget>

#include "glaxnimate/model/object.hpp"

namespace glanimate::something {

// ...

} // namespace glanimate::something
```


Credits and Licensing
---------------------------------------

If you make significant contributions add your name or nickname in the appropriate section of AUTHORS.md,
you can also include your email but you don't have to.

If you contribute to the Glaxnimate, you agree that your contributions are
compatible with the licensing terms.

For documentation contributions the license is dual GPLv3+ and CC BY-SA 4.0.

For everything else in the Glaxnimate repository, the license is GPLv3+.

Some submodules have their own licensing terms.
