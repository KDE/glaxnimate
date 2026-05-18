/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QApplication>
#include <QDialog>
#include <QSocketNotifier>

#ifdef WITH_READLINE
#   include <readline/readline.h>
#   include <readline/history.h>
#else
#   include <iostream>
#endif


#include "glaxnimate/module/module.hpp"
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

        if ( interp.is_halted() )
        {
            finalize();
            app->quit();
        }
    }

#ifdef WITH_READLINE
    static void finalize()
    {
        rl_callback_handler_remove();
    }

    static void static_on_line(char* line)
    {
        add_history(line);
        instance->on_line(line);
        free(line);
    }

    static void read_char()
    {
        rl_callback_read_char();
    }

    static void init(QApplication& app)
    {
        LineHandler::instance = new LineHandler(&app);

        rl_callback_handler_install("glps> ", &LineHandler::static_on_line);        
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
        fputs("glps> ", stdout);
        fflush(stdout);
    }

    static void init(QApplication& app)
    {
        LineHandler::instance = new LineHandler(&app);
        fputs("glps> ", stdout);
        fflush(stdout);
    }
#endif

};

LineHandler* LineHandler::instance;

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    glaxnimate::module::initialize();
    LineHandler::init(app);
    QSocketNotifier notifier(fileno(stdin), QSocketNotifier::Read);
    QObject::connect(&notifier, &QSocketNotifier::activated, &LineHandler::read_char);
    return app.exec();
}
