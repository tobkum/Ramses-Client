#include "ramabstractobject.h"

#include "duqf-app/app-config.h"
#include "ramuuid.h"
#include "dbinterface.h"
#include "ramses.h"
#include "ramnamemanager.h"

// STATIC //

QRegularExpression RamAbstractObject::m_rxsn = QRegularExpression();

QHash<QString, QPixmap> RamAbstractObject::m_iconPixmaps = QHash<QString, QPixmap>();

QHash<QString, RamAbstractObject*> RamAbstractObject::m_allObjects = QHash<QString, RamAbstractObject*>();

QSet<RamAbstractObject*> RamAbstractObject::m_invalidObjects = QSet<RamAbstractObject*>();

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
    case ScheduleEntry: return "RamScheduleEntry";
    case ScheduleRow: return "RamScheduleRow";
    case TemplateStep: return "RamTemplateStep";
    case TemplateAssetGroup: return "RamTemplateAssetGroup";
    case Ramses: return "Ramses";
    }
    return "RamObject";
}

RamAbstractObject::ObjectType RamAbstractObject::objectTypeFromName(QString name)
{
    if (name == "RamApplication") return Application;
    if (name == "RamAsset") return Asset;
    if (name == "RamApplication") return Application;
    if (name == "RamAssetGroup") return AssetGroup;
    if (name == "RamFileType") return FileType;
    if (name == "RamObject") return Object;
    if (name == "RamItem") return Item;
    if (name == "RamPipe") return Pipe;
    if (name == "RamPipeFile") return PipeFile;
    if (name == "RamProject") return Project;
    if (name == "RamSequence") return Sequence;
    if (name == "RamShot") return Shot;
    if (name == "RamState") return State;
    if (name == "RamStatus") return Status;
    if (name == "RamStep") return Step;
    if (name == "RamUser") return User;
    if (name == "RamScheduleEntry") return ScheduleEntry;
    if (name == "RamScheduleRow") return ScheduleRow;
    if (name == "RamTemplateStep") return TemplateStep;
    if (name == "RamTemplateAssetGroup") return TemplateAssetGroup;
    if (name == "Ramses") return Ramses;
    return Object;
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
    case OutputFolder: return "06-OUTPUT";
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
    return "";
}

void RamAbstractObject::setObjectData(QString uuid, QString dataStr)
{
    // TODO use the localdbinterface instead? (we need the type)

    RamAbstractObject *obj = nullptr;
    if (m_allObjects.contains(uuid)) obj = m_allObjects[uuid];
    if (!obj) return;

    obj->setDataString(dataStr);
}

void RamAbstractObject::setObjectData(QString uuid, QJsonObject data)
{
    // TODO use the localdbinterface instead? (we need the type)

    RamAbstractObject *obj = nullptr;
    if (m_allObjects.contains(uuid)) obj = m_allObjects[uuid];
    if (!obj) return;

    obj->setData(data);
}

QJsonObject RamAbstractObject::getObjectData(QString uuid)
{
    // TODO use the localdbinterface instead? (we need the type)

    RamAbstractObject *obj = nullptr;
    if (m_allObjects.contains(uuid)) obj = m_allObjects[uuid];
    if (!obj) return QJsonObject();

    return obj->data();
}

QString RamAbstractObject::getObjectDataString(QString uuid)
{
    // TODO use the localdbinterface instead? (we need the type)

    RamAbstractObject *obj = nullptr;
    if (m_allObjects.contains(uuid)) obj = m_allObjects[uuid];
    if (!obj) return "{}";

    return obj->dataString();
}

QString RamAbstractObject::getObjectPath(QString uuid)
{
    RamAbstractObject *obj = nullptr;
    if (m_allObjects.contains(uuid)) obj = m_allObjects[uuid];
    if (!obj) return "";

    return obj->path();
}

