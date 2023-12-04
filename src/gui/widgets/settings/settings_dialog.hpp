#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <KConfigDialog>

namespace glaxnimate::gui {

class SettingsDialog : public KConfigDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget *parent);

private:
    // class AutoConfigPage;
};

} // namespace glaxnimate::gui

#endif // SETTINGSDIALOG_H
