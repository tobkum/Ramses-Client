#include "localdatainterface.h"

#include "datacrypto.h"

// QUERIER

Querier::Querier(QString dbName): DuQFLoggerObject("Local Data Interface Querier")
{
    m_dbName = dbName;
}

void Querier::setDataFile(QString f)
{
    m_dataFile = f;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_dbName);
    db.setHostName("localhost");
    db.setDatabaseName(m_dataFile);
    if (!db.open())
    {
        emit error("Can't access data from the disk.");
    }
}

QSqlQuery Querier::query(QString q)
{
    if (m_dataFile =="") return QSqlQuery();

    //Load local database
    QSqlDatabase db = QSqlDatabase::database(m_dbName);
    QSqlQuery qry = QSqlQuery(db);

    //log(tr("Querying:") + "\n" + q, DuQFLog::Data);
    //qDebug() << q;

    if (!qry.exec(q))
    {
        QString errorMessage = "Something went wrong when %1 the data.\nHere's some information:";
        if (m_dbName == "getter") errorMessage = errorMessage.arg("getting");
        else errorMessage = errorMessage.arg("saving");
        errorMessage += "\n> " + tr("Query:") + "\n" + qry.lastQuery();
        errorMessage += "\n> " + tr("Database Error:") + "\n" + qry.lastError().databaseText();
        errorMessage += "\n> " + tr("Driver Error:") + "\n" + qry.lastError().driverText();
        emit error(errorMessage);
        return QSqlQuery();
    }

    emit ready(qry);
    return qry;
}

// INTERFACE

LocalDataInterface *LocalDataInterface::_instance = nullptr;

LocalDataInterface *LocalDataInterface::instance()
{
    if (!_instance) _instance = new LocalDataInterface();
    return _instance;
}

void LocalDataInterface::setServerSettings(QString dbFile, ServerConfig c)
{
    // Make sure the interface is ready
    LocalDataInterface::instance();

    QSqlDatabase db = QSqlDatabase::database("editdb");
    // Set the SQLite file
    db.close();
    // Open
    db.setDatabaseName(dbFile);
    if (!db.open()) LocalDataInterface::instance()->log("Can't save data to the disk.", DuQFLog::Fatal);

    // Remove previous settings
    QSqlQuery qry = QSqlQuery(db);
    if (!qry.exec("DELETE FROM RamServer;"))
    {
        QString errorMessage = "Something went wrong when saving the data.\nHere's some information:";
        errorMessage += "\n> " + tr("Query:") + "\n" + qry.lastQuery();
        errorMessage += "\n> " + tr("Database Error:") + "\n" + qry.lastError().databaseText();
        errorMessage += "\n> " + tr("Driver Error:") + "\n" + qry.lastError().driverText();
        LocalDataInterface::instance()->log(errorMessage, DuQFLog::Critical);

        db.close();
        return;
    }

    // Add new settings
    QString q = "INSERT INTO RamServer (address, useSsl, updateDelay, timeout) "
            "VALUES ('%1', %2, %3, %4)";

    QString useSsl = "1";
    if (!c.useSsl) useSsl = "0";
    if (!qry.exec(q.arg(c.address, useSsl, QString::number(c.updateDelay), QString::number(c.timeout))))
    {
        QString errorMessage = "Something went wrong when saving the data.\nHere's some information:";
        errorMessage += "\n> " + tr("Query:") + "\n" + qry.lastQuery();
        errorMessage += "\n> " + tr("Database Error:") + "\n" + qry.lastError().databaseText();
        errorMessage += "\n> " + tr("Driver Error:") + "\n" + qry.lastError().driverText();
        LocalDataInterface::instance()->log(errorMessage, DuQFLog::Critical);
    }

    db.close();
}

ServerConfig LocalDataInterface::getServerSettings(QString dbFile)
{
    // Make sure the interface is ready
    LocalDataInterface::instance();

    QSqlDatabase db = QSqlDatabase::database("editdb");
    // Set the SQLite file
    db.close();
    // Open
    db.setDatabaseName(dbFile);
    if (!db.open()) LocalDataInterface::instance()->log("Can't save data to the disk.", DuQFLog::Fatal);

    // Get settings
    QSqlQuery qry = QSqlQuery(db);

    if (!qry.exec("SELECT address, useSsl, updateDelay, timeout FROM RamServer;"))
    {
        QString errorMessage = "Something went wrong when saving the data.\nHere's some information:";
        errorMessage += "\n> " + tr("Query:") + "\n" + qry.lastQuery();
        errorMessage += "\n> " + tr("Database Error:") + "\n" + qry.lastError().databaseText();
        errorMessage += "\n> " + tr("Driver Error:") + "\n" + qry.lastError().driverText();
        LocalDataInterface::instance()->log(errorMessage, DuQFLog::Critical);

        db.close();
        return ServerConfig();
    }

    ServerConfig s;

    if (qry.first())
    {
        s.address = qry.value(0).toString();
        s.useSsl = qry.value(1).toBool();
        s.updateDelay = qry.value(2).toInt();
        s.timeout = qry.value(3).toInt();
    }

    db.close();

    return s;
}

