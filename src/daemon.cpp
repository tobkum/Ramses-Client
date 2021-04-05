#include "daemon.h"

Daemon *Daemon::_instance = nullptr;

Daemon* Daemon::instance()
{
    if (!_instance) _instance = new Daemon();
    return _instance;
}

void Daemon::start()
{
    if (!_tcpServer->listen( QHostAddress::LocalHost, _settings.value("daemon/port", 18185).toInt() )) {
        qDebug() << _tcpServer->errorString();
        log("Unable to start the daemon server.\n" + _tcpServer->errorString(), DuQFLog::Warning);
    }
    else
    {
        qDebug() << "Daemon started and listening on port " + QString::number(_tcpServer->serverPort());
        log("Daemon started and listening on port " + QString::number(_tcpServer->serverPort()), DuQFLog::Information);
    }
}

void Daemon::stop()
{
    _tcpServer->close();
    log("Daemon stopped.", DuQFLog::Information);
}

void Daemon::restart()
{
    stop();
    start();
}

void Daemon::newConnection()
{
    QTcpSocket *client = _tcpServer->nextPendingConnection();
    connect(client, &QAbstractSocket::disconnected, client, &QObject::deleteLater);
    connect(client, &QAbstractSocket::readyRead, this, &Daemon::reply);
}

void Daemon::reply()
{
    QTcpSocket *client = (QTcpSocket*)sender();
    QString request = client->readAll();
    //split args
    QStringList requestArgs = request.split("&");
    log("I've got these args: \n" + requestArgs.join("\n"), DuQFLog::Debug);

    //Parse
    QMap<QString, QString> args;
    for(const QString &arg: qAsConst(requestArgs))
    {
        QStringList aList = arg.split("=");
        if (aList.count() > 1)
        {
            args[aList.at(0)] = aList.at(1);
        }
        else
        {
            args[aList.at(0)] = "";
        }
    }

    //Read
    if (args.contains("ping"))
        ping(client);
    else if (args.contains("raise"))
        emit raise();
    else if (args.contains("setCurrentProject"))
        setCurrentProject(args.value("shortName"), args.value("name"), client);
    else
        post(client, QJsonObject(), "", "Unknown query.", false, false);

}

void Daemon::ping(QTcpSocket *client)
{
    log("I'm replying to this request: ping", DuQFLog::Information);

    QJsonObject content;
    content.insert("version", STR_VERSION);
    content.insert("ramses", STR_INTERNALNAME);
    post(client, content, "ping","Hi, this is the Ramses Daemon");
}

void Daemon::setCurrentProject(QString shortName, QString name, QTcpSocket *client)
{
    log("I'm replying to this request: setCurrentProject: " + shortName, DuQFLog::Information);

    Ramses::instance()->setCurrentProject(shortName, name);

    QJsonObject content;
    RamProject *p = Ramses::instance()->currentProject();
    if (p)
    {
        content.insert("name", p->name());
        content.insert("shortName", p->shortName());
        content.insert("path", p->folderPath());
        content.insert("uuid", p->uuid());
        post(client, content, "setCurrentProject", "Current project set to: " + p->name());
    }
    else
    {
        content.insert("name", "");
        content.insert("shortName", "");
        content.insert("path", "");
        content.insert("uuid", "");
        post(client, content, "setCurrentProject", "Project " + shortName + " not found, sorry!", false);
    }
}

Daemon::Daemon(QObject *parent) : DuQFLoggerObject("Daemon", parent)
{
    _tcpServer = new QTcpServer(this);

    start();

    connect(_tcpServer, &QTcpServer::newConnection, this, &Daemon::newConnection);
}

void Daemon::post(QTcpSocket *client, QJsonObject content, QString query, QString message, bool success, bool accepted)
{
    QJsonObject obj;
    obj.insert("query", query);
    obj.insert("message", message);
    obj.insert("accepted", accepted);
    obj.insert("success", success);
    obj.insert("content", content);
    QJsonDocument json(obj);

    QString jsonReply = json.toJson();
    client->write( jsonReply.toUtf8() );

    log("Posting:\n" + jsonReply, DuQFLog::Debug);
}
