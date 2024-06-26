﻿#include "ramstatus.h"

#include "ramnamemanager.h"
#include "ramshot.h"
#include "ramasset.h"
#include "ramworkingfolder.h"
#include "statuseditwidget.h"
#include "ramses.h"
#include "ramuuid.h"

// PROTECTED //

QFrame *RamStatus::ui_editWidget = nullptr;

QHash<QString, RamStatus*> RamStatus::m_existingObjects = QHash<QString, RamStatus*>();

RamStatus *RamStatus::get(QString uuid)
{
    if (!checkUuid(uuid, Status)) return nullptr;

    RamStatus *s = m_existingObjects.value(uuid);
    if (s) return s;

    // Finally return a new instance
    return new RamStatus(uuid);
}

RamStatus *RamStatus::c(RamObject *o)
{
    //return qobject_cast<RamStatus*>(o);
    // For performance, reinterpret_cast, but be careful with the object passed!
    return reinterpret_cast<RamStatus*>(o);
}

RamStatus *RamStatus::copy(RamStatus *other, RamUser *user)
{
    RamStatus *status = new RamStatus(
                user,
                other->item(),
                other->step()
                );

    status->setState( other->state() );
    status->setCompletionRatio( other->completionRatio() );
    status->setVersion( other->version() );
    status->assignUser( other->assignedUser() );
    status->setDifficulty( other->difficulty() );
    status->setGoal( other->goal() );
    status->setUseAutoEstimation( other->useAutoEstimation() );
    status->setComment( other->comment() );

    return status;
}

RamStatus *RamStatus::noStatus(RamAbstractItem *item, RamStep *step)
{
    qDebug() << "Creating a noStatus for the item: " << item->shortName() << " and step: " << step->shortName();

    RamStatus *no = new RamStatus(
                Ramses::instance()->ramsesUser(),
                item,
                step
                );
    return no;
}

// PUBLIC //

RamStatus::RamStatus(RamUser *user, RamAbstractItem *item, RamStep *step, bool isVirtual):
    RamObject(item->shortName() + "-" + step->shortName(),
              item->name() + " | " + step->name(),
              Status,
              item,
              isVirtual)
{
    construct();

    m_step = step;
    m_item = item;

    if (m_virtual) return;

    QJsonObject d = data();

    if (user) d.insert("user", user->uuid());
    else d.insert("user", "none");

    d.insert("item", item->uuid());
    if (item->objectType() == RamObject::Shot) d.insert("itemType", "shot");
    else if (item->objectType() == RamObject::Asset) d.insert("itemType", "asset");
    else d.insert("itemType", "item");

    d.insert("step", step->uuid());

    RamState *state = Ramses::instance()->noState();
    d.insert("state", state->uuid());
    d.insert("completionRatio", 0);

    setData(d);

    createData();
}

RamStatus::RamStatus(QString uuid):
    RamObject(uuid, Status)
{
    construct();

    QJsonObject d = data();

    QString itemType = d.value("itemType").toString("asset");
    QString itemUuid = d.value("item").toString("none");

    if (itemType == "shot") {
        m_item = RamShot::get( itemUuid );
    }
    else {
        m_item = RamAsset::get( itemUuid );
    }

    m_step = RamStep::get( d.value("step").toString("none") );

    if (m_step && m_item) connectEvents();
    else invalidate();
}

RamUser *RamStatus::modifiedBy() const
{
    QString userUuid( getData("user").toString("none") );
    RamUser *u = RamUser::get( userUuid );
    if (u) return u;
    return Ramses::instance()->ramsesUser();
}

void RamStatus::setModifiedBy(RamUser *user)
{
    // Update date
    setDate();

    RamUser *prevUser = modifiedBy();
    if (prevUser->is(user)) return;

    if (!user) insertData("user", "none");
    else insertData("user", user->uuid());
    // Create history
    LocalDataInterface::instance()->createObject(
                RamUuid::generateUuidString(this->shortName() + this->name()),
                "RamStatusHistory",
                this->dataString()
                );
}

RamStep *RamStatus::step() const
{
    return m_step;
}

QString RamStatus::stepUuid() const
{
    return getData("step").toString();
}

