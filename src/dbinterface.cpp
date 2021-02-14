#include "dbinterface.h"

DBInterface *DBInterface::_instance = nullptr;

DBInterface *DBInterface::instance()
{
    if (!_instance) _instance = new DBInterface();
    return _instance;
}

void DBInterface::login(QString username, QString password)
{
    //request url
    QStringList q("login");
    q << "username=" + username;
    q << "password=" + generatePassHash(password);

    request(q);
}

void DBInterface::setOffline()
{
    _status = NetworkUtils::Offline;
    _sessionToken = "";
    emit connectionStatusChanged(_status);
}

void DBInterface::setOnline()
{
    //ping
    _status = NetworkUtils::Connecting;
    request("?ping", false);
}

void DBInterface::getUsers()
{
    QString q = "?getUsers";
    request(q);
}

void DBInterface::updateUser(QString uuid, QString shortName, QString name)
{
    QStringList q("updateUser");
    q << "uuid=" + uuid;
    q << "shortName=" + shortName;
    q << "name=" + name;

    request(q);
}

void DBInterface::updateUserPassword(QString uuid, QString c, QString n)
{
    QStringList q("updatePassword");
    q << "uuid=" + uuid;
    q << "current=" + generatePassHash(c);
    q << "new=" + generatePassHash(n);

    request(q);
}

DBInterface::DBInterface(QObject *parent) : QObject(parent)
{
    // LOCAL

    //Load local database
    _localDB = QSqlDatabase::addDatabase("QSQLITE","localdata");

    //TODO create local db etc

    // REMOTE

    _network.setCookieJar(new QNetworkCookieJar());
    _sessionToken = "";

    // Connect events
    connect( &_network, &QNetworkAccessManager::finished, this, &DBInterface::dataReceived);
    connect(&_network, SIGNAL(sslErrors(QNetworkReply *,QList<QSslError>)), this,SLOT(sslError(QNetworkReply *,QList<QSslError>)));


    _status = NetworkUtils::Offline;
}

NetworkUtils::NetworkStatus DBInterface::connectionStatus() const
{
    return _status;
}

void DBInterface::dataReceived(QNetworkReply * rep)
{
    if (rep->error() != QNetworkReply::NoError) return;

    QString repAll = rep->readAll();

    QJsonDocument repDoc = QJsonDocument::fromJson(repAll.toUtf8());
    QJsonObject repObj = repDoc.object();

    if (repObj.isEmpty())
    {
        repObj.insert("message",repAll);
        repObj.insert("accepted",false);
    }

    QString repQuery = repObj.value("query").toString();
    QString repMessage = repObj.value("message").toString();
    bool repSuccess = repObj.value("success").toBool();

    emit log(repQuery + "\n" + repMessage + "\nContent:\n" + repAll, LogUtils::Remote);

    if (!repSuccess)
    {
        emit log(repMessage, LogUtils::Warning);
    }
    else
    {
        emit log(repMessage, LogUtils::Information);
    }

    //if we recieved the reply from the ping, set online
    if (repQuery == "ping" && repSuccess) setConnectionStatus(NetworkUtils::Online);
    else if (repQuery == "ping") setConnectionStatus(NetworkUtils::Offline);

    //if login, get the token
    if (repQuery == "login" && repSuccess) _sessionToken = repObj.value("content").toObject().value("token").toString();
    else if (repQuery == "login") _sessionToken = "";

    emit data(repObj);
}

void DBInterface::sslError(QNetworkReply* /*rep*/, QList<QSslError> errs)
{
    foreach (QSslError err, errs)
    {
        emit log(err.errorString(), LogUtils::Warning);
    }
    emit log("SSL Error. Connection may not be secured.", LogUtils::Warning);
}