const QString RamAbstractObject::uuidFromPath(QString path, ObjectType type)
{
    path = QDir::cleanPath(path);
    if (!path.endsWith("/")) path = path + "/";

    // Check the path of all existing ramObjects
    QHashIterator<QString, RamAbstractObject*> i = QHashIterator<QString, RamAbstractObject*>(m_allObjects);
    // We'll keep the closest match
    while (i.hasNext())
    {
        i.next();
        RamAbstractObject *o = i.value();
        if (!o->isValid()) continue;
        if (o->objectType() != type) continue;

        // If we have the same starting path, that's the one!
        QString testPath = o->path();
        if (!testPath.endsWith("/")) testPath = testPath + "/";

        if (path.startsWith(testPath)) return o->uuid();
    }
    return "";
}

// PUBLIC //

RamAbstractObject::RamAbstractObject(QString shortName, QString name, ObjectType type, bool isVirtual)
{
    m_uuid = RamUuid::generateUuidString(shortName + name);
    m_objectType = type;
    m_virtual = isVirtual;

    // Create in the database
    QJsonObject data;
    data.insert("shortName", shortName);
    data.insert("name", name);
    data.insert("comment", "");
    data.insert("order", 0);

    setData(data);

    construct();
}

RamAbstractObject::~RamAbstractObject()
{
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

bool RamAbstractObject::isValid() const
{
    return m_valid;
}

void RamAbstractObject::invalidate()
{
    m_valid = false;
    m_invalidObjects.insert(this);
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
    const QString str = doc.toJson(QJsonDocument::Compact);
    setDataString(str);
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
    return getData("shortName").toString("UNKNOWN");
}

void RamAbstractObject::setShortName(const QString &shortName)
{
    insertData("shortName", shortName);
}

bool RamAbstractObject::validateShortName(const QString &shortName)
{
    // Accept "NEW"
    if (shortName.toLower() == "new") return true;

    QRegularExpression rxsn = shortNameRegularExpression();
    QRegularExpressionMatch match = rxsn.match( shortName );
    if (!match.hasMatch()) return false;

    return true;
}

QString RamAbstractObject::name() const
{
    return getData("name").toString("Unnamed");
}

void RamAbstractObject::setName(const QString &name)
{
    insertData("name", name);
}

QString RamAbstractObject::comment() const
{
    return getData("comment").toString("");
}

void RamAbstractObject::setComment(const QString &comment)
{
    insertData("comment", comment.trimmed());
}

QColor RamAbstractObject::color() const
{
    QString colorName = getData("color").toString("");
    if (colorName == "") return QColor(157,157,157);
    return QColor( colorName );
}

void RamAbstractObject::setColor(QColor color)
{
    insertData("color", color.name() );
}

int RamAbstractObject::order() const
{
    return getData("order").toInt(0);
}

void RamAbstractObject::setOrder(int o)
{
    insertData("order", o);
}

QString RamAbstractObject::customSettings() const
{
    return getData("customSettings").toString();
}

void RamAbstractObject::setCustomSettings(const QString &newGeneralSettings)
{
    insertData("customSettings", newGeneralSettings);
}

DuIcon RamAbstractObject::icon() const
{
    return DuIcon(iconName());
}

QPixmap RamAbstractObject::iconPixmap() const
{
    return RamAbstractObject::iconPixmap( iconName() );
}

QVariant RamAbstractObject::roleData(int role) const
{
    switch(role)
    {
    case Qt::DisplayRole: return this->name();
    case Qt::ToolTipRole: {
        QString tt = this->shortName() + " | " + this->name();
        tt += "\nUUID: " + this->uuid();
        return tt;
    }
    case Qt::StatusTipRole: return this->shortName() + " | " + this->name();
    case Qt::ForegroundRole: return QBrush(this->color());
    case Qt::DecorationRole: return this->iconPixmap();
    case Qt::EditRole: return "";
    case RamAbstractObject::Name: return this->name();
    case RamAbstractObject::ShortName: return this->shortName();
    case RamAbstractObject::Type: return this->objectType();
    case RamAbstractObject::Details: return this->details();
    case RamAbstractObject::Disabled: return this->isDisabled();
    case RamAbstractObject::Path: return this->path();
    case RamAbstractObject::Comment: return this->comment();
    case RamAbstractObject::Completion: return 100;
    case RamAbstractObject::Lateness: return 0;
    case RamAbstractObject::Estimation: return 0;
    case RamAbstractObject::Goal: return 1;
    case RamAbstractObject::TimeSpent: return 0;
    case RamAbstractObject::ProgressColor: return this->color();
    case RamAbstractObject::LabelColor: return QColor();
    case RamAbstractObject::PreviewImagePath: return this->previewImagePath();
    case RamAbstractObject::SubDetails: return this->subDetails();
    case RamAbstractObject::SizeHint: return QSize(200, 30);
    case RamAbstractObject::DetailedSizeHint: {
        int h = 30;
        int w = 200;
        QTextDocument td;
        td.setIndentWidth(20);
        QString comment = this->comment();
        if (comment != "") {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            td.setPlainText(comment);
#else
            td.setMarkdown(comment);
#endif
            h += 5 + td.size().height();
            w = std::fmax(w, td.size().width());
        }
        QString details = this->details();
        if (details != "") {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            td.setPlainText(details);
#else
            td.setMarkdown(details);
#endif
            h += 5 + td.size().height();
            w = std::fmax(w, td.size().width());
        }
        QString subDetails = this->subDetails();
        if (subDetails != "") {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            td.setPlainText(subDetails);
#else
            td.setMarkdown(subDetails);
#endif
            h += 5 + td.size().height();
            w = std::fmax(w, td.size().width());
        }
        QString imagePath = this->previewImagePath();
        if (imagePath != "") {
            h += w*9/16;
        }
        return QSize(w, h);
    }
    case RamAbstractObject::IsPM: return false;
    case RamAbstractObject::Date: return QDateTime();
    case RamAbstractObject::IsComment: return false;
    case RamAbstractObject::Difficulty: return 0;
    case RamAbstractObject::Duration: return 0;
    case RamAbstractObject::Order: return order();
    case RamAbstractObject::FileName: return fileName();
    case RamAbstractObject::Published: return false;
    case RamAbstractObject::Priority: return 0.0;
    }

    return this->uuid();
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

bool RamAbstractObject::isRemoved()
{
    return DBInterface::instance()->isRemoved(m_uuid, objectTypeName());
}

QDateTime RamAbstractObject::modificationDate() const
{
    QString d = DBInterface::instance()->modificationDate(m_uuid, objectTypeName());
    return QDateTime::fromString(d, DATETIME_DATA_FORMAT);
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

    return QDir::cleanPath(p);
}

QString RamAbstractObject::fileName() const
{
    return "";
}

QStringList RamAbstractObject::listFiles(RamObject::SubFolder subFolder, QString subPath) const
{
    QDir dir( path(subFolder) + "/" + subPath);
    QStringList files = dir.entryList( QDir::Files );
    files.removeAll( RamNameManager::MetaDataFileName );
    return files;
}

QFileInfoList RamAbstractObject::listFileInfos(SubFolder subFolder, QString subPath) const
{
    QDir dir( path(subFolder) + "/" + subPath);
    QFileInfoList files = dir.entryInfoList( QDir::Files );
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

QString RamAbstractObject::previewImagePath() const
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
    m_created = true;

    m_uuid = uuid;
    m_objectType = type;

    // cache the data
    m_cachedData = dataString();

    construct();
}

bool RamAbstractObject::isSaveSuspended() const
{
    return m_saveSuspended;
}

void RamAbstractObject::suspendSave(bool suspend)
{
    m_saveSuspended = suspend;
}

void RamAbstractObject::setDataString(QString data)
{
    m_savingData = true;

    // Cache the data to improve performance
    m_cachedData = data;

    if (m_virtual || m_saveSuspended || !m_created) return;

#ifdef DEBUG_DATA
    qDebug() << "<<<";
    qDebug().noquote() << "Setting data for: " + shortName() + " (" + objectTypeName() + ")";
    qDebug().noquote() << "UUID: " + m_uuid;
    qDebug().noquote() << "DATA: " + data;
    qDebug() << ">>>";
#endif

    DBInterface::instance()->setObjectData(m_uuid, objectTypeName(), data);

    m_savingData = false;

    emitDataChanged();
}

QString RamAbstractObject::dataString() const
{
    // If we have cached the data already, return it
    if (m_cachedData != "" && CACHE_RAMOBJECT_DATA) return m_cachedData;

    if (!m_created) return m_cachedData;

    QString dataStr = DBInterface::instance()->objectData(m_uuid, objectTypeName());
    if (dataStr == "") return "";

    // Cache the data to improve performance
    //m_cachedData = dataStr;

    return dataStr;
}

void RamAbstractObject::createData(QString data)
{
    if (m_virtual || m_saveSuspended) return;

    if (data == "") data = m_cachedData;

    // Cache the data to improve performance
    m_cachedData = data;

    DBInterface::instance()->createObject(m_uuid, objectTypeName(), data);

    m_created = true;
}

bool RamAbstractObject::checkUuid(QString uuid, ObjectType type, bool mayBeVirtual, bool includeRemoved)
{
    QString table = objectTypeName(type);
    if (uuid == "")
        qFatal( "RamAbstractObject::get - getting empty UUID." );
    Q_ASSERT_X(uuid != "", QString("%1::get").arg(table).toUtf8(), "UUID can't be empty");

    if (uuid == "none") return false;

    if (mayBeVirtual) return true;

    // Check if the uuid exists in the DB
    if (!DBInterface::instance()->contains(uuid, table, includeRemoved))
    {
        qCritical() << QString("%1::get - This uuid can't be found in the database: %2").arg(table, uuid);
        // Don't do anything, let the caller handle it
        return false;
    }

    // Check if it's removed
    //if (!includeRemoved && DBInterface::instance()->isRemoved(uuid, table)) return false;

    return true;
}

QSet<RamAbstractObject *> RamAbstractObject::invalidObjects()
{
    return m_invalidObjects;
}

void RamAbstractObject::removeInvalidObjects()
{
    foreach(RamAbstractObject *o, m_invalidObjects)
    {
        o->remove();
    }
    m_invalidObjects.clear();
}

QPixmap RamAbstractObject::iconPixmap(QString iconName)
{
    if (m_iconPixmaps.isEmpty())
    {
        m_iconPixmaps = QHash<QString, QPixmap>({
                                               {":/icons/asset", DuIcon(":/icons/asset").pixmap(QSize(12,12))},
                                               {":/icons/application", DuIcon(":/icons/application").pixmap(QSize(12,12))},
                                               {":/icons/asset-group", DuIcon(":/icons/asset-group").pixmap(QSize(12,12))},
                                               {":/icons/file", DuIcon(":/icons/file").pixmap(QSize(12,12))},
                                               {":/icons/connection", DuIcon(":/icons/connection").pixmap(QSize(12,12))},
                                               {":/icons/project", DuIcon(":/icons/project").pixmap(QSize(12,12))},
                                               {":/icons/calendar", DuIcon(":/icons/calendar").pixmap(QSize(12,12))},
                                               {":/icons/sequence", DuIcon(":/icons/sequence").pixmap(QSize(12,12))},
                                               {":/icons/shot", DuIcon(":/icons/shot").pixmap(QSize(12,12))},
                                               {":/icons/state-l", DuIcon(":/icons/state-l").pixmap(QSize(12,12))},
                                               {":/icons/status", DuIcon(":/icons/status").pixmap(QSize(12,12))},
                                               {":/icons/film", DuIcon(":/icons/film").pixmap(QSize(12,12))},
                                               {":/icons/step", DuIcon(":/icons/step").pixmap(QSize(12,12))},
                                               {":/icons/admin", DuIcon(":/icons/admin").pixmap(QSize(12,12))},
                                               {":/icons/project-admin", DuIcon(":/icons/project-admin").pixmap(QSize(12,12))},
                                               {":/icons/lead", DuIcon(":/icons/lead").pixmap(QSize(12,12))},
                                               {":/icons/user", DuIcon(":/icons/user").pixmap(QSize(12,12))}
                                                });
    }
    return m_iconPixmaps.value(iconName, QPixmap());
}

QRegularExpression RamAbstractObject::shortNameRegularExpression()
{
    if (m_rxsn.pattern() != "") return m_rxsn;
    m_rxsn = RegExUtils::getRegularExpression("shortname", "", "", true);
    return m_rxsn;
}

void RamAbstractObject::construct()
{
    m_allObjects[m_uuid] = this;
}