RamAbstractItem *RamStatus::item() const
{
    return m_item;
}

QString RamStatus::itemUuid() const
{
    return getData("item").toString();
}

bool RamStatus::isNoState() const
{
    RamState *noState = Ramses::instance()->noState();
    return noState->is(state());
}

bool RamStatus::isDisabled() const
{
    return isNoState();
}

int RamStatus::completionRatio() const
{
    return getData("completionRatio").toInt(50);
}

void RamStatus::setCompletionRatio(int completionRatio)
{
    QJsonObject d = data();
    d.insert("completionRatio", completionRatio);
    updateData(&d);
}

RamState *RamStatus::state() const
{
    if (m_virtual) return Ramses::instance()->noState();
    return RamState::get( getData("state").toString("none") );
}

void RamStatus::setState(RamState *newState)
{
    RamState *currentState = state();
    if (currentState) disconnect(currentState, nullptr, this, nullptr);

    QJsonObject d = data();

    d.insert("state", newState->uuid());
    d.insert( "completionRatio", newState->completionRatio() );
    updateData(&d);

    connect(newState, SIGNAL(removed(RamObject*)), this, SLOT(stateRemoved()));
}

int RamStatus::version() const
{
    if (m_virtual) return 0;
    return getData("version").toInt();
}

void RamStatus::setVersion(int version)
{
    QJsonObject d = data();
    d.insert("version", version);
    updateData(&d);
}

QDateTime RamStatus::date() const
{
    if (m_virtual) return QDateTime::currentDateTime();
    return QDateTime::fromString( getData("date").toString(), "yyyy-MM-dd hh:mm:ss");
}

void RamStatus::setDate(const QDateTime &date)
{
    insertData("date", date.toString("yyyy-MM-dd hh:mm:ss"));
}

bool RamStatus::useDueDate() const
{
    if (m_virtual) return false;
    return getData("useDueDate").toBool(false);
}

void RamStatus::setUseDueDate(bool use)
{
    insertData("useDueDate", use);
}

QDate RamStatus::dueDate() const
{
    QString defaultDate = QDate::currentDate().toString("yyyy-MM-dd");
    if (m_virtual) return QDate::fromString(defaultDate, "yyyy-MM-dd");
    return QDate::fromString( getData("dueDate").toString(defaultDate), "yyyy-MM-dd");
}

void RamStatus::setDueDate(const QDate &date)
{
    insertData("dueDate", date.toString("yyyy-MM-dd"));
    setUseDueDate(true);
}

RamStatus::Priority RamStatus::priority() const
{
    return static_cast<Priority>(getData("priority").toInt(NoPriority));
}

void RamStatus::setPriority(Priority p)
{
    insertData("priority", p);
}

qreal RamStatus::lateness() const
{
    if (!useDueDate()) return 0;
    // estimation / days left
    qreal daysLeft = QDate::currentDate().daysTo( dueDate() );
    qreal est = qreal(estimation()*(100-completionRatio()))/100.0;
    if (est <= 0) return 0;
    if (daysLeft <= 0) return 3;
    return est/daysLeft;
}

bool RamStatus::isPublished() const
{
    if (m_virtual) return false;
    return getData("published").toBool();
}

void RamStatus::setPublished(bool published)
{
    QJsonObject d = data();
    d.insert("published", published);
    updateData(&d);
}

RamUser *RamStatus::assignedUser() const
{
    if (m_virtual) return nullptr;
    return RamUser::get( getData("assignedUser").toString("none") );
}

void RamStatus::assignUser(RamObject *user)
{
    QJsonObject d = data();
    RamUser *currentUser = assignedUser();
    if (currentUser) disconnect(assignedUser(), nullptr, this, nullptr);
    if (!user) d.insert("assignedUser", "none");
    else {
        d.insert("assignedUser", user->uuid());
        connect(user, SIGNAL(removed(RamObject*)), this, SLOT(assignedUserRemoved()));
    }
    updateData(&d);
}

