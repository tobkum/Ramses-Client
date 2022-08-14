#include "ramobjectfiltermodel.h"

RamObjectFilterModel::RamObjectFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    m_emptyList = RamObjectList::getObject("emptylist", true);
    setSourceModel(m_emptyList);
}

void RamObjectFilterModel::setList(RamObjectList *list)
{
    if (this->sourceModel() == list) return;

    if (!list) this->setSourceModel(m_emptyList);
    else this->setSourceModel(list);

    // Refresh
    QString f = m_currentFilterUuid;
    setFilterUuid("");
    setFilterUuid(f);
}

void RamObjectFilterModel::setFilterUuid(const QString &filterUuid)
{
    m_currentFilterUuid = filterUuid;
    prepareFilter();
}

void RamObjectFilterModel::search(const QString &searchStr)
{
    m_searchString = searchStr;
    prepareFilter();
}

RamObject *RamObjectFilterModel::at(int i) const
{
    // Get the source index
    if (!hasIndex(i, 0)) return nullptr;

    return RamObjectList::at(index(i, 0));
}

RamObject *RamObjectFilterModel::at(QModelIndex i) const
{
    return RamObjectList::at(i);
}

bool RamObjectFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    RamObject *obj = RamObjectList::at(index);
    if (!obj) return false;

    bool filterOK = m_currentFilterUuid == "" || obj->filterUuid() == m_currentFilterUuid;
    if (!filterOK) return false;
    if (m_searchString == "") return true;
    if (obj->shortName().contains(m_searchString, Qt::CaseInsensitive)) return true;
    return obj->name().contains(m_searchString, Qt::CaseInsensitive);
}

void RamObjectFilterModel::prepareFilter()
{
    emit aboutToFilter();
    invalidateFilter();
}
