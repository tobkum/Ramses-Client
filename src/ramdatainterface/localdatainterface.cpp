#include "localdatainterface.h"

#include "datacrypto.h"

LocalDataInterface *LocalDataInterface::_instance = nullptr;

LocalDataInterface *LocalDataInterface::instance()
{
    if (!_instance) _instance = new LocalDataInterface();
    return _instance;
}

QString LocalDataInterface::login(QString username, QString password) const
{
    // Get user data
    QString q = "SELECT data, uuid FROM RamUser WHERE userName = '%1';";
    QSqlQuery qry = query( q.arg(username) );

    if(!qry.first()) return "";

    // Decrypt and check password
    QString data = qry.value(0).toString();

    if (ENCRYPT_USER_DATA) data = DataCrypto::instance()->clientDecrypt(data);

    QJsonDocument doc = QJsonDocument::fromJson( data.toUtf8() );
    QJsonObject dataObj = doc.object();

    QString test = dataObj.value("password").toString();

    if ( password == test )
    {
        // Save the current DB to recent files
        QSettings settings;
        // Get all to remove non existing, and insert new at first
        QStringList dbs;
        int n = settings.beginReadArray("database/recent");
        for (int i = 0; i < n; i++)
        {
            settings.setArrayIndex(i);
            dbs << settings.value("path", "-").toString();
        }
        settings.endArray();
        // Check
        for (int i = dbs.count() - 1; i >= 0; i--)
        {
            if (!QFileInfo::exists(dbs.at(i)))
            {
                dbs.removeAt(i);
                continue;
            }
            if (dbs.at(i) == m_dataFile) dbs.removeAt(i);
        }
        // Insert
        dbs.insert(0, m_dataFile);
        // Write
        settings.beginWriteArray("database/recent");
        for (int i = 0; i < dbs.count(); i++)
        {
            settings.setArrayIndex(i);
            settings.setValue("path", dbs.at(i));
        }
        settings.endArray();

        return qry.value(1).toString();
    }

    return "";
}

void LocalDataInterface::setRamsesPath(QString p) const
{
    // Keep history
    QString q = "UPDATE Settings SET current = 0 WHERE current = 1;";
    query( q );

    // Add new
    q = "INSERT INTO Settings (localDataPath, current) VALUES  ( '%1', 1);";
    query( q.arg(p) );

}

QStringList LocalDataInterface::tableData(QString table) const
{
    QString q = "SELECT uuid FROM '%1' WHERE removed = 0;";
    QSqlQuery qry = query( q.arg(table) );

    QStringList data;

    while (qry.next()) data << qry.value(0).toString();

    return data;
}

void LocalDataInterface::createObject(QString uuid, QString table, QString data) const
{
    QDateTime modified = QDateTime::currentDateTimeUtc();

    QString q = "INSERT INTO '%1' (uuid, data, modified, removed)"
                "VALUES ( '%2' , '%3' , '%4' , 0);";

    query( q.arg(
                  table,
                  uuid,
                  data,
                  modified.toString("yyyy-MM-dd hh:mm:ss:zzz")
                  )
            );
}

QString LocalDataInterface::objectData(QString uuid, QString table) const
{
    QString q = "SELECT data FROM %1 WHERE uuid = '%2';";
    QSqlQuery qry = query( q.arg(table, uuid) );

    if (qry.first()) return qry.value(0).toString();
    return "";
}

void LocalDataInterface::setObjectData(QString uuid, QString table, QString data) const
{
    QDateTime modified = QDateTime::currentDateTimeUtc();

    data = data.replace("'", "\\'");

    QString q = "UPDATE %1 SET data = '%2', modified = '%3' WHERE uuid = '%4';";
    query( q.arg(table, data, modified.toString("yyyy-MM-dd hh:mm:ss:zzz"), uuid) );
}

void LocalDataInterface::removeObject(QString uuid, QString table) const
{
    QString q = "UPDATE %1 SET removed = 0 WHERE uuid = '%2';";
    query( q.arg(table, uuid) );
}

void LocalDataInterface::restoreObject(QString uuid, QString table) const
{
    QString q = "UPDATE %1 SET removed = 1 WHERE uuid = '%2';";
    query( q.arg(table, uuid) );
}

bool LocalDataInterface::isRemoved(QString uuid, QString table) const
{
    QString q = "SELECT removed FROM %1 WHERE uuid = '%2';";
    QSqlQuery qry = query( q.arg(table, uuid) );

    if (qry.first())
    {
        return qry.value(0).toBool();
    }

    return true;
}

void LocalDataInterface::setUsername(QString uuid, QString username) const
{
    QString q = "UPDATE RamUser SET userName = '%1' WHERE uuid = '%2';";
    query( q.arg(username, uuid) );
}

ServerConfig LocalDataInterface::serverConfig() const
{
    QString q = "SELECT address, useSsl, updateDelay, timeout FROM RamServer;";
    QSqlQuery qry = query( q );

    ServerConfig config;
    if (qry.first()) {
        config.address = qry.value(0).toString();
        config.useSsl = qry.value(1).toBool();
        config.updateDelay = qry.value(2).toInt();
        config.timeout = qry.value(3).toInt();
    }

    return config;
}

const QString &LocalDataInterface::dataFile() const
{
    return m_dataFile;
}

ServerConfig LocalDataInterface::setDataFile(const QString &file)
{
    QSqlDatabase db = QSqlDatabase::database("localdata");
    // Set the SQLite file
    db.close();
    // Open
    db.setDatabaseName(file);
    if (!db.open()) log("Can't save data to the disk.", DuQFLog::Fatal);

    m_dataFile = file;

    return serverConfig();
}

LocalDataInterface::LocalDataInterface()
{
    //Load local database
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","localdata");
    db.setHostName("localhost");
}

QSqlQuery LocalDataInterface::query(QString q) const
{
    QSqlDatabase db = QSqlDatabase::database("localdata");
    QSqlQuery qry = QSqlQuery(db);

    log(tr("Querying:") + "\n" + q, DuQFLog::Data);

    if (!qry.exec(q))
    {
        QString errorMessage = "Something went wrong when saving the data.\nHere's some information:";
        errorMessage += "\n> " + tr("Query:") + "\n" + qry.lastQuery();
        errorMessage += "\n> " + tr("Database Error:") + "\n" + qry.lastError().databaseText();
        errorMessage += "\n> " + tr("Driver Error:") + "\n" + qry.lastError().driverText();
        log(errorMessage, DuQFLog::Critical);
    }

    return qry;
}
