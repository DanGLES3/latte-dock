/*
    SPDX-FileCopyrightText: 2021 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "actionshandler.h"

// local
#include "ui_actionsdialog.h"
#include "actionsdialog.h"
#include "../settingsdialog/tabpreferenceshandler.h"
#include "../../data/contextmenudata.h"

// KDE
#include <KLocalizedString>

namespace Latte {
namespace Settings {
namespace Handler {

ActionsHandler::ActionsHandler(Dialog::ActionsDialog *dialog)
    : Generic(dialog),
      m_dialog(dialog),
      m_ui(m_dialog->ui())
{
    init();
    loadItems(m_dialog->preferencesHandler()->contextMenuAlwaysActions());
}

ActionsHandler::~ActionsHandler()
{
}

void ActionsHandler::init()
{
    def_alwaysActions = table(Data::ContextMenu::ACTIONSALWAYSVISIBLE);

    QString itemid = Latte::Data::ContextMenu::LAYOUTSACTION;
    int itemindex = Latte::Data::ContextMenu::ACTIONSEDITORDER.indexOf(itemid);
    m_items[itemid] = new ActionsDialog::ActionListWidgetItem(QIcon::fromTheme("user-identity"),
                                                              i18n("Layouts"),
                                                              itemindex,
                                                              itemid);

    itemid = Latte::Data::ContextMenu::PREFERENCESACTION;
    itemindex = Latte::Data::ContextMenu::ACTIONSEDITORDER.indexOf(itemid);
    m_items[itemid] = new ActionsDialog::ActionListWidgetItem(QIcon::fromTheme("configure"),
                                                              i18nc("global settings window", "Configure Latte..."),
                                                              itemindex,
                                                              itemid);

    itemid = Latte::Data::ContextMenu::QUITLATTEACTION;
    itemindex = Latte::Data::ContextMenu::ACTIONSEDITORDER.indexOf(itemid);
    m_items[itemid] = new ActionsDialog::ActionListWidgetItem(QIcon::fromTheme("application-exit"),
                                                              i18nc("quit application", "Quit Latte"),
                                                              itemindex,
                                                              itemid);

    itemid = Latte::Data::ContextMenu::SEPARATOR1ACTION;
    itemindex = Latte::Data::ContextMenu::ACTIONSEDITORDER.indexOf(itemid);
    m_items[itemid] = new ActionsDialog::ActionListWidgetItem(QIcon::fromTheme(""),
                                                              i18n(" --- separator --- "),
                                                              itemindex,
                                                              itemid);

    itemid = Latte::Data::ContextMenu::ADDWIDGETSACTION;
    itemindex = Latte::Data::ContextMenu::ACTIONSEDITORDER.indexOf(itemid);
    m_items[itemid] = new ActionsDialog::ActionListWidgetItem(QIcon::fromTheme("list-add"),
                                                              i18n("Add Widgets..."),
                                                              itemindex,
                                                              itemid);

    itemid = Latte::Data::ContextMenu::ADDVIEWACTION;
    itemindex = Latte::Data::ContextMenu::ACTIONSEDITORDER.indexOf(itemid);
    m_items[itemid] = new ActionsDialog::ActionListWidgetItem(QIcon::fromTheme("list-add"),
                                                              i18n("Add Dock/Panel"),
                                                              itemindex,
                                                              itemid);

    itemid = Latte::Data::ContextMenu::MOVEVIEWACTION;
    itemindex = Latte::Data::ContextMenu::ACTIONSEDITORDER.indexOf(itemid);
    m_items[itemid] = new ActionsDialog::ActionListWidgetItem(QIcon::fromTheme("transform-move-horizontal"),
                                                              i18n("Move Dock/Panel To Layout"),
                                                              itemindex,
                                                              itemid);

    itemid = Latte::Data::ContextMenu::EXPORTVIEWTEMPLATEACTION;
    itemindex = Latte::Data::ContextMenu::ACTIONSEDITORDER.indexOf(itemid);
    m_items[itemid] = new ActionsDialog::ActionListWidgetItem(QIcon::fromTheme("document-export"),
                                                              i18n("Export Dock/Panel as Template..."),
                                                              itemindex,
                                                              itemid);

    itemid = Latte::Data::ContextMenu::REMOVEVIEWACTION;
    itemindex = Latte::Data::ContextMenu::ACTIONSEDITORDER.indexOf(itemid);
    m_items[itemid] = new ActionsDialog::ActionListWidgetItem(QIcon::fromTheme("delete"),
                                                              i18n("Remove Dock/Panel"),
                                                              itemindex,
                                                              itemid);
}

void ActionsHandler::loadItems(const QStringList &alwaysActions)
{
    for(int i=0; i<Latte::Data::ContextMenu::ACTIONSEDITORDER.count(); ++i) {
        QString actionname = Latte::Data::ContextMenu::ACTIONSEDITORDER[i];
        bool inalways = alwaysActions.contains(actionname);

        int rowinalways = rowInAlways(m_items[actionname]);
        int rowinedit = rowInEdit(m_items[actionname]);

        if (inalways && rowinalways == -1) {
            if (rowinedit >= 0) {
                m_ui->actionsSelector->availableListWidget()->takeItem(rowinedit);
            }
            m_ui->actionsSelector->selectedListWidget()->addItem(m_items[actionname]);
        } else if (!inalways && rowinedit == -1) {
            if (rowinalways >= 0) {
                m_ui->actionsSelector->selectedListWidget()->takeItem(rowinalways);
            }
            m_ui->actionsSelector->availableListWidget()->addItem(m_items[actionname]);
        }
    }

}

bool ActionsHandler::hasChangedData() const
{
    return c_alwaysActions != c_alwaysActions;
}

bool ActionsHandler::inDefaultValues() const
{
    return c_alwaysActions == def_alwaysActions;
}

int ActionsHandler::rowInAlways(const Settings::ActionsDialog::ActionListWidgetItem *item) const
{
    for(int i=0; i<m_ui->actionsSelector->selectedListWidget()->count(); ++i) {
        if (m_ui->actionsSelector->selectedListWidget()->item(i) == item) {
            return i;
        }
    }

    return -1;
}

int ActionsHandler::rowInEdit(const Settings::ActionsDialog::ActionListWidgetItem *item) const
{
    for(int i=0; i<m_ui->actionsSelector->availableListWidget()->count(); ++i) {
        if (m_ui->actionsSelector->availableListWidget()->item(i) == item) {
            return i;
        }
    }

    return -1;
}

Data::GenericTable<Data::Generic> ActionsHandler::table(const QStringList &ids)
{
    Data::GenericTable<Data::Generic> bastable;

    for(int i=0; i<ids.count(); ++i) {
        bastable << Data::Generic(ids[i], "");
    }

    return bastable;
}

QStringList ActionsHandler::currentData() const
{
    return c_alwaysActions.ids();
}

void ActionsHandler::reset()
{
    c_alwaysActions = o_alwaysActions;
}

void ActionsHandler::resetDefaults()
{
    c_alwaysActions = def_alwaysActions;
}

void ActionsHandler::save()
{
    //do nothing
}

}
}
}
