#include "aligndock.h"
#include "kactioncollection.h"
#include "qactiongroup.h"
#include "qcombobox.h"
#include "widgets/scalable_button.hpp"

#include <QGridLayout>

//#include "ui_logs.h"

using namespace glaxnimate::gui;

class AlignDock::Private
{
public:
   // ::Ui::dock_logs ui;
    static QToolButton* action_button(QAction* action, QWidget* parent)
    {
        auto button = new ScalableButton(parent);
        button->setDefaultAction(action);
        button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        button->resize(16, 16);
        button->setMaximumSize(64, 64);
        return button;
    }

    static void action_combo(QComboBox* box, QAction* action)
    {
        int index = box->count();
        box->addItem(action->icon(), KLocalizedString::removeAcceleratorMarker(action->text()), QVariant::fromValue(action));
        QObject::connect(action, &QAction::triggered, box, [index, box]{
            box->setCurrentIndex(index);
        });
        QObject::connect(action, &QAction::changed, box, [index, box, action]{
            box->setItemIcon(index, action->icon());
            box->setItemText(index, KLocalizedString::removeAcceleratorMarker(action->text()));
        });
    }
};

AlignDock::AlignDock(GlaxnimateWindow *parent)
    : QDockWidget(i18n("Align"), parent)
    , d(std::make_unique<Private>())
{
    //d->ui.setupUi(this);
    setWindowIcon(QIcon::fromTheme(QStringLiteral("dialog-align-and-distribute")));
    setObjectName(QStringLiteral("dock_align"));

    QWidget *mainWidget = new QWidget(this);

    QGridLayout *align_grid = new QGridLayout();
    mainWidget->setLayout(align_grid);

    setWidget(mainWidget);

    auto combo_align_to = new QComboBox(widget());
    combo_align_to->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    align_grid->addWidget(combo_align_to, 0, 0, 1, 3);

    QAction *align_to_selection = parent->action(QStringLiteral("align_to_selection"));
    AlignDock::Private::action_combo(combo_align_to, align_to_selection);
    QAction *align_to_canvas = parent->action(QStringLiteral("align_to_canvas"));
    AlignDock::Private::action_combo(combo_align_to, align_to_canvas);
    QAction *align_to_canvas_group = parent->action(QStringLiteral("align_to_canvas_group"));
    AlignDock::Private::action_combo(combo_align_to, align_to_canvas_group);

    QActionGroup *align_relative = new QActionGroup(parent);
    align_relative->setExclusive(true);
    align_to_canvas->setActionGroup(align_relative);
    align_to_selection->setActionGroup(align_relative);
    align_to_canvas_group->setActionGroup(align_relative);

    connect(combo_align_to, qOverload<int>(&QComboBox::currentIndexChanged), parent, [combo_align_to](int i){
        combo_align_to->itemData(i).value<QAction*>()->setChecked(true);
    });


    int row = 1;
    QAction *align_hor_left = parent->action(QStringLiteral("align_hor_left"));
    align_grid->addWidget(AlignDock::Private::action_button(align_hor_left, widget()),         row, 0);
    QAction *align_hor_center = parent->action(QStringLiteral("align_hor_center"));
    align_grid->addWidget(AlignDock::Private::action_button(align_hor_center, widget()),       row, 1);
    QAction *align_hor_right = parent->action(QStringLiteral("align_hor_right"));
    align_grid->addWidget(AlignDock::Private::action_button(align_hor_right, widget()),        row, 2);
    row++;
    QAction *hor_left_out = parent->action(QStringLiteral("align_hor_left_out"));
    align_grid->addWidget(AlignDock::Private::action_button(hor_left_out, widget()),     row, 0);
    QAction *hor_right_out = parent->action(QStringLiteral("align_hor_right_out"));
    align_grid->addWidget(AlignDock::Private::action_button(hor_right_out, widget()),    row, 2);
    row++;
    QAction *align_vert_top = parent->action(QStringLiteral("align_vert_top"));
    align_grid->addWidget(AlignDock::Private::action_button(align_vert_top, widget()),         row, 0);
    QAction *align_vert_center = parent->action(QStringLiteral("align_vert_center"));
    align_grid->addWidget(AlignDock::Private::action_button(align_vert_center, widget()),      row, 1);
    QAction *align_vert_bottom = parent->action(QStringLiteral("align_vert_bottom"));
    align_grid->addWidget(AlignDock::Private::action_button(align_vert_bottom, widget()),      row, 2);
    row++;
    QAction *vert_top_out = parent->action(QStringLiteral("align_vert_top_out"));
    align_grid->addWidget(AlignDock::Private::action_button(vert_top_out, widget()),     row, 0);
    QAction *vert_bottom_out = parent->action(QStringLiteral("align_vert_bottom_out"));
    align_grid->addWidget(AlignDock::Private::action_button(vert_bottom_out, widget()),  row, 2);
    row++;
    align_grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding),        row, 0);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

AlignDock::~AlignDock() = default;