RamStatus::Difficulty RamStatus::difficulty() const
{
    if (m_virtual) return Medium;
    QString dffclt = getData("difficulty").toString("medium");
    if (dffclt == "veryEasy") return VeryEasy;
    else if (dffclt == "easy") return Easy;
    else if (dffclt == "medium") return Medium;
    else if (dffclt == "hard") return Hard;
    else if (dffclt == "veryHard") return VeryHard;
    return Medium;
}

void RamStatus::setDifficulty(Difficulty newDifficulty)
{
    QJsonObject d = data();
    switch(newDifficulty) {
    case VeryEasy:
        d.insert("difficulty", "veryEasy");
        break;
    case Easy:
        d.insert("difficulty", "easy");
        break;
    case Medium:
        d.insert("difficulty", "medium");
        break;
    case Hard:
        d.insert("difficulty", "hard");
        break;
    case VeryHard:
        d.insert("difficulty", "veryHard");
        break;
    }

    updateData(&d);
}

float RamStatus::goal() const
{
    if (m_virtual) return 0;
    // If state is none, 0!
    RamState *noState = Ramses::instance()->noState();
    if (noState->is(state())) return 0.0;

    float g = getData("goal").toDouble();

    return g;
}

void RamStatus::setGoal(float newGoal)
{
    QJsonObject d = data();
    d.insert("goal", newGoal);
    updateData(&d);
}

float RamStatus::estimation() const
{
    return estimation( difficulty() );
}

float RamStatus::estimation(int difficulty) const
{
    float est = 0.0;

    // If state is none, 0!
    RamState *noState = Ramses::instance()->noState();
    if (noState->is(state())) return 0.0;

    // If step doesn't exist, it's 0
    if (!m_step) return 0.0;

    switch (difficulty)
    {
    case VeryEasy:
    {
        est = m_step->estimationVeryEasy();
        break;
    }
    case Easy:
    {
        est = m_step->estimationEasy();
        break;
    }
    case Medium:
    {
        est = m_step->estimationMedium();
        break;
    }
    case Hard:
    {
        est = m_step->estimationHard();
        break;
    }
    case VeryHard:
    {
        est = m_step->estimationVeryHard();
        break;
    }
    default:
    {
        est = m_step->estimationMedium();
    }
    }

    // Multiply by duration and num assets if shot
    if ( m_item && m_item->objectType() == RamObject::Shot )
    {
        RamShot *shot = qobject_cast<RamShot*>( m_item );
        if (m_step->estimationMethod() == RamStep::EstimatePerSecond)
            est *= shot->duration();
        RamAssetGroup *ag = m_step->estimationMultiplyGroup();
        if (ag)
        {
            // count assets
            int numAssets = 0;
            for (int i = 0; i < shot->assets()->rowCount(); i++)
            {
                RamAsset *asset = RamAsset::c( shot->assets()->get(i) );
                if (asset->assetGroup()->is(ag)) numAssets++;
            }
            if (numAssets > 0) est *= numAssets;
        }
    }
    return est;
}

bool RamStatus::useAutoEstimation() const
{
    if (m_virtual) return true;
    RamState *noState = Ramses::instance()->noState();
    if (noState->is(state())) return true;

    return getData("useAutoEstimation").toBool(true);
}

void RamStatus::setUseAutoEstimation(bool newAutoEstimation)
{
    QJsonObject d = data();

    d.insert("useAutoEstimation", newAutoEstimation);
    if (!newAutoEstimation && d.value("goal").toDouble() <= 0) d.insert("goal", estimation());

    updateData(&d);
}

RamWorkingFolder RamStatus::workingFolder() const
{
    return RamWorkingFolder(this->path());
}

QString RamStatus::createFileFromTemplate(QString filePath) const
{
    if (!QFileInfo::exists( filePath)) return "";

    // Generate destination name
    RamNameManager nm;
    nm.setFileName(filePath);
    nm.setProject( m_item->project()->shortName() );
    nm.setStep( m_step->shortName() );
    nm.setResource( nm.shortName() );
    if (m_item->objectType() == Asset) nm.setType("A");
    else if (m_item->objectType() == Shot) nm.setType("S");
    else nm.setType("G");
    nm.setShortName( m_item->shortName() );
    if (nm.resource() == "Template") nm.setResource("");

    QString destination = QDir(
                path(NoFolder, true)
                ).filePath( nm.fileName() );

     if (QFileInfo::exists( destination ))
     {
         nm.setResource( nm.resource() + "+new+");
         destination = QDir(
                     path(NoFolder, true)
                     ).filePath( nm.fileName() );
     }

     if (QFile::copy(filePath, destination)) return destination;
     return "";
}