QString LocalDataInterface::login(QString username, QString password) const
{
    username.replace("'", "''");

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

void LocalDataInterface::setRamsesPath(QString p)
{
    p.replace("'", "''");

    // Keep history
    QString q = "UPDATE Settings SET current = 0 WHERE current = 1;";
    query( q );

    // Add new
    q = "INSERT INTO Settings (localDataPath, current) VALUES  ( '%1', 1);";
    threadedQuery( q.arg(p) );

}

QStringList LocalDataInterface::tableData(QString table) const
{
    QString q = "SELECT uuid FROM '%1' WHERE removed = 0;";
    QSqlQuery qry = query( q.arg(table) );

    QStringList data;

    while (qry.next()) data << qry.value(0).toString();

    return data;
}

bool LocalDataInterface::contains(QString uuid, QString table) const
{
    QString q = "SELECT uuid FROM '%1' WHERE uuid = '%2' AND removed = 0;";
    q = q.arg(table, uuid);
    QSqlQuery qry = query( q );

    if (qry.first() && qry.value(0) != "") return true;
    return false;
}

void LocalDataInterface::createObject(QString uuid, QString table, QString data)
{
    data.replace("'", "''");

    QDateTime modified = QDateTime::currentDateTimeUtc();

    QString q = "INSERT INTO '%1' (uuid, data, modified, removed) "
                "VALUES ('%2', '%3', '%4', 0) "
                "ON CONFLICT(uuid) DO UPDATE "
                "SET data=excluded.data, modified=excluded.modified ;";

    threadedQuery( q.arg(
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

void LocalDataInterface::setObjectData(QString uuid, QString table, QString data)
{
    data.replace("'", "''");

    QDateTime modified = QDateTime::currentDateTimeUtc();


    QString q = "INSERT INTO %1 (data, modified, uuid) "
                "VALUES ( '%2', '%3', '%4') "
                "ON CONFLICT(uuid) DO UPDATE "
                "SET data=excluded.data, modified=excluded.modified ;";

    threadedQuery( q.arg(table, data, modified.toString("yyyy-MM-dd hh:mm:ss:zzz"), uuid) );
}

void LocalDataInterface::removeObject(QString uuid, QString table)
{
    QString q = "UPDATE %1 SET removed = 1 WHERE uuid = '%2';";
    threadedQuery( q.arg(table, uuid) );
}

void LocalDataInterface::restoreObject(QString uuid, QString table)
{
    QString q = "UPDATE %1 SET removed = 0 WHERE uuid = '%2';";
    threadedQuery( q.arg(table, uuid) );
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

void LocalDataInterface::setUsername(QString uuid, QString username)
{
    username.replace("'", "''");

    QDateTime modified = QDateTime::currentDateTimeUtc();

    QString q = "INSERT INTO RamUser (userName, modified, uuid) "
                "VALUES ('%1', '%2', '%3') "
                "ON CONFLICT(uuid) DO UPDATE "
                "SET userName=excluded.userName, modified=excluded.modified ;";
    threadedQuery( q.arg(username, modified.toString("yyyy-MM-dd hh:mm:ss:zzz"), uuid) );
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
    /*QSqlDatabase db = QSqlDatabase::database("localdata");
    // Set the SQLite file
    db.close();
    // Open
    db.setDatabaseName(file);
    if (!db.open()) log("Can't save data to the disk.", DuQFLog::Fatal);*/

    m_querier->setDataFile(file);
    emit newDataFile(file);

    m_dataFile = file;

    emit dataReset();

    return serverConfig();
}

void LocalDataInterface::logError(QString err)
{
    log(err, DuQFLog::Critical);
}

LocalDataInterface::LocalDataInterface() :
    DuQFLoggerObject("Local Data Interface")
{
    m_querier = new Querier("getter");
    m_tQuerier = new Querier("setter");
    m_tQuerier->moveToThread(&m_queryThread);

    connect(m_querier, &Querier::error, this, &LocalDataInterface::logError);
    connect(m_tQuerier, &Querier::error, this, &LocalDataInterface::logError);
    connect(this, &LocalDataInterface::newQuery, m_tQuerier, &Querier::query);
    connect(this, &LocalDataInterface::newDataFile, m_tQuerier, &Querier::setDataFile);

    QSqlDatabase editdb = QSqlDatabase::addDatabase("QSQLITE","editdb");
    editdb.setHostName("localhost");

    m_queryThread.start();
}

QSqlQuery LocalDataInterface::query(QString q) const
{
    return m_querier->query(q);
}

void LocalDataInterface::threadedQuery(QString q)
{
    emit newQuery(q);
}

