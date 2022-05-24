#include "loginpage.h"

LoginPage::LoginPage(QWidget *parent) :
    QWidget(parent)
{
    setupUi();

    ui_capsLockLabel->hide(); // TODO implement CAPS Lock detection

    _ramses = Ramses::instance();
    _failedTimer = new QTimer(this);
    _failedTimer->setSingleShot(true);
    _uiTimer = new QTimer(this);
    _failedAttempts = 0;

    connectEvents();
}

void LoginPage::showEvent(QShowEvent *event)
{
    if(event->spontaneous())
    {
        QWidget::showEvent(event);
        return;
    }

    serverAddressChanged( ui_serverBox->address() );
}

void LoginPage::loggedIn(RamUser *user)
{
    ui_loginWidget->hide();
    ui_connectionStatusLabel->setText("Connected as " + user->name());

    // Save server address to history
    QSettings settings;
    QString address = settings.value("server/address", "localhost/ramses/").toString();
    int historySize = settings.beginReadArray("server/serverHistory");
    int historyIndex = 0;
    bool found = false;
    for (int i = 0; i < historySize; i++)
    {
        settings.setArrayIndex(i);
        // Get adress in settings, check with "/" no matter what
        QString settingsAddress = settings.value("address").toString();
        if (!settingsAddress.endsWith("/")) settingsAddress += "/";

        if (settingsAddress == address )
        {
            found = true;
            break;
        }
        historyIndex++;
    }
    settings.endArray();

    if (!found) historySize++;
    settings.beginWriteArray("server/serverHistory", historySize);
    settings.setArrayIndex(historyIndex);
    settings.setValue("address", address);

    // Save credentials
    if (ui_saveUsername->isChecked())
    {
        // Encrypt
        SimpleCrypt crypto( SimpleCrypt::machineKey() );

        settings.setValue("username", crypto.encryptToString(ui_usernameEdit->text()));

        if (ui_savePassword->isChecked())
        {
            // Save the hashed password
            QString hashed;
            if (ui_passwordEdit->text() == "" && ui_passwordEdit->placeholderText() == "Use saved password." && m_hashedPassword != "")
            {
                hashed = m_hashedPassword;
            }
            else
            {
                hashed = DBInterface::instance()->generatePassHash( ui_passwordEdit->text() );
            }
            // But encrypted, as the hashed password can be used to login
            settings.setValue("password", crypto.encryptToString( hashed ));
        }
        else
        {
            settings.setValue("password", "");
        }
    }
    else
    {
        settings.setValue("username", "");
        settings.setValue("password", "");
    }
    settings.endArray();

    ui_passwordEdit->setText("");
    if (ui_savePassword->isChecked())
        ui_passwordEdit->setPlaceholderText("Use saved password.");
    else
        ui_passwordEdit->setPlaceholderText("Password");

    if (!ui_saveUsername->isChecked())
        ui_saveUsername->setText("");
}

void LoginPage::loggedOut()
{
    ui_loginWidget->show();
    ui_connectionStatusLabel->setText("Ready");
    serverAddressChanged( ui_serverBox->address() );
}

void LoginPage::dbiData(QJsonObject data)
{
    if (data.value("query").toString() == "login")
    {
        if (data.value("success").toBool())
        {
            unFreeze();
            _failedAttempts = 0;
        }
        else
        {
            _failedAttempts++;
            freeze();
        }
    }
}

void LoginPage::loginButton_clicked()
{
    //check login in database, initiate
    if (ui_usernameEdit->text() == "")
    {
        ui_connectionStatusLabel->setText("Please fill your username in.");
        return;
    }

    if (ui_passwordEdit->text() == "" && (ui_passwordEdit->placeholderText() != "Use saved password." || m_hashedPassword == ""))
    {
        ui_connectionStatusLabel->setText("Please fill your password in.");
        return;
    }

    if (ui_passwordEdit->text() == "")
    {
        _ramses->loginHashed(ui_usernameEdit->text(), m_hashedPassword);
    }
    else
    {
        _ramses->login(ui_usernameEdit->text(), ui_passwordEdit->text());
    }

    ui_connectionStatusLabel->setText("Connecting...");
}

void LoginPage::serverSettingsButton_clicked()
{
    emit serverSettings();
}

