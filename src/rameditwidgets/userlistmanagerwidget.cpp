#include "userlistmanagerwidget.h"

#include "ramses.h"

UserListManagerWidget::UserListManagerWidget(QWidget *parent) :
    ObjectListManagerWidget<RamUser*, int>(
        Ramses::instance()->users(),
        "Users",
        QIcon(":icons/user"),
        parent )
{
    m_listEditWidget->setEditMode(ObjectListEditWidget<RamUser*, int>::RemoveObjects);
    QStringList dontRemove;
    dontRemove << "Ramses" << "Removed";
    m_listEditWidget->setDontRemoveShortNameList(dontRemove);
}

RamUser *UserListManagerWidget::createObject()
{
    RamUser *user = new RamUser("NEW","J-Doe");

    Ramses::instance()->users()->append(user);
    user->edit();
    return user;
}

