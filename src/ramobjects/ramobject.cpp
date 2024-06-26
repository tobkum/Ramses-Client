#include "ramobject.h"

#include "duqf-utils/guiutils.h"
#include "objecteditwidget.h"
#include "mainwindow.h"
#include "ramapplication.h"
#include "ramasset.h"
#include "rampipe.h"
#include "rampipefile.h"
#include "ramsequence.h"
#include "ramscheduleentry.h"
#include "ramses.h"
#include "ramshot.h"
#include "ramstatus.h"

// PUBLIC //

RamObject *RamObject::get(QString uuid, ObjectType type)
{
    Q_ASSERT(type != Object);
    Q_ASSERT(type != Item);

    switch(type)
    {
    case Application: return RamApplication::get(uuid);
    case Asset: return RamAsset::get(uuid);
    case AssetGroup: return RamAssetGroup::get(uuid);
    case FileType: return RamFileType::get(uuid);
    case Pipe: return RamPipe::get(uuid);
    case PipeFile: return RamPipeFile::get(uuid);
    case Project: return RamProject::get(uuid);
    case Sequence: return RamSequence::get(uuid);
    case Shot: return RamShot::get(uuid);
    case State: return RamState::get(uuid);
    case Status: return RamStatus::get(uuid);
    case Step: return RamStep::get(uuid);
    case User: return RamUser::get(uuid);
    case ScheduleEntry: return RamScheduleEntry::get(uuid);
    case ScheduleRow: return RamScheduleRow::get(uuid);
    case TemplateStep: return RamTemplateStep::get(uuid);
    case TemplateAssetGroup: return RamTemplateAssetGroup::get(uuid);
    case Ramses: return Ramses::instance();
        // These aren't valid RamObjects
    case Item: return nullptr;
    case Object: return nullptr;
    }

    return nullptr;
}

RamObject *RamObject::get(QString uuid, QString tableName)
{
    return RamObject::get(uuid, RamObject::objectTypeFromName(tableName));
}

bool RamObject::validateData(const QString &data, ObjectType type)
{
    switch(type) {
    case Application:
        return RamApplication::validateData(data);
    case Asset:
        return RamAsset::validateData(data);
    case AssetGroup:
        return RamAssetGroup::validateData(data);
    case FileType:
        return RamFileType::validateData(data);
    case Pipe:
        return RamPipe::validateData(data);
    case PipeFile:
        return RamPipeFile::validateData(data);
    case Project:
        return RamProject::validateData(data);
    case Sequence:
        return RamSequence::validateData(data);
    case Shot:
        return RamShot::validateData(data);
    case State:
        return RamState::validateData(data);
    case Status:
        return RamStatus::validateData(data);
    case Step:
        return RamStep::validateData(data);
    case User:
        return RamUser::validateData(data);
    case ScheduleEntry:
        return RamScheduleEntry::validateData(data);
    case ScheduleRow:
        return RamScheduleRow::validateData(data);
    case TemplateStep:
        return RamTemplateStep::validateData(data);
    case TemplateAssetGroup:
        return RamTemplateAssetGroup::validateData(data);
    case Ramses:
    case Object:
    case Item:
        break;
    }
    return true;
}

RamObject::RamObject(QString shortName, QString name, ObjectType type, QObject *parent, bool isVirtual):
    QObject(parent),
    RamAbstractObject(shortName, name, type, isVirtual)
{
    construct(parent);
}

RamObject *RamObject::objectForColumn(QString columnUuid) const {
    Q_UNUSED(columnUuid);
    return nullptr;
}

void RamObject::remove()
{
    RamAbstractObject::remove();
    this->disconnect();
}

bool RamObject::canEdit()
{
    RamUser *u = Ramses::instance()->currentUser();
    if (!u) return false;
    return u->role() >= m_editRole;
}

void RamObject::reload()
{
    QJsonObject d = reloadData();
    emit dataChanged(this);
}

// PROTECTED //

RamObject::RamObject(QString uuid, ObjectType type, QObject *parent):
    QObject(parent),
    RamAbstractObject(uuid, type)
{
    construct(parent);
}

QJsonObject RamObject::reloadData()
{
    return data();
}

void RamObject::emitDataChanged()
{
    if (!m_saveSuspended) emit dataChanged(this);
}

