#include "ramabstractobject.h"

#include "ramuuid.h"
#include "dbinterface.h"
#include "ramses.h"
#include "ramnamemanager.h"
#include "duqf-app/app-style.h"

// STATIC //

QMap<QString, RamAbstractObject*> RamAbstractObject::m_existingObjects = QMap<QString, RamAbstractObject*>();

const QString RamAbstractObject::objectTypeName(ObjectType type)
{
    switch (type)
    {
    case Application: return "RamApplication";
    case Asset: return "RamAsset";
    case AssetGroup: return "RamAssetGroup";
    case FileType: return "RamFileType";
    case Object: return "RamObject";
    case Item: return "RamItem";
    case Pipe: return "RamPipe";
    case PipeFile: return "RamPipeFile";
    case Project: return "RamProject";
    case Sequence: return "RamSequence";
    case Shot: return "RamShot";
    case State: return "RamState";
    case Status: return "RamStatus";
    case Step: return "RamStep";
    case User: return "RamUser";
    case ObjectList: return "RamObjectList";
    case StepStatusHistory: return "RamStepStatusHistory";
    case ScheduleEntry: return "RamScheduleEntry";
    case ScheduleComment: return "RamScheduleComment";
    case TemplateStep: return "RamTemplateStep";
    case TemplateAssetGroup: return "RamTemplateAssetGroup";
    case Ramses: return "Ramses";
    case ItemTable: return "RamItemTable";
    }
}

const QString RamAbstractObject::subFolderName(SubFolder folder)
{
    switch(folder)
    {
    case AdminFolder: return "00-ADMIN";
    case ConfigFolder: return "_config";
    case PreProdFolder: return "01-PRE-PROD";
    case ProdFolder: return "02-PROD";
    case PostProdFolder: return "03-POST-PROD";
    case AssetsFolder: return "04-ASSETS";
    case ShotsFolder: return "05-SHOTS";
    case ExportFolder: return "06-EXPORT";
    case TemplatesFolder: return "Templates";
    case PublishFolder: return "_published";
    case VersionsFolder: return "_versions";
    case PreviewFolder: return "_preview";
    case UsersFolder: return "Users";
    case ProjectsFolder: return "Projects";
    case TrashFolder: return "_trash";
    case DataFolder: return "_data";
    case NoFolder: return "";
    }
}

RamAbstractObject *RamAbstractObject::getObject(QString uuid)
{
    Q_ASSERT_X(uuid != "", "RamObject::obj(uuid)", "UUID cannot be empty!");

    return m_existingObjects.value( uuid, nullptr );
}

RamAbstractObject *RamAbstractObject::getObject(QString shortNameOrName, ObjectType objType)
{
    QMapIterator<QString, RamAbstractObject*> i(m_existingObjects);
    while (i.hasNext())
    {
        i.next();
        RamAbstractObject *obj = i.value();
        if (obj->objectType() == objType)
        {
            if ( obj->shortName() == shortNameOrName ) return obj;
            if ( obj->name() == shortNameOrName ) return obj;
        }
    }
    return nullptr;
}

// PUBLIC //

RamAbstractObject::RamAbstractObject(QString shortName, QString name, ObjectType type, bool isVirtual)
{
    m_uuid = RamUuid::generateUuidString(shortName + name);
    m_objectType = type;

    m_virtual = isVirtual;
    if (m_virtual) return;

    // Create in the database
    QJsonObject data;
    data.insert("shortName", shortName);
    data.insert("name", name);
    data.insert("comment", "");
    QJsonDocument doc(data);
    createData(doc.toJson(QJsonDocument::Compact));
}

RamAbstractObject::~RamAbstractObject()
{
    m_existingObjects.remove(m_uuid);

    m_settings->deleteLater();
}

bool RamAbstractObject::is(RamAbstractObject *other) const
{
    if (!other) return false;
    return this->uuid() == other->uuid();
}

QString RamAbstractObject::uuid() const
{
    return m_uuid;
}

RamAbstractObject::ObjectType RamAbstractObject::objectType() const
{
    return m_objectType;
}

QString RamAbstractObject::objectTypeName() const
{
    return RamAbstractObject::objectTypeName(m_objectType);
}

QJsonObject RamAbstractObject::data() const
{
    QString dataStr = dataString();
    if (dataStr == "") return QJsonObject();

    QJsonDocument doc = QJsonDocument::fromJson(dataStr.toUtf8());
    return doc.object();
}

QJsonValue RamAbstractObject::getData(QString key) const
{
    return data().value(key);
}

void RamAbstractObject::setData(QJsonObject data)
{
    QJsonDocument doc = QJsonDocument(data);
    setDataString(doc.toJson(QJsonDocument::Compact));
    emitDataChanged(data);
}

void RamAbstractObject::insertData(QString key, QJsonValue value)
{
    // Update data before inserting
    QJsonObject d = data();
    d.insert(key, value);
    setData(d);
}

QString RamAbstractObject::shortName() const
{
    return getData("shortName").toString("");
}

void RamAbstractObject::setShortName(const QString &shortName)
{
    insertData("shortName", shortName);
}