QString RamStatus::createFileFromResource(QString filePath) const
{
    // Get infos
    RamNameManager nm;
    nm.setFileName(filePath);

    RamWorkingFolder wf( QFileInfo(filePath).path() );

    // Get the latest publish if any
    QStringList templateFiles = wf.publishedFiles(nm.resource());
    foreach (QString tFile, templateFiles)
    {
        // Check the file extension, which should be the same
        if ( QFileInfo(tFile).suffix() == nm.extension()) return createFileFromTemplate(tFile);
    }
    // Try with the first published file
    if (templateFiles.count() > 0) return createFileFromTemplate(templateFiles.at(0));

    // Use the given file path
    return createFileFromTemplate( filePath );
}

QString RamStatus::restoreVersionFile(QString fileName) const
{
    QString restoredPath;
    RamNameManager nm;
    nm.setFileName( fileName );
    QString resource = nm.resource();
    QString v = QString::number( nm.version() );
    while(v.count() < 3)
        v = "0" + v;
    resource += "+restored-v" + v + "+";
    nm.setResource(resource);
    nm.setVersion(-1);

    QString versionPath = QDir( path(RamObject::VersionsFolder) ).filePath( fileName );
    restoredPath = QDir( path()).filePath( nm.fileName() );

    // Copy
    QFile::copy(versionPath, restoredPath);

    return restoredPath;

}

QString RamStatus::name() const
{
    RamAbstractItem *item = this->item();
    RamStep *step = this->step();
    RamState *state = this->state();
    if (!item || !step) return "Unknown Task";
    if (!state) return item->name() + " | " + step->name();
    return item->name() + " | " + step->name() + " | " + state->shortName();
}

QString RamStatus::shortName() const
{
    RamAbstractItem *item = this->item();
    RamStep *step = this->step();
    if (!item || !step) return "Task";
    return item->shortName() + " | " + step->shortName();
}

QString RamStatus::previewImagePath() const
{
    if (this->isNoState()) return "";
    return RamObject::previewImagePath();
}

QString RamStatus::details() const
{
    if (m_virtual) return "";
    if (this->isNoState()) return "";
    QString details;

    RamUser *au = assignedUser();
    if (au) details = "Assigned to: **" +
            au->name() %
            "**  \nDifficulty: ";
    else details = "Not assigned  \nDifficulty: ";

    switch( difficulty() )
    {
    case RamStatus::VeryEasy: { details += "**Very easy**"; break; }
    case RamStatus::Easy: { details += "**Easy**"; break; }
    case RamStatus::Medium: { details += "**Medium**"; break; }
    case RamStatus::Hard: { details += "**Hard**"; break; }
    case RamStatus::VeryHard: { details += "**Very hard**"; break; }
    }


    // Ratio
    float est = 0;
    bool useAuto = useAutoEstimation();
    if (useAuto) est = estimation();
    else est = goal();

    // Estimation or goal
    if (useAuto) details +=
            "  \nEstimation: **" +
            QString::number(est, 'f', 1) +
            " days**";
    else details +=
            "  \nGoal: **" +
            QString::number(est, 'f', 1) +
            " days**";

    if (useDueDate()) details += "  \nDue date: " + dueDate().toString("**yyyy-MM-dd**");

    qDebug() << priority();

    switch (priority()) {
    case LowPriority:
        details += "  \nPriority: **Low**";
        break;
    case MediumPriority:
        details += "  \nPriority: **Medium**";
        break;
    case HighPriority:
        details += "  \nPriority: **High**";
        break;
    case NoPriority:
        break;
    }

   if (isPublished()) details += "  \n► Published";

   return "_" + details + "_";

}

