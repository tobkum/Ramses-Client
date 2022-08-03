#ifndef RAMABSTRACTOBJECT_H
#define RAMABSTRACTOBJECT_H

#include <QSettings>

/**
 * @brief The RamAbstractObject class is the base class for RamObject and RamObjectList
 */
class RamAbstractObject
{
public:

    // ENUMS //

    /**
     * @brief The ObjectType enum lists all types of RamObjects
     */
    enum ObjectType { Application,
                    Asset,
                    AssetGroup,
                    FileType,
                    Object,
                    Item,
                    ItemTable,
                    Pipe,
                    PipeFile,
                    Project,
                    Sequence,
                    Shot,
                    State,
                    Status,
                    Step,
                    User,
                    ObjectList,
                    StepStatusHistory,
                    ScheduleEntry,
                    ScheduleComment };
    /**
     * @brief The SubFolder enum lists all predefined subfolders
     */
    enum SubFolder { NoFolder,
                   ConfigFolder,
                   AdminFolder,
                   PreProdFolder,
                   ProdFolder,
                   PostProdFolder,
                   AssetsFolder,
                   ShotsFolder,
                   ExportFolder,
                   TemplatesFolder,
                   PublishFolder,
                   VersionsFolder,
                   PreviewFolder,
                   UsersFolder,
                   ProjectsFolder,
                   TrashFolder,
                   DataFolder };
    /**
     * @brief The UserRole enum lists the available roles for users, and the level of their rights
     */
    enum UserRole { Admin = 3,
                    ProjectAdmin = 2,
                    Lead = 1,
                    Standard = 0 };

    // STATIC METHODS //

    /**
     * @brief objectTypeName gets the name of a type, as used in the Database and API classes
     * @param type
     * @return
     */
    static const QString objectTypeName(ObjectType type);

    /**
     * @brief subFolderName gets the actual name of a subfolder
     * @param folder
     * @return
     */
    static const QString subFolderName(SubFolder folder);

    /**
     * @brief object Gets the existing object
     * @param uuid The object uuid
     * @return the object or nullptr if it doesn't exist yet
     */
    static RamAbstractObject *getObject(QString uuid);

    /**
     * @brief objFromName gets an object using its name or shortname
     * @param shortNameOrName
     * @param objType
     * @return
     */
    static RamAbstractObject *getObject(QString shortNameOrName , ObjectType objType);


    // METHODS //

    RamAbstractObject(QString shortName, QString name, ObjectType type);
    ~RamAbstractObject();

    bool is(RamAbstractObject *other) const;

    /**
     * @brief uuid is this object's uuid
     * @return
     */
    QString uuid() const;

    /**
     * @brief objectType the type of ramobject
     * @return
     */
    ObjectType objectType() const;
    QString objectTypeName() const;

    /**
     * @brief data gets the data from the database
     * @return
     */
    virtual QJsonObject data() const;
    /**
     * @brief getData returns the data for a specific key
     * @return
     */
    QJsonValue getData(QString key) const;
    /**
     * @brief setData sets an entirely new data
     * @param data
     */
    virtual void setData(QJsonObject data);
    /**
     * @brief insertData inserts or updates a value in the data
     * @param key
     * @param value
     */
    void insertData(QString key, QJsonValue value);

    /**
     * @brief shortName the identifier of the object
     * @return
     */
    virtual QString shortName() const;
    void setShortName(const QString &shortName);

    /**
     * @brief name the user-friendly name of the object
     * @return
     */
    virtual QString name() const;
    void setName(const QString &name);

    /**
     * @brief comment is a comment associated to this object. Plain text or markdown
     * @return
     */
    QString comment() const;
    void setComment(const QString comment);

    /**
     * @brief remove marks the object as removed in the database
     */
    virtual void remove();
    /**
     * @brief restore markes the object as available in the database
     */
    void restore();
    bool removed();

    /**
     * @brief settings are the settings corresponding to this object
     * @return
     */
    QSettings *settings();
    void reInitSettingsFile();

    QString path(SubFolder subFolder = NoFolder, bool create = false) const;
    QString path(SubFolder subFolder, QString subPath, bool create = false) const;
    QStringList listFiles(SubFolder subFolder = NoFolder, QString subPath = "") const;
    QList<QFileInfo> listFileInfos(SubFolder subFolder = NoFolder, QString subPath = "") const;
    QStringList listFolders(SubFolder subFolder = NoFolder) const;
    QStringList listFolders(SubFolder subFolder, QString subPath) const;
    void deleteFile(QString fileName, SubFolder folder=NoFolder) const;
    void revealFolder(SubFolder subFolder = NoFolder);

protected:

    // METHODS //

    /**
     * @brief RamAbstractObject constructs an existing object from an existing uuid. This method is private, use obj(QString) to get an object from the uuid
     * @param uuid
     */
    RamAbstractObject(QString uuid, ObjectType type);

    // Low level data handling.
    virtual QString dataString() const;
    virtual void setDataString(QString data);
    virtual void createData(QString data);

    virtual void emitDataChanged(QJsonObject data) { Q_UNUSED(data) };
    virtual void emitRemoved() {};
    virtual void emitRestored() {};

    /**
     * @brief folderPath the folder of this object
     * @return
     */
    virtual QString folderPath() const { return QString(); };

    // STATIC ATTRIBUTES //

    /**
     * @brief m_existingObjects is the list of all objects, used to get them from uuid.
     */
    static QMap<QString, RamAbstractObject*> m_existingObjects;

    // ATTRIBUTES //

    QString m_uuid;
    ObjectType m_objectType;

private:
    QSettings *m_settings = nullptr;
};

#endif // RAMABSTRACTOBJECT_H
