/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <QApplication>
#include <QDialog>
#include <QSocketNotifier>

#ifdef WITH_READLINE
#   include <readline/readline.h>
#   include <readline/history.h>
#endif

#include "glaxnimate/app_info.hpp"
#include "glaxnimate/utils/data_paths.hpp"
#include "glaxnimate/module/module.hpp"
#include "glaxnimate/module/postscript/ps_lexer.hpp"
#include "glaxnimate/module/postscript/ps_loader.hpp"

class Interpreter : public glaxnimate::ps::PsToGlaxnimate
{
public:
    Interpreter(glaxnimate::model::Document* doc, QWidget* page_widget)
        : glaxnimate::ps::PsToGlaxnimate(doc), page_widget(page_widget)
    {}

    QWidget* page_widget;

    void exec_string(QByteArray& line)
    {
        QBuffer buf(&line);
        buf.open(QIODeviceBase::ReadOnly);
        execute(&buf);
    }

    void define(const QByteArrayView& name, glaxnimate::ps::Value value)
    {
        memory().systemdict.put(name.toByteArray(), std::move(value));
    }

protected:
    void on_print(const QString &text) override
    {
        fputs(text.toStdString().c_str(), stdout);
        fflush(stdout);
    }

    void on_error(const QString &text) override
    {
        printf("Error %s\n", text.toStdString().c_str());
    }

    void on_warning(const QString &text) override
    {
        printf("Warning %s\n", text.toStdString().c_str());
    }

    void on_comp_finished(glaxnimate::model::Composition* comp) override
    {
        update_page(comp);
        page_widget->show();
    }

private:
    void update_page(glaxnimate::model::Composition* comp)
    {
        QImage img = comp->render_image();
        page_widget->resize(img.size());
        QPainter p(page_widget);
        p.fillRect(page_widget->rect(), page_widget->palette().base());
        p.drawImage(0, 0, img);
        p.end();
    }
};


class LineHandler
{
public:
    static LineHandler* instance;
    QApplication* app;
    QDialog dialog;
    glaxnimate::model::Document doc{""};
    Interpreter interp{&doc, &dialog};
    std::string prompt = "glps> ";

    LineHandler(QApplication* app)
        : app(app)
    {
    }

    void on_line(const char* line)
    {
        if ( !line )
        {
            finalize();
            app->quit();
            return;
        }

        QByteArray input(line);
        interp.exec_string(input);
        prompt = "glps";
        if ( interp.stack().size() > 0 )
            prompt += "<" + std::to_string(interp.stack().size());
        prompt += "> ";
        set_prompt();

        if ( interp.is_halted() )
        {
            finalize();
            app->quit();
        }
    }

#ifdef WITH_READLINE
    static void finalize()
    {
        QFileInfo(history_file()).dir().mkpath(QStringLiteral("."));
        std::string fname = history_file().toStdString();
        write_history(fname.c_str());
        rl_callback_handler_remove();
    }

    static void static_on_line(char* line)
    {
        instance->on_line(line);
        if ( line )
        {
            add_history(line);
            free(line);
        }
    }

    static void read_char()
    {
        rl_callback_read_char();
    }

    static QString history_file()
    {
        return glaxnimate::utils::writable_data_path(QStringLiteral("glps_history"));
    }

    static void init()
    {
        read_history(history_file().toStdString().c_str());
        rl_callback_handler_install(instance->prompt.c_str(), &LineHandler::static_on_line);
    }

    void set_prompt()
    {
        rl_set_prompt(prompt.c_str());
    }

#else
    static void finalize()
    {
    }

    static void read_char()
    {
        std::string line;
        std::getline(std::cin, line);

        if ( std::cin.eof() )
        {
            instance->on_line(nullptr);
            return;
        }

        instance->on_line(line.c_str());
        fputs(instance->prompt.c_str(), stdout);
        fflush(stdout);
    }

    static void init()
    {
        fputs(instance->prompt.c_str(), stdout);
        fflush(stdout);
    }
#endif
};

LineHandler* LineHandler::instance;

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(glaxnimate::AppInfo::instance().organization());
    app.setApplicationName(QStringLiteral("glps"));
    glaxnimate::module::initialize();
    LineHandler::instance = new LineHandler(&app);

    for ( int i = 1; i < argc; i++ )
    {
        QByteArray args = argv[i];
        QByteArrayView arg = args;

        if ( arg.startsWith("-D") || arg.startsWith("-d") || arg.startsWith("-S") || arg.startsWith("-s") )
        {
            char mode = std::tolower(arg[1]);
            auto split = arg.indexOf('=');

            if ( split == -1 )
            {
                LineHandler::instance->interp.define(arg.mid(2), {});
            }
            else
            {
                QByteArrayView name = arg.mid(2, split);
                QByteArray val = arg.mid(split+1).toByteArray();

                // string
                if ( mode == 's' )
                {
                    LineHandler::instance->interp.define(name, val);
                }
                // token
                else if ( mode == 'd' )
                {
                    QBuffer buf(&val);
                    buf.open(QIODeviceBase::ReadOnly);
                    auto tok = glaxnimate::ps::Lexer(&buf).next_token_nocomment();
                    if ( tok.type == glaxnimate::ps::Token::Literal )
                    {
                        LineHandler::instance->interp.define(name, tok.value);
                    }
                    else
                    {
                        LineHandler::instance->interp.define(name, {});
                    }
                }
            }
        }
        else if ( arg == "--help" || arg == "-h" )
        {
            std::cout << "glps PostScript interpreter\n\n";
            std::cout << "Synopsis:\n\tglps [options] [files] ...\n\n";
            std::cout << "Options:\n\n";
            std::cout << "-Dname=token\n-dname=token\n\tDefines \"name\" in systemdict with the value of the given token.";
            std::cout << " Equivalent to\n\t/name token def\n\n";
            std::cout << "-Dname\n-dname\n\tDefines \"name\" in systemdict with the null ass value.";
            std::cout << " Equivalent to\n\t/name null def\n\n";
            std::cout << "-Sname=string\n-sname=string\n\tDefines \"name\" in systemdict with a string value.";
            std::cout << " Equivalent to\n\t/name (string) def\n\n";
            return 0;

        }
        else if ( !arg.startsWith("-") )
        {
            QFile infile(args);
            if ( infile.open(QFile::ReadOnly) )
                LineHandler::instance->interp.execute_subfile(&infile);
            else
                LineHandler::instance->interp.error(QStringLiteral("Invalid file: %1").arg(arg));
        }
    }

    LineHandler::init();
    QSocketNotifier notifier(fileno(stdin), QSocketNotifier::Read);
    QObject::connect(&notifier, &QSocketNotifier::activated, &LineHandler::read_char);
    return app.exec();
}