QString RamStatus::subDetails() const
{
    if (m_virtual) return "";
    if (this->isNoState()) return "";
    //subdetails
    QString dateFormat = "yyyy-MM-dd hh:mm:ss";
    RamUser *u = Ramses::instance()->currentUser();
    if (u)
    {
        QSettings settings;
        dateFormat = settings.value("appearance/dateFormat", dateFormat).toString();
    }
    return "Modified on: " +
            date().toString(dateFormat) +
            "\nBy: " +
            modifiedBy()->name();
}

QVariant RamStatus::roleData(int role) const
{
    switch(role)
    {
    case Qt::ToolTipRole: {
        RamAbstractItem *item = this->item();
        RamStep *step = this->step();
        RamState *state = this->state();
        return item->name() + " | " + step->name() + " | " + state->name();
    }
    case Qt::StatusTipRole: {
        RamAbstractItem *item = this->item();
        RamStep *step = this->step();
        RamState *state = this->state();
        return item->shortName() + " | " + step->shortName() + " | " + state->name();
    }
    case RamAbstractObject::Completion: return this->completionRatio();
    case RamAbstractObject::Estimation: {
        if (this->useAutoEstimation()) return this->estimation();
        return this->goal();
    }
    case RamAbstractObject::Goal: return this->goal();
    case RamAbstractObject::ProgressColor: return this->state()->color();
    case RamAbstractObject::LabelColor: {
        RamUser *u = this->assignedUser();
        if (u) return u->color();
        return QColor();
    }
    case RamAbstractObject::Difficulty: return this->difficulty();
    case RamAbstractObject::SizeHint: {
        QSize s = RamObject::roleData(role).toSize();
        s.setWidth(300);
        return s;
    }
    case RamAbstractObject::DetailedSizeHint: {
        QSize s = RamObject::roleData(role).toSize();
        if (s.width() < 300) s.setWidth(300);
        return s;
    }
    case RamAbstractObject::Published: {
        return this->isPublished();
    }
    case RamAbstractObject::Priority: {
        if (this->completionRatio() >= 100)
            return 0;

        return this->lateness() + this->priority();
    }
    }

    return RamObject::roleData(role);
}

QString RamStatus::fileName() const
{
    RamAbstractItem *item = this->item();
    if (!item) return RamAbstractObject::fileName();
    RamStep *step = this->step();
    if (!step) return RamAbstractObject::fileName();
    RamProject *proj = item->project();
    if (!proj) proj = step->project();
    if (!proj) return RamAbstractObject::fileName();
    QString t = "_G_";
    if (item->productionType() == RamStep::AssetProduction) t = "_A_";
    else if (item->productionType() == RamStep::ShotProduction) t = "_S_";
    return proj->shortName() + t + item->shortName() + "_" + step->shortName();
}

// PUBLIC SLOTS //

void RamStatus::edit(bool show)
{
    if (!ui_editWidget) ui_editWidget = createEditFrame(new StatusEditWidget());

    if (show) showEdit( ui_editWidget );
}

// PROTECTED //

QString RamStatus::folderPath() const
{
    if (!m_item) return "";
    RamProject *project = m_item->project();
    if (!project) return "";
    QString type = "_G_";
    if (m_item->objectType() == RamObject::Shot) type = "_S_";
    else if (m_item->objectType() == RamObject::Asset) type = "_A_";
    return m_item->path() + "/" + project->shortName() + type + m_item->shortName() + "_" + m_step->shortName();
}

// PRIVATE SLOTS //

void RamStatus::stateRemoved()
{
    this->setState( Ramses::instance()->wipState() );
}

void RamStatus::assignedUserRemoved()
{
    assignUser(nullptr);
}

// PRIVATE //

void RamStatus::construct()
{
    m_existingObjects[m_uuid] = this;
    m_icon = ":/icons/status";
    m_editRole = Lead;
}

void RamStatus::connectEvents()
{
    if (m_item) connect(m_item, &RamAbstractItem::removed, this, &RamStatus::remove);
    if( m_step) connect(m_step, &RamStep::removed, this, &RamStatus::remove);
}

void RamStatus::updateData(QJsonObject *d)
{
    d->insert("date", QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd hh:mm:ss"));

    setData(*d);

    // Emit for the models
    RamAbstractItem *item = this->item();
    if (item) item->emitDataChanged();
}
