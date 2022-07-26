#include "ramserverinterface.h"

#include "duqf-app/app-version.h"

RamServerInterface RamServerInterface::*_instance = nullptr;

RamServerInterface *RamServerInterface::instance()
{
    if (!_instance) _instance = new RamServerInterface();
    return _instance;
}

bool RamServerInterface::ssl() const
{
    return m_ssl;
}

void RamServerInterface::setSsl(bool useSsl)
{
    if (m_ssl == useSsl) return;

    if (useSsl && !QSslSocket::supportsSsl()) {
        log("SSL is not available on this system. Please install OpenSSL to securely connect to the specified server.", DuQFLog::Critical);
        m_ssl = false;
    }
    else
    {
        m_ssl = useSsl;
    }
    emit sslChanged(m_ssl);
}

const QString &RamServerInterface::serverVersion() const
{
    return m_serverVersion;
}

void RamServerInterface::post(QString query, QJsonObject data)
{
    queueRequest(query, data);
}

const QString &RamServerInterface::serverAddress() const
{
    return m_serverAddress;
}

const QString RamServerInterface::serverProtocol()
{
    if (m_ssl)
    {
        if (!QSslSocket::supportsSsl()) {
            log("SSL is not available on this system. Please install OpenSSL to securely connect to the specified server.", DuQFLog::Critical);
            return "http://";
        }
        else return "https://";
    }

    return "http://";
}

void RamServerInterface::setOnline()
{
    setConnectionStatus(NetworkUtils::Connecting, "Server ping");
    Request r = buildRequest("ping", QJsonObject());
    postRequest(r);
    m_pingTimer->start(m_pingDelay);
}

void RamServerInterface::setOffline()
{
    setConnectionStatus(NetworkUtils::Offline, "Switched to offline mode.");
}

void RamServerInterface::setServerAddress(const QString &newServerAddress)
{
    if (newServerAddress == m_serverAddress) return;
    m_serverAddress = newServerAddress;
}

void RamServerInterface::nextRequest()
{
    if (m_requestQueue.isEmpty())
    {
        m_requestQueueTimer->stop();
        return;
    }

    if (!waitPing()) {
        m_requestQueueTimer->stop();
        return; // Return now if we're offline
    }

    postRequest( m_requestQueue.takeFirst() );
}

void RamServerInterface::checkConnection()
{
    waitPing(true);
}

void RamServerInterface::setConnectionStatus(NetworkUtils::NetworkStatus s, QString reason)
{
    // Check security
    if (s != m_status && s == NetworkUtils::Online && !m_ssl) log("Connection is not secured!", DuQFLog::Critical);

    // update status
    m_status = s;
    // remove token if offline
    if (s == NetworkUtils::Offline) m_sessionToken = "";

    // log and emit signal
    if (s == NetworkUtils::Offline) log("Disconnected: " + reason);
    else if (s == NetworkUtils::Connecting) log("Connecting: " + reason);
    else if (s == NetworkUtils::Online) log("Connected: " + reason);
    else if (s == NetworkUtils::Error) log("Connection error: " + reason);

    emit connectionStatusChanged(s, reason);
}

void RamServerInterface::dataReceived(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) return;

    QString repAll = reply->readAll();

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

    log(repQuery + "\n" + repMessage + "\nContent:\n" + repAll, DuQFLog::Data);

    if (!repSuccess || repMessage.toLower().startsWith("warning"))
    {
        log(repMessage, DuQFLog::Warning);
    }
    else
    {
        log(repMessage, DuQFLog::Information);
    }

    //if we recieved the reply from the ping, set online and restarts the queue
    if (repQuery == "ping")
    {
        if (repSuccess)
        {
            setConnectionStatus(NetworkUtils::Online, "Server ready");
            startQueue();
        }
        else
        {
            setConnectionStatus(NetworkUtils::Error, "The server request was not successful.\nThis is probably a bug or a configuration error.\nPlease report bugs on " + QString(URL_SOURCECODE));
        }

        // get the new token
        m_sessionToken = repObj.value("token").toString();

        // get the server version
        m_serverVersion = repObj.value("content").toObject().value("version").toString();
    }


    if (repQuery == "loggedout")
    {
        setConnectionStatus(NetworkUtils::Offline, "The server ended your session.");
    }

    emit data(repObj);
}

void RamServerInterface::networkError(QNetworkReply::NetworkError err)
{
    QString reason = "";
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
    setConnectionStatus( NetworkUtils::Offline, "Network error:\n" + reason);
    log(reason, DuQFLog::Critical);
}

void RamServerInterface::sslError(QNetworkReply *reply, QList<QSslError> errs)
{
    Q_UNUSED(reply);

    QString errors;
    foreach (QSslError err, errs)
    {
        log(err.errorString(), DuQFLog::Warning);
        errors = errors + "\n> " + err.errorString();
    }
    log("SSL Error. Connection may not be secured.", DuQFLog::Warning);
    setConnectionStatus( NetworkUtils::Offline, "There were SSL errors, the connection may not be secured.\n" + errors );
}

