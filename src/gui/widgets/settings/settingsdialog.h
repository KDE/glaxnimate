#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <KConfigDialog>

class SettingsDialog : public KConfigDialog
{
    Q_OBJECT
public:
    SettingsDialog(QWidget *parent);
};

#endif // SETTINGSDIALOG_H