void DBInterface::networkError(QNetworkReply::NetworkError err)
{
    QString reason;
    if (err == QNetworkReply::ConnectionRefusedError)
    {
        reason = "The server refused the connection,\nplease try again later.";
    }
    else if (err == QNetworkReply::RemoteHostClosedError)
    {
        reason = "Server unavailable,\nplease try again later.";
    }
    else if (err == QNetworkReply::HostNotFoundError)
    {
        reason = "Invalid server,\ncheck the network settings.";
    }
    else if (err == QNetworkReply::TimeoutError)
    {
        reason = "Server unavailable,\nplease try again later.";
    }
    else if (err == QNetworkReply::OperationCanceledError)
    {
        reason = "Operation canceled.";
    }
    else if (err == QNetworkReply::SslHandshakeFailedError)
    {
        reason = "Secure connection (SSL) unavailable.\nCheck the network settings.";
    }
    else if (err == QNetworkReply::TemporaryNetworkFailureError)
    {
        reason = "Temporary network error,\nplease try again later.";
    }
    else if (err == QNetworkReply::NetworkSessionFailedError)
    {
        reason = "No connection found,\nplease check your network connection.";
    }
    else if (err == QNetworkReply::BackgroundRequestNotAllowedError)
    {
        reason = "Background request not allowed,\nplease check your system firewall settings.";
    }
#if QT_VERSION >= 0x050700
    else if (err == QNetworkReply::TooManyRedirectsError)
    {
        reason = "Too many redirections,\nthis server is misconfigured.";
    }
    else if (err == QNetworkReply::InsecureRedirectError)
    {
        reason = "Insecure redirection.";
    }
#endif
    else if (err == QNetworkReply::ProxyConnectionRefusedError)
    {
        reason = "The proxy refused the connection.";
    }
    else if (err == QNetworkReply::ProxyConnectionClosedError)
    {
        reason = "The proxy closed the connection prematurely.";
    }
    else if (err == QNetworkReply::ProxyNotFoundError)
    {
        reason = "The proxy was not found.\nPlease check your proxy settings.";
    }
    else if (err == QNetworkReply::ProxyTimeoutError)
    {
        reason = "The proxy did not respond in time.\nPlease check your proxy settings.";
    }
    else if (err == QNetworkReply::ProxyAuthenticationRequiredError)
    {
        reason = "The proxy needs authentication.";
    }
    else if (err == QNetworkReply::ContentAccessDenied)
    {
        reason = "The server denied the access (401).";
    }
    else if (err == QNetworkReply::ContentOperationNotPermittedError)
    {
        reason = "Operation not permitted.";
    }
    else if (err == QNetworkReply::ContentNotFoundError)
    {
        reason = "Content not found (404).\nCheck network settings.";
    }
    else if (err == QNetworkReply::AuthenticationRequiredError)
    {
        reason = "The server needs authentication.";
    }
    else if (err == QNetworkReply::ContentReSendError)
    {
        reason = "The request failed.\nPlease try again later.";
    }
    else if (err == QNetworkReply::ContentConflictError)
    {
        reason = "Content conflict.";
    }
    else if (err == QNetworkReply::ContentGoneError)
    {
        reason = "The requested resource is no longer available.";
    }
    else if (err == QNetworkReply::InternalServerError)
    {
        reason = "Internal server error.";
    }
    else if (err == QNetworkReply::OperationNotImplementedError)
    {
        reason = "The server cannot reply (operation not implemented).";
    }
    else if (err == QNetworkReply::ServiceUnavailableError)
    {
        reason = "Service unavailable.\nPlease try again later.";
    }
    else if (err == QNetworkReply::UnknownNetworkError)
    {
        reason = "Unknown network error.";
    }
    else if (err == QNetworkReply::UnknownProxyError)
    {
        reason = "Unknown proxy error.";
    }
    else if (err == QNetworkReply::UnknownContentError)
    {
        reason = "Unknown content error.";
    }
    else if (err == QNetworkReply::ProtocolFailure)
    {
        reason = "Protocol failure.";
    }
    else if (err == QNetworkReply::ProtocolFailure)
    {
        reason = "Unknown server error.";
    }

    setConnectionStatus( NetworkUtils::Offline );
    emit log(reason, LogUtils::Critical);
}

void DBInterface::setConnectionStatus(NetworkUtils::NetworkStatus s)
{
    _status = s;
    emit connectionStatusChanged(s);
}

void DBInterface::request(QString req, bool waitPing)
{
    if (waitPing)
    {
        // If not online or connecting, we need to get online
        if (_status == NetworkUtils::Offline) setOnline();
        //wait three seconds when connecting or set offline
        int timeout = QSettings().value("server/timeout", 3000).toInt();
        QDeadlineTimer t(timeout);
        while (_status != NetworkUtils::Online)
        {
            qApp->processEvents();
            if ( t.hasExpired() || _status == NetworkUtils::Offline )
            {
                setOffline();
                emit log("Cannot process request, server unavailable.", LogUtils::Critical);
                return;
            }
        }
    }

    QSettings settings;

    //Get server address
    QString protocol = "http://";
    if (settings.value("server/ssl", true).toBool()) protocol = "https://";
    QString serverAddress = settings.value("server/address", "localhost/ramses/").toString();

    //add token to the request
    if (_sessionToken != "") req += "&token=" + _sessionToken;

    //request
    QUrl url(protocol + serverAddress + req);
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::UserAgentHeader, QString(STR_INTERNALNAME) + " v" + QString(STR_VERSION));
    _reply = _network.get(request);

    if (req.indexOf("login") >= 0)
    {
#ifdef QT_DEBUG
        emit log("New request: " + protocol + serverAddress + req, LogUtils::Remote);
#else
        emit log("New request: " + protocol + serverAddress + "[Hidden login info]", LogUtils::Remote);
#endif
    }
    else
    {
        emit log("New request: " + protocol + serverAddress + req, LogUtils::Remote);
    }

    connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), this,SLOT(networkError(QNetworkReply::NetworkError)));
}

void DBInterface::request(QStringList args)
{
    QString q = "?" + args.join("&");
    request(q);
}

QString DBInterface::generatePassHash(QString password, QString salt)
{
    //hash password
    QString passToHash = password + salt;
    return QCryptographicHash::hash(passToHash.toUtf8(), QCryptographicHash::Sha3_512).toHex();
}