void LoginPage::serverAddressChanged(QString address)
{
    // Notify the interface
    DBInterface::instance()->setServerAddress(address);

    // Load saved username and password
    QSettings settings;

    int historySize = settings.beginReadArray("server/serverHistory");
    for (int i = 0; i < historySize; i++)
    {
        settings.setArrayIndex(i);
        // Get adress in settings, check with "/" no matter what
        QString settingsAddress = settings.value("address").toString();
        if (!settingsAddress.endsWith("/")) settingsAddress += "/";
        if (settingsAddress == address )
        {
            // Decrypt
            SimpleCrypt crypto( SimpleCrypt::machineKey() );

            QString username = settings.value("username", "").toString();

            if (username == "")
            {
                ui_saveUsername->setChecked(false);
                ui_usernameEdit->setText("");
                ui_passwordEdit->setText("");
                ui_passwordEdit->setPlaceholderText("Password");
            }
            else
            {
                username = crypto.decryptToString( username );
                ui_usernameEdit->setText( username );
                ui_saveUsername->setChecked(true);

                QString password = settings.value("password", "").toString();
                if (password == "")
                {
                    ui_savePassword->setChecked(false);
                    ui_passwordEdit->setText("");
                    ui_passwordEdit->setPlaceholderText("Password");
                    m_hashedPassword = "";
                }
                else
                {
                    ui_savePassword->setChecked(true);
                    m_hashedPassword = crypto.decryptToString( password );
                    ui_passwordEdit->setPlaceholderText("Use saved password.");
                    ui_passwordEdit->setText("");
                }
            }
            break;
        }
    }
    settings.endArray();
}

void LoginPage::toggleSaveUsername(bool enabled)
{
    if (enabled) {
        ui_savePassword->setEnabled(true);
    }
    else {
        ui_savePassword->setEnabled(false);
        ui_savePassword->setChecked(false);
    }
}

void LoginPage::toggleSavePassword(bool enabled)
{
    if (!enabled) return;

    // Confirm that !

    QMessageBox::StandardButton result = QMessageBox::question(
                this,
                "Please confirm",
                "Saving the password is unsafe.\nAre you sure you want to save your password?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No
                );
    if (result == QMessageBox::No) ui_savePassword->setChecked(false);
}

void LoginPage::setupUi()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addStretch();

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(3);
    mainLayout->addLayout(layout);

    layout->addStretch();

    QLabel *logoLabel = new QLabel(this);
    logoLabel->setPixmap(QPixmap(":/icons/logo_large"));
    logoLabel->setMinimumSize(QSize(256, 298));
    logoLabel->setMaximumSize(QSize(256, 298));
    logoLabel->setScaledContents(true);
    logoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(logoLabel);

    layout->addStretch();

    ui_loginWidget = new QWidget(this);
    ui_loginWidget->setMaximumWidth(256);
    layout->addWidget(ui_loginWidget);

    QVBoxLayout *loginLayout = new QVBoxLayout(ui_loginWidget);
    loginLayout->setSpacing(3);
    loginLayout->setContentsMargins(0,0,0,0);

    QHBoxLayout *serverLayout = new QHBoxLayout();
    serverLayout->setSpacing(3);
    serverLayout->setContentsMargins(0,0,0,0);
    loginLayout->addLayout(serverLayout);

    ui_serverBox = new DuQFServerComboBox("localhost/ramses", this);
    serverLayout->addWidget(ui_serverBox);

    ui_sslBox = new DuQFSSLCheckbox("Secure", this);
    serverLayout->addWidget(ui_sslBox);

    serverLayout->setStretch(0, 100);
    serverLayout->setStretch(1, 0);

    QHBoxLayout *usernameLayout = new QHBoxLayout();
    usernameLayout->setSpacing(3);
    usernameLayout->setContentsMargins(0,0,0,0);
    loginLayout->addLayout(usernameLayout);

    ui_usernameEdit = new QLineEdit(this);
    ui_usernameEdit->setPlaceholderText("Username");
    usernameLayout->addWidget(ui_usernameEdit);

    ui_saveUsername = new QCheckBox("Save", this);
    usernameLayout->addWidget(ui_saveUsername);

    QHBoxLayout *passwordLayout = new QHBoxLayout();
    passwordLayout->setSpacing(3);
    passwordLayout->setContentsMargins(0,0,0,0);
    loginLayout->addLayout(passwordLayout);

    ui_passwordEdit = new QLineEdit(this);
    ui_passwordEdit->setEchoMode(QLineEdit::Password);
    ui_passwordEdit->setPlaceholderText("Password");
    passwordLayout->addWidget(ui_passwordEdit);

    ui_savePassword = new QCheckBox("Save", this);
    ui_savePassword->setEnabled(false);
    passwordLayout->addWidget(ui_savePassword);

    ui_capsLockLabel = new QLabel("Be careful, CAPS LOCK is on!", this);
    ui_capsLockLabel->setWordWrap(true);
    loginLayout->addWidget(ui_capsLockLabel);

    ui_loginButton = new QPushButton("Log in", this);
    ui_loginButton->setIcon(QIcon(":/icons/login"));
    loginLayout->addWidget(ui_loginButton);

    ui_connectionStatusLabel = new QLabel("Ready...", this);
    ui_connectionStatusLabel->setEnabled(false);
    ui_connectionStatusLabel->setWordWrap(true);
    ui_connectionStatusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(ui_connectionStatusLabel);

    layout->addStretch();
    mainLayout->addStretch();

    // Tab order
    QWidget::setTabOrder(ui_serverBox, ui_usernameEdit);
    QWidget::setTabOrder(ui_usernameEdit, ui_passwordEdit);
}

