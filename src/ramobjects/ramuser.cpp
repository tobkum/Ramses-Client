#include "ramuser.h"

#include "data-models/ramitemtable.h"
#include "ramabstractitem.h"
#include "ramses.h"
#include "ramscheduleentry.h"
#include "usereditwidget.h"
#include "datacrypto.h"
#include "ramdatainterface/dbinterface.h"
#include "ramasset.h"
#include "ramshot.h"

// STATIC //

QMap<QString, RamUser*> RamUser::m_existingObjects = QMap<QString, RamUser*>();

RamUser *RamUser::get(QString uuid )
{
    if (!checkUuid(uuid, User)) return nullptr;

    if (m_existingObjects.contains(uuid)) return m_existingObjects.value(uuid);

    // Finally return a new instance
    return new RamUser(uuid);
}

RamUser *RamUser::c(RamObject *o)
{
    return qobject_cast<RamUser*>(o);
}

// PUBLIC //

RamUser::RamUser(QString shortName, QString name) :
    RamObject(shortName, name, User, nullptr, false, ENCRYPT_USER_DATA)
{
    construct();
    DBInterface::instance()->setUsername(m_uuid, shortName);
}

RamUser::RamUser(QString uuid):
    RamObject(uuid, User, nullptr, ENCRYPT_USER_DATA)
{
    construct();
}

void RamUser::setShortName(const QString &shortName)
{
    RamAbstractObject::setShortName(shortName);
    // Set username
    DBInterface::instance()->setUsername(m_uuid, shortName);
}

RamUser::UserRole RamUser::role() const
{
    QString roleStr = getData("role").toString("standard");
    if (roleStr == "admin") return Admin;
    if (roleStr == "project") return ProjectAdmin;
    if (roleStr == "lead") return Lead;
    return Standard;
}

void RamUser::setRole(const UserRole &role)
{
    switch(role)
    {
    case Admin:
        insertData("role", "admin");
        m_icon = ":/icons/admin";
        break;
    case ProjectAdmin:
        insertData("role", "project");
        m_icon = ":/icons/project-admin";
        break;
    case Lead:
        insertData("role", "lead");
        m_icon = ":/icons/lead";
        break;
    case Standard:
        insertData("role", "standard");
        m_icon = ":/icons/user";
        break;
    }
}

void RamUser::setRole(const QString role)
{
    insertData("role", role);
}

RamObjectList *RamUser::schedule() const
{
    return m_schedule;
}

bool RamUser::isStepAssigned(RamStep *step) const
{
    // Check in schedule
    for(int i = 0; i < m_schedule->rowCount(); i++)
    {
        RamScheduleEntry *entry = RamScheduleEntry::c( m_schedule->at(i) );
        if (!entry) continue;

        if (step->is(entry->step())) return true;
    }

    if (step->type() != RamStep::ShotProduction && step->type() != RamStep::AssetProduction) return false;

    // Check in status
    RamItemTable *items;
    RamStep::Type type = step->type();
    if (type == RamStep::ShotProduction) items = step->project()->shots();
    else items = step->project()->assets();

    for (int i =0; i < items->rowCount(); i++)
    {
        RamAbstractItem *item;
        if (type == RamStep::ShotProduction) item = RamShot::c( items->at(i) );
        else item = RamAsset::c( items->at(i) );
        RamStatus *status = item->status(step);
        if (!status) continue;
        if (this->is(status->assignedUser())) return true;
    }

    return false;
}

void RamUser::updatePassword(QString c, QString n)
{
    QString prev = getData("password").toString("");

    // Hash passwords
    if (c != "") c = DataCrypto::instance()->generatePassHash(c);
    n = DataCrypto::instance()->generatePassHash(n);

    if (c == prev) insertData("password", n);
}

QString RamUser::iconName() const
{
    switch(role())
    {
    case Admin:
        return ":/icons/admin";
    case ProjectAdmin:
        return ":/icons/project-admin";
    case Lead:
        return ":/icons/lead";
    case Standard:
        return ":/icons/user";
    }
}

QString RamUser::details() const
{
    switch(role())
    {
    case Admin:
        return "Administrator";
    case ProjectAdmin:
        return "Project Admin";
    case Lead:
        return "Lead";
    case Standard:
        return "Standard User";
    }
}

// PUBLIC SLOTS //

void RamUser::edit(bool show)
{
    if (!ui_editWidget) setEditWidget(new UserEditWidget(this));

    if (show) showEdit();
}

// PROTECTED //

QString RamUser::folderPath() const
{
    return Ramses::instance()->path(RamObject::UsersFolder) + "/" + shortName();
}

// PRIVATE //

void RamUser::construct()
{
    m_existingObjects[m_uuid] = this;
    m_icon = ":/icons/user";
    m_editRole = Admin;
    getCreateLists();
}

void RamUser::getCreateLists()
{
    QJsonObject d = data();

    QString uuid = d.value("schedule").toString();
    if (uuid == "") m_schedule = new RamObjectList("schedule", "Schedule", ScheduleEntry, RamObjectList::ListObject, this);
    else m_schedule = RamObjectList::get( uuid );
    m_schedule->setParent(this);
    d.insert("schedule", m_schedule->uuid());

    setData(d);
}