QString RamAbstractObject::name() const
{
    return getData("name").toString("");
}

void RamAbstractObject::setName(const QString &name)
{
    insertData("name", name);
}

QString RamAbstractObject::comment() const
{
    return getData("comment").toString("");
}

void RamAbstractObject::setComment(const QString comment)
{
    insertData("comment", comment);
}

QColor RamAbstractObject::color() const
{
    QString colorName = getData("color").toString("");
    if (colorName == "") return DuUI::getColor("less-light-grey");
    return QColor( colorName );
}

void RamAbstractObject::setColor(QColor color)
{
    insertData("color", color.name() );
}

void RamAbstractObject::remove()
{
    DBInterface::instance()->removeObject(m_uuid, objectTypeName());
    emitRemoved();
}

void RamAbstractObject::restore()
{
    DBInterface::instance()->restoreObject(m_uuid, objectTypeName());
    emitRestored();
}

bool RamAbstractObject::removed()
{
    return DBInterface::instance()->isRemoved(m_uuid, objectTypeName());
}

QSettings *RamAbstractObject::settings()
{
    if (m_settings) return m_settings;

    // create settings folder and file
    QString settingsPath = path(RamAbstractObject::ConfigFolder, true) % "/" % "ramses_settings.ini";
    m_settings = new QSettings( settingsPath, QSettings::IniFormat);
    return m_settings;
}

void RamAbstractObject::reInitSettingsFile()
{
    m_settings->deleteLater();
    m_settings = nullptr;
}

QString RamAbstractObject::path(RamAbstractObject::SubFolder subFolder, bool create) const
{
    QString p = this->folderPath();
    if (p == "") return "";

    QString sub = subFolderName(subFolder);
    if (sub != "") p += "/" + sub;

    return Ramses::instance()->pathFromRamses( p, create );
}

QString RamAbstractObject::path(SubFolder subFolder, QString subPath, bool create) const
{
    QString p = path(subFolder, create);
    if (p == "") return "";

    p += "/" + subPath;

    return p;
}

QStringList RamAbstractObject::listFiles(RamObject::SubFolder subFolder, QString subPath) const
{
    QDir dir( path(subFolder) + "/" + subPath);
    QStringList files = dir.entryList( QDir::Files );
    files.removeAll( RamNameManager::MetaDataFileName );
    return files;
}

QList<QFileInfo> RamAbstractObject::listFileInfos(SubFolder subFolder, QString subPath) const
{
    QDir dir( path(subFolder) + "/" + subPath);
    QList<QFileInfo> files = dir.entryInfoList( QDir::Files );
    // Remove the ramses data file
    for (int i = files.length() - 1; i >= 0; i--)
    {
        if (files.at(i).fileName() == RamNameManager::MetaDataFileName ) files.removeAt(i);
    }
    return files;
}

QStringList RamAbstractObject::listFolders(SubFolder subFolder) const
{
    QDir dir( path(subFolder) );
    QStringList folders = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );
    return folders;
}

QStringList RamAbstractObject::listFolders(SubFolder subFolder, QString subPath) const
{
    QDir dir( path(subFolder, subPath) );
    QStringList folders = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );
    return folders;
}

void RamAbstractObject::deleteFile(QString fileName, RamObject::SubFolder folder) const
{
    QFile file( QDir(path(folder)).filePath(fileName));

    QString trashPath = path(TrashFolder, true);

    QString destination = QDir( trashPath ).filePath(fileName);
    if (QFileInfo::exists(destination))
        if (!FileUtils::moveToTrash(destination))
            QFile::remove(destination);

    file.rename( destination );
}

void RamAbstractObject::revealFolder(RamObject::SubFolder subFolder)
{
    QString p = path(subFolder);
    if (p == "") return;
    FileUtils::openInExplorer( p, true );
}

QString RamAbstractObject::previewImagePath()
{
    QDir previewDir = path(RamObject::PreviewFolder);
    QStringList filters;
    filters << "*.jpg" << "*.png" << "*.jpeg" << "*.gif";
    QStringList images = previewDir.entryList(filters, QDir::Files );

    if (images.count() == 0) return "";

    RamNameManager nm;

    foreach(QString file, images)
    {
        if (nm.setFileName(file))
        {
            if (nm.shortName().toLower() != shortName().toLower()) continue;
            return previewDir.filePath( file );
        }
    }

    // Not found, return the first one
    return previewDir.filePath( images.at(0) );
}

// PROTECTED //

RamAbstractObject::RamAbstractObject(QString uuid, ObjectType type)
{
    m_uuid = uuid;
    m_objectType = type;
}

void RamAbstractObject::setDataString(QString data)
{
    if (m_virtual) return;
    DBInterface::instance()->setObjectData(m_uuid, objectTypeName(), data);
}

QString RamAbstractObject::dataString() const
{
    return DBInterface::instance()->objectData(m_uuid, objectTypeName());
}

void RamAbstractObject::createData(QString data)
{
    if (m_virtual) return;
    DBInterface::instance()->createObject(m_uuid, objectTypeName(), data);
}