void RamServerInterface::flushRequests()
{
    if ( m_requestQueue.isEmpty() ) return;

    log( "Flushing remaining requests." );

    m_requestQueueTimer->stop();

    // Post the remaining requests
    while( !m_requestQueue.isEmpty() )
    {
        nextRequest();
        qApp->processEvents();
        // Wait a small bit
        QThread::msleep( m_requestDelay / 4 );
    }

    // Wait a last bit to be sure everything is sent
    QThread::msleep( m_requestDelay );
    qApp->processEvents();

    log( "All requests sent." );
}

RamServerInterface::RamServerInterface():
    DuQFLoggerObject("Ramses Server Interface", nullptr)
{
    // Prepare network connection
    m_network.setCookieJar(new QNetworkCookieJar());
    m_network.setStrictTransportSecurityEnabled(true);

    m_forbiddenWords << "and" << "or" << "if" << "else" << "insert" << "update" << "select" << "drop" << "alter";

    m_requestQueueTimer = new QTimer(this);
    m_pingTimer = new QTimer(this);
}

void RamServerInterface::connectEvents()
{
    connect(m_requestQueueTimer, SIGNAL(timeout()), this, SLOT(nextRequest()));
    connect(m_pingTimer, SIGNAL(timeout()), this, SLOT(checkConnection()));
    connect(&m_network, SIGNAL(finished(QNetworkReply*)), this, SLOT(dataReceived(QNetworkReply*)));
    connect(&m_network, SIGNAL(sslErrors(QNetworkReply*,QList)), this, SLOT(sslError(QNetworkReply*,QList)));
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(flushRequests()));
}

bool RamServerInterface::waitPing(bool force)
{
    // If offline, we need to get online
    if (m_status == NetworkUtils::Offline || force) setOnline();
    //wait three seconds when connecting or set offline
    int timeout = QSettings().value("server/timeout", 3000).toInt();
    QDeadlineTimer t(timeout);
    while (m_status != NetworkUtils::Online)
    {
        qApp->processEvents();
        if ( t.hasExpired() || m_status == NetworkUtils::Offline )
        {
            setOffline();
            log("Cannot process request, server unavailable.", DuQFLog::Critical);
            return false;
        }
    }
    return true;
}

void RamServerInterface::postRequest(Request r)
{
    QNetworkReply *reply = m_network.post(r.request, r.body.toUtf8());
    QUrl url = r.request.url();

    // Log URL / GET
    log( "New request: " +  url.toString(QUrl::RemovePassword), DuQFLog::Debug);
    // Log POST body
    if (r.query == "login")
        #ifdef QT_DEBUG
        log("Request data: " + r.body, DuQFLog::Data);
        #else
        log("Request data: [Hidden login info]", DuQFLog::Data);
        #endif
    else
        log("Request data: " + r.body, DuQFLog::Data);

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,SLOT(networkError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));

    emit queried(r.query);
}

Request RamServerInterface::buildRequest(QString query, QJsonObject body)
{
    Request r = buildRequest(query);

    // Update header to JSON type
    r.request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Post body
    // Add version
    body.insert("version", STR_VERSION);
    // Current token
    if (m_sessionToken != "") body.insert("token", m_sessionToken );
    // Build body
    QJsonDocument bodyDoc;
    bodyDoc.setObject(body);
    r.body = bodyDoc.toJson(QJsonDocument::Compact);

    return r;
}

Request RamServerInterface::buildRequest(QString query, QStringList args)
{
    Request r = buildRequest(query);
    // Update header to URL Encoded form type
    r.request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // Post body
    // Add version
    args << "version=" + QString(STR_VERSION);
    // Current token
    if (m_sessionToken != "") args << "token=" + m_sessionToken;
    // Build body
    QString body = buildFormEncodedString(args);

    r.body = body;

    return r;
}

Request RamServerInterface::buildRequest(QString query)
{
    // Get info to build the URL
    QString address = serverAddress();
    QString protocol = serverProtocol();

    QUrl url(protocol % address % "?" % query);

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString(STR_INTERNALNAME) + " v" + QString(STR_VERSION));

    Request r;
    r.request = request;
    r.query = query;
    return r;
}

void RamServerInterface::queueRequest(QString query, QJsonObject body)
{
    Request r = buildRequest(query, body);
    queueRequest(r);
}

void RamServerInterface::queueRequest(QString query, QStringList args)
{
    Request r = buildRequest(query, args);
    queueRequest(r);
}

void RamServerInterface::queueRequest(Request r)
{
    m_requestQueue << r;
    startQueue();
}

void RamServerInterface::startQueue()
{
    if (m_status != NetworkUtils::Online) return;
    if (!m_requestQueueTimer->isActive()) m_requestQueueTimer->start(m_requestDelay);
}

QString RamServerInterface::buildFormEncodedString(QStringList args)
{
    QString q;
    bool first = true;
    foreach(QString arg, args)
    {
        if (!first) q += "&";
        first = false;
        // Replace forbidden words
        foreach(QString word, m_forbiddenWords)
        {
            arg = arg.replace(" " + word, "%" + word + "%");
        }
        q += QUrl::toPercentEncoding(arg, "=");
    }
    return q;
}
