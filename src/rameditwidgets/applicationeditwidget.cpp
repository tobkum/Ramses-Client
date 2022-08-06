#include "applicationeditwidget.h"

#include "ramfiletype.h"
#include "ramses.h"

ApplicationEditWidget::ApplicationEditWidget(QWidget *parent) : ObjectEditWidget(parent)
{
    setupUi();
    connectEvents();
}

ApplicationEditWidget::ApplicationEditWidget(RamApplication *app, QWidget *parent) : ObjectEditWidget(parent)
{
    setupUi();
    connectEvents();
    setObject(app);
}

RamApplication *ApplicationEditWidget::application() const
{
    return m_application;
}

void ApplicationEditWidget::reInit(RamObject *o)
{
    m_application = qobject_cast<RamApplication* >(o);
    if (m_application)
    {
        m_folderSelector->setPath(m_application->executableFilePath());
        m_nativeList->setList(m_application->nativeFileTypes());
        m_importList->setList(m_application->importFileTypes());
        m_exportList->setList(m_application->exportFileTypes());
    }
    else
    {
        m_folderSelector->setPath("");
        m_nativeList->clear();
        m_importList->clear();
        m_exportList->clear();
    }
}

void ApplicationEditWidget::createForNative()
{
    if (!m_application) return;
    RamFileType *ft = new RamFileType(
                "NEW",
                "New file type");
    Ramses::instance()->fileTypes()->append(ft);
    m_application->nativeFileTypes()->append(ft);
    ft->edit();
}

void ApplicationEditWidget::createForImport()
{
    if (!m_application) return;
    RamFileType *ft = new RamFileType(
                "NEW",
                "New file type");
    Ramses::instance()->fileTypes()->append(ft);
    m_application->importFileTypes()->append(ft);
    ft->edit();
}

void ApplicationEditWidget::createForExport()
{
    if (!m_application) return;
    RamFileType *ft = new RamFileType(
                "NEW",
                "New file type");
    Ramses::instance()->fileTypes()->append(ft);
    m_application->exportFileTypes()->append(ft);
    ft->edit();
}

void ApplicationEditWidget::setupUi()
{
    QLabel *fileLabel = new QLabel("Executable file", this);
    ui_mainFormLayout->addWidget(fileLabel, 3, 0);

    m_folderSelector = new DuQFFolderSelectorWidget(DuQFFolderSelectorWidget::File, this);
    ui_mainFormLayout->addWidget(m_folderSelector, 3, 1);

    QTabWidget *tabWidget = new QTabWidget(this);

    m_nativeList = new ObjectListEditWidget<RamFileType*, int>(true, RamUser::Admin);
    m_nativeList->setEditMode(ObjectListEditWidget<RamFileType*, int>::UnassignObjects);
    m_nativeList->setTitle("Native file types");
    m_nativeList->setAssignList(Ramses::instance()->fileTypes());
    tabWidget->addTab(m_nativeList, QIcon(":/icons/files"), "Native");

    m_importList = new ObjectListEditWidget<RamFileType*, int>(true, RamUser::Admin);
    m_importList->setEditMode(ObjectListEditWidget<RamFileType*, int>::UnassignObjects);
    m_importList->setTitle("Imports");
    m_importList->setAssignList(Ramses::instance()->fileTypes());
    tabWidget->addTab(m_importList, QIcon(":/icons/files"), "Import");

    m_exportList = new ObjectListEditWidget<RamFileType*, int>(true, RamUser::Admin);
    m_exportList->setEditMode(ObjectListEditWidget<RamFileType*, int>::UnassignObjects);
    m_exportList->setTitle("Exports");
    m_exportList->setAssignList(Ramses::instance()->fileTypes());
    tabWidget->addTab(m_exportList, QIcon(":/icons/files"), "Export");

    ui_mainLayout->addWidget(tabWidget);

    ui_mainLayout->setStretch(2, 100);
}

void ApplicationEditWidget::connectEvents()
{
    connect(m_nativeList, SIGNAL(add()), this, SLOT(createForNative()));
    connect(m_importList, SIGNAL(add()), this, SLOT(createForImport()));
    connect(m_exportList, SIGNAL(add()), this, SLOT(createForExport()));
}