void LoginPage::connectEvents()
{
    connect(ui_saveUsername, SIGNAL(toggled(bool)), this, SLOT(toggleSaveUsername(bool)));
    connect(ui_savePassword, SIGNAL(clicked(bool)), this, SLOT(toggleSavePassword(bool)));

    connect(_ramses,&Ramses::loggedIn, this, &LoginPage::loggedIn);
    connect(_ramses,&Ramses::loggedOut, this, &LoginPage::loggedOut);
    connect(ui_usernameEdit, &QLineEdit::returnPressed, this, &LoginPage::loginButton_clicked);
    connect(ui_passwordEdit, &QLineEdit::returnPressed, this, &LoginPage::loginButton_clicked);
    //connect(DBInterface::instance(), &DBInterface::log, this, &LoginPage::dbiLog);
    //connect(Daemon::instance(), &Daemon::log, this, &LoginPage::daemonLog);
    connect(DBInterface::instance(), &DBInterface::data, this, &LoginPage::dbiData);
    connect(ui_loginButton, SIGNAL(clicked()), this, SLOT(loginButton_clicked()));
    connect(_failedTimer, &QTimer::timeout, this, &LoginPage::unFreeze);
    connect(_uiTimer, &QTimer::timeout, this, &LoginPage::updateFreeze);

    connect(DBInterface::instance(), SIGNAL(serverAddressChanged(QString)), ui_serverBox, SLOT(setAddress(QString)));
    connect(DBInterface::instance(), SIGNAL(sslChanged(bool)), ui_sslBox, SLOT(setChecked(bool)));

    connect(ui_serverBox, SIGNAL(addressChanged(QString)), this, SLOT(serverAddressChanged(QString)));
    connect(ui_sslBox, SIGNAL(toggled(bool)), DBInterface::instance(), SLOT(setSSL(bool)));

    // Needs to be called once
    serverAddressChanged(ui_serverBox->currentText());
}

void LoginPage::freeze()
{
    ui_loginWidget->hide();
    int timeout = _failedAttempts * _failedAttempts;
    qDebug() << "Freezing login page for " + QString::number(timeout) + " seconds";
    _failedTimer->start(timeout*1000);
    _uiTimer->start(1000);

    ui_connectionStatusLabel->setEnabled(true);
}

void LoginPage::unFreeze()
{
    ui_loginWidget->show();
    ui_connectionStatusLabel->setText("Ready.");
    ui_connectionStatusLabel->setEnabled(false);
    _uiTimer->stop();
    _failedTimer->stop();
}

void LoginPage::updateFreeze()
{
    int remaining = _failedTimer->remainingTime() / 1000;
    ui_connectionStatusLabel->setText(QString::number(_failedAttempts) + " failed Attempts.\nPlease wait " + QString::number(remaining) + " seconds.");
}