void RamObject::emitRemoved()
{
    emit removed(this);
}

void RamObject::emitRestored()
{
    emit restored(this);
}

QFrame *RamObject::createEditFrame(ObjectEditWidget *w)
{
    QFrame *f = new QFrame();
    QVBoxLayout *l = new QVBoxLayout();
    l->setContentsMargins(3,3,3,3);
    l->addWidget(w);
    f->setLayout(l);
    return f;
}

void RamObject::showEdit(QWidget *w, QString title)
{
    if (!w) return;

    ObjectEditWidget *oEdit = qobject_cast<ObjectEditWidget*>( w->layout()->itemAt(0)->widget() );
    if (oEdit) oEdit->setObject(this);

    MainWindow *mw = (MainWindow*)GuiUtils::appMainWindow();
    if (title == "") title = this->name();
    if (title == "") title = this->shortName();
    if (title == "") title = "Properties";

    w->setEnabled(false);

    if (m_editable)
    {
        RamUser *u = Ramses::instance()->currentUser();
        if (u)
        {
            w->setEnabled(u->role() >= m_editRole);
            if (u->is(this)) w->setEnabled(true);
        }
    }

    mw->setPropertiesDockWidget( w, title, m_icon);
}

RamObjectModel *RamObject::createModel(RamObject::ObjectType type, QString modelName)
{
    RamObjectModel *model = new RamObjectModel(type, this);

    // Connect all model changes to save the list
    connect(model, &RamObjectModel::rowsInserted, this, &RamObject::saveModel);
    connect(model, &RamObjectModel::rowsRemoved, this, &RamObject::saveModel);
    connect(model, &RamObjectModel::rowsMoved, this, &RamObject::saveModel);

    // keep
    m_subModels[model] = modelName;

    return model;
}

void RamObject::checkData(QString uuid, QString d, QString table)
{
    // Don't reload if we're currently saving the data
    if (m_savingData) return;
    // Not for me...
    if (table != objectTypeName()) return;
    if (uuid != m_uuid) return;
    // Update cache!
    m_cachedData = d;
    // Reset lists
    QJsonObject dataObj = data();
    QMapIterator<RamObjectModel *, QString> it = QMapIterator<RamObjectModel*, QString>( m_subModels );
    while (it.hasNext()) {
        it.next();
        // Clear
        RamObjectModel *model = it.key();
        QString modelName = it.value();
        //if (m_objectType != RamAbstractObject::Project)
            loadModel(model, modelName, dataObj);
    }
    emit dataChanged(this);
}

void RamObject::checkAvailability(QString uuid, bool availability)
{
    // Not for me...
    if (uuid != m_uuid) return;

    if (availability) emit restored(this);
    else emit removed(this);
}

void RamObject::saveModel()
{
    emit dataChanged(this);
    if (m_loadingModels) return;
    // Get the model and its name
    RamObjectModel *o = qobject_cast<RamObjectModel*>( sender() );
    QString modelName = m_subModels.value(o, "");
    if (modelName == "") return;

    // Get the stringlist
    QStringList uuids = o->toStringList();
    QJsonArray arr = QJsonArray::fromStringList(uuids);
    insertData(modelName, arr);
}

void RamObject::loadModel(RamObjectModel *model, QString modelName, QJsonObject d)
{
    if (modelName == "") return;
    m_loadingModels = true;
    if (d.isEmpty()) d = data();
    model->clear();
    // Get uuids
    QVector<QString> uuids;
    QJsonArray arr = d.value(modelName).toArray();
    for (int i = 0; i < arr.count(); i++)
    {
        uuids << arr.at(i).toString();
    }
    // Set uuids
    model->insertObjects(0, uuids);
    m_loadingModels = false;
}

void RamObject::construct(QObject *parent)
{
    if (!parent && m_objectType != Ramses) this->setParent(Ramses::instance());
    this->setObjectName( objectTypeName() + " | " + shortName() + " (" + m_uuid + ")" );

    // Monitor db changes
    connect(LocalDataInterface::instance(), &LocalDataInterface::dataChanged, this, &RamObject::checkData);
    connect(LocalDataInterface::instance(), &LocalDataInterface::availabilityChanged, this, &RamObject::checkAvailability);
}





