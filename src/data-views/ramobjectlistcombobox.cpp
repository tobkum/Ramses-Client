#include "ramobjectlistcombobox.h"

#include "data-models/ramobjectfilterlist.h"
#include "ramobjectdelegate.h"

RamObjectListComboBox::RamObjectListComboBox(QWidget *parent) :
    QComboBox(parent)
{
    setupUi();
    setList(nullptr);
    connectEvents();
}

RamObjectListComboBox::RamObjectListComboBox(bool isFilterBox, QWidget *parent) :
    QComboBox(parent)
{
    setupUi();
    m_isFilterBox = isFilterBox;
    setList(nullptr);
    connectEvents();
}

RamObjectListComboBox::RamObjectListComboBox(RamObjectList *list, QWidget *parent) :
    QComboBox(parent)
{
    setupUi();
    setList(list);
    connectEvents();
}

void RamObjectListComboBox::setList(RamObjectList *list)
{
    if (m_isFilterBox)
    {
        RamObjectFilterList *proxyModel = new RamObjectFilterList(this);
        proxyModel->setList(list);
        this->setModel(proxyModel);
    }
    else
    {
        this->setModel(list);
    }
}

RamObject *RamObjectListComboBox::currentObject()
{
    int i = this->currentIndex();
    return object(i);
}

QString RamObjectListComboBox::currentUuid()
{
    int i = this->currentIndex();
    return uuid(i);
}

void RamObjectListComboBox::setObject(QString objUuid)
{
    if (objUuid == "" && m_isFilterBox)
    {
        setCurrentIndex(0);
        return;
    }
    if (objUuid == "")
    {
        setCurrentIndex(-1);
        return;
    }

    for(int i = 0; i < this->count(); i++)
    {
        QString ou = uuid(i);
        if (objUuid == ou)
        {
            this->setCurrentIndex(i);
            return;
        }
    }
    this->setCurrentIndex(-1);
}

void RamObjectListComboBox::setObject(RamObject *obj)
{
    if (!obj) setObject("");
    else setObject(obj->uuid());
}

RamObject *RamObjectListComboBox::object(int i)
{
    if (m_isFilterBox)
    {
        RamObjectFilterList *m = reinterpret_cast<RamObjectFilterList*>(model());
        return m->at(i);
    }
    else
    {
        RamObjectList *m = qobject_cast<RamObjectList*>(model());
        return m->at(i);
    }
}

QString RamObjectListComboBox::uuid(int i)
{
    RamObject *o = object(i);
    if (o) return o->uuid();
    return "";
}

void RamObjectListComboBox::beginReset()
{
    m_resetting = true;
    m_resettingObject = currentObject();
}

void RamObjectListComboBox::endReset()
{
    m_resetting = false;
    setObject(m_resettingObject);
}

void RamObjectListComboBox::showPopup()
{
    // Update size
    // get the minimum width that fits the largest item.
    int width = this->minimumSizeHint().width();
    // set the view to that width + icon and margins.
    this->view()->setMinimumWidth(width + 40);

    QComboBox::showPopup();
    emit popupShown();
}

void RamObjectListComboBox::hidePopup()
{
    QComboBox::hidePopup();
    emit popupHidden();
}

void RamObjectListComboBox::currentObjectIndexChanged(int i)
{
    Q_UNUSED(i)

#ifdef DUMP_OBJECT_DEBUG
    dumpObjectInfo();
#endif

    if (!m_resetting)
    {
        emit currentObjectChanged( currentObject() );
        emit currentUuidChanged( currentUuid() );
    }
}

void RamObjectListComboBox::objectIndexActivated(int i)
{
    Q_UNUSED(i)

#ifdef DUMP_OBJECT_DEBUG
    dumpObjectInfo();
#endif

    emit objectActivated( currentObject() );
    emit uuidActivated( currentUuid() );
}

void RamObjectListComboBox::setupUi()
{
    //this->setSizeAdjustPolicy(SizeAdjustPolicy::AdjustToContents);
    RamObjectDelegate *delegate = new RamObjectDelegate(this);
    delegate->setComboBoxMode(true);
    this->setItemDelegate(delegate);
}

void RamObjectListComboBox::connectEvents()
{
    connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(currentObjectIndexChanged(int)));
}
