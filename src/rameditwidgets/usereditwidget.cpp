#include "usereditwidget.h"

UserEditWidget::UserEditWidget(RamUser *user, QWidget *parent) :
    ObjectEditWidget(user, parent)
{
    setupUi();
    connectEvents();

    setObject(user);
}

UserEditWidget::UserEditWidget(QWidget *parent) :
    ObjectEditWidget(parent)
{
    setupUi();
    connectEvents();

    setObject(nullptr);
}

RamUser *UserEditWidget::user() const
{
    return _user;
}

void UserEditWidget::setObject(RamObject *obj)
{
    RamUser *user = qobject_cast<RamUser*>(obj);

    this->setEnabled(false);

    ObjectEditWidget::setObject(user);
    _user = user;

    QSignalBlocker b1(ui_cpasswordEdit);
    QSignalBlocker b2(ui_npassword1Edit);
    QSignalBlocker b3(ui_npassword2Edit);
    QSignalBlocker b4(ui_roleBox);
    QSignalBlocker b5(ui_folderSelector);

    ui_cpasswordEdit->setText("");
    ui_npassword1Edit->setText("");
    ui_npassword2Edit->setText("");
    ui_roleBox->setCurrentIndex(0);
    ui_roleBox->setEnabled(true);
    ui_roleBox->setToolTip("");
    ui_cpasswordEdit->setEnabled(false);
    ui_folderSelector->setPath("");
    ui_folderSelector->setPlaceHolderText("Default (Ramses/Users/User_ShortName)");
    updateFolderLabel("");

    RamUser *current = Ramses::instance()->currentUser();
    if (!user || !current) return;

    ui_roleBox->setCurrentIndex(user->role());

    if (user->folderPath() != "auto") ui_folderSelector->setPath( user->folderPath() );
    ui_folderSelector->setPlaceHolderText( Ramses::instance()->defaultUserPath(user) );
    ui_folderLabel->setText( Ramses::instance()->path(user) );

    if (user->uuid() == current->uuid())
    {
        ui_roleBox->setEnabled(false);
        ui_roleBox->setToolTip("You cannot change your own role!");
        ui_cpasswordEdit->setEnabled(true);
        this->setEnabled(true);
    }
    else
    {
        this->setEnabled(Ramses::instance()->isAdmin());
    }
}

void UserEditWidget::changePassword()
{
    if (!checkInput()) return;

    if (ui_npassword1Edit->text() != "")
    {
        _user->updatePassword(
                    ui_cpasswordEdit->text(),
                    ui_npassword1Edit->text() );
    }

    ui_npassword1Edit->setText("");
    ui_npassword2Edit->setText("");
    ui_cpasswordEdit->setText("");
}

void UserEditWidget::update()
{
    if (!_user) return;

    updating = true;

    _user->setFolderPath( ui_folderSelector->path());

    int roleIndex = ui_roleBox->currentIndex();
    if (roleIndex == 3) _user->setRole(RamUser::Admin);
    else if (roleIndex == 2) _user->setRole(RamUser::ProjectAdmin);
    else if (roleIndex == 1) _user->setRole(RamUser::Lead);
    else _user->setRole(RamUser::Standard);

    ObjectEditWidget::update();

    updating = false;
}

bool UserEditWidget::checkInput()
{
    if (!_user) return false;

    return ObjectEditWidget::checkInput();

    if (ui_npassword1Edit->text() != "")
    {
        if (ui_cpasswordEdit->text() == "" && _user->uuid() == Ramses::instance()->currentUser()->uuid())
        {
            statusLabel->setText("You must specify your current password to be able to modify it.");
            return false;
        }
        if (ui_npassword1Edit->text() != ui_npassword2Edit->text())
        {
            statusLabel->setText("The two fields for the new password are different.");
            return false;
        }
    }

    statusLabel->setText("");

    return true;
}

void UserEditWidget::updateFolderLabel(QString path)
{
    if (path != "") ui_folderLabel->setText( Ramses::instance()->pathFromRamses(path) );
    else if (_user) ui_folderLabel->setText( Ramses::instance()->path(_user) );
}

void UserEditWidget::setupUi()
{
    QLabel *roleLabel = new QLabel("Current role", this);
    mainFormLayout->addWidget(roleLabel, 2, 0);

    ui_roleBox = new QComboBox(this);
    ui_roleBox->addItem(QIcon(":/icons/user"), "Standard");
    ui_roleBox->addItem(QIcon(":/icons/lead"), "Lead");
    ui_roleBox->addItem(QIcon(":/icons/project-admin"), "Project Admin");
    ui_roleBox->addItem(QIcon(":/icons/admin"), "Administrator");
    ui_roleBox->setCurrentIndex(0);
    mainFormLayout->addWidget(ui_roleBox, 2, 1);

    QLabel *currentPasswordLabel = new QLabel("Current password", this);
    mainFormLayout->addWidget(currentPasswordLabel, 3, 0);

    ui_cpasswordEdit = new QLineEdit(this);
    ui_cpasswordEdit->setEchoMode(QLineEdit::Password);
    mainFormLayout->addWidget(ui_cpasswordEdit, 3, 1);

    QLabel *newPasswordLabel = new QLabel("New password", this);
    mainFormLayout->addWidget(newPasswordLabel, 4, 0);

    ui_npassword1Edit = new QLineEdit(this);
    ui_npassword1Edit->setEchoMode(QLineEdit::Password);
    mainFormLayout->addWidget(ui_npassword1Edit, 4, 1);

    ui_npassword2Edit = new QLineEdit(this);
    ui_npassword2Edit->setEchoMode(QLineEdit::Password);
    mainFormLayout->addWidget(ui_npassword2Edit, 5, 1);

    ui_passwordButton = new QToolButton(this);
    ui_passwordButton->setText("Change password");
    mainFormLayout->addWidget(ui_passwordButton, 6, 1);

    QLabel *uFolderLabel = new QLabel("Personal folder", this);
    mainFormLayout->addWidget(uFolderLabel, 7, 0);

    ui_folderSelector = new DuQFFolderSelectorWidget(DuQFFolderSelectorWidget::Folder, this);
    ui_folderSelector->setPlaceHolderText("Default (Ramses/Users/User_ShortName)");
    mainFormLayout->addWidget(ui_folderSelector, 7, 1);

    ui_folderLabel = new QLabel(this);
    ui_folderLabel->setEnabled(false);
    mainLayout->insertWidget(1, ui_folderLabel);

    mainLayout->addStretch();
}

void UserEditWidget::connectEvents()
{
    connect(ui_cpasswordEdit, &QLineEdit::textChanged, this, &UserEditWidget::checkInput);
    connect(ui_npassword1Edit, &QLineEdit::textChanged, this, &UserEditWidget::checkInput);
    connect(ui_npassword2Edit, &QLineEdit::textChanged, this, &UserEditWidget::checkInput);
    connect(ui_roleBox, SIGNAL(currentIndexChanged(int)), this, SLOT(update()));
    connect(ui_passwordButton, SIGNAL(clicked()), this, SLOT(changePassword()));
    connect(ui_folderSelector, &DuQFFolderSelectorWidget::pathChanging, this, &UserEditWidget::updateFolderLabel);
    connect(ui_folderSelector, &DuQFFolderSelectorWidget::pathChanged, this, &UserEditWidget::update);
    connect(Ramses::instance(), &Ramses::loggedIn, this, &UserEditWidget::objectChanged);
}
