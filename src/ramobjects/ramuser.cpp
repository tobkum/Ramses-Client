#include "ramuser.h"

#include "data-models/ramitemtable.h"
#include "ramitem.h"
#include "ramses.h"
#include "ramscheduleentry.h"
#include "usereditwidget.h"
#include "datacrypto.h"
#include "ramdatainterface/dbinterface.h"

// STATIC //

RamUser *RamUser::getObject(QString uuid, bool constructNew )
{
    RamObject *obj = RamObject::getObject(uuid);
    if (!obj && constructNew) return new RamUser( uuid );
    return qobject_cast<RamUser*>( obj );
}

RamUser *RamUser::c(RamObject *o)
{
    return qobject_cast<RamUser*>(o);
}

// PUBLIC //

RamUser::RamUser(QString shortName, QString name) :
    RamObject(shortName, name, User)
{
    construct();

    m_schedule = new RamObjectList(shortName + "-schdl", name + " | Schedule", this);
    insertData("schedule", m_schedule->uuid());
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
    if (step->type() == RamStep::ShotProduction) items = step->project()->shots();
    else items = step->project()->assets();

    for (int i =0; i < items->rowCount(); i++)
    {
        RamItem *item = RamItem::c( items->at(i) );
        RamStatus *status = item->status(step);
        if (!status) continue;
        if (this->is(status->assignedUser())) return true;
    }

    return false;
}

void RamUser::updatePassword(QString c, QString n)
{
    QString prev = getData("password").toString();

    // Hash passwords
    c = DataCrypto::instance()->generatePassHash(c);
    n = DataCrypto::instance()->generatePassHash(n);

    if (c == prev) insertData("password", n);
}

QIcon RamUser::icon() const
{
    switch(role())
    {
    case Admin:
        return QIcon(":/icons/admin");
    case ProjectAdmin:
        return QIcon(":/icons/project-admin");
    case Lead:
        return QIcon(":/icons/lead");
    case Standard:
        return QIcon(":/icons/user");
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

RamUser::RamUser(QString uuid):
    RamObject(uuid, State)
{
    construct();

    m_schedule = RamObjectList::getObject( getData("schedule").toString(), true);
}

QString RamUser::folderPath() const
{
    return Ramses::instance()->path(RamObject::UsersFolder) + "/" + shortName();
}

QString RamUser::dataString() const
{
    QString dataStr = RamAbstractObject::dataString();
    if (dataStr == "") return "";
    // Decrypt
    return DataCrypto::instance()->clientDecrypt( dataStr );
}

void RamUser::setDataString(QString data)
{
    // Encrypt
    data = DataCrypto::instance()->clientEncrypt( data );
    // Set
    RamAbstractObject::setDataString(data);
}

void RamUser::createData(QString data)
{
    // Encrypt
    data = DataCrypto::instance()->clientEncrypt( data );
    // Set
    RamAbstractObject::createData(data);
}

// PRIVATE //

void RamUser::construct()
{
    m_icon = ":/icons/user";
    m_editRole = Admin;
}
