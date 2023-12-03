#include "settingsdialog.h"

#include "glaxnimatesettings.h"
#include "ui_settings_open_save.h"

SettingsDialog::SettingsDialog(QWidget *parent)
: KConfigDialog(parent, QStringLiteral("settings"), GlaxnimateSettings::self())
{
    QWidget *p1 = new QWidget;
    Ui_SettingsOpenSave page_open_save;
    page_open_save.setupUi(p1);
    addPage(p1, tr("Open / Save"), QStringLiteral("kfloppy"));

}
