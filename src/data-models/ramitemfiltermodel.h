#ifndef RAMITEMFILTERMODEL_H
#define RAMITEMFILTERMODEL_H

#include "ramobjectfiltermodel.h"
#include "ramabstractitem.h"

/**
 * @brief The RamItemFilterModel class is used to filters items according to current state, step or assigned user.
 * It also sorts the items according to: their completion ratio, their estimation, their time spent, their difficulty, their name or their ID (or default)
 */
class RamItemFilterModel : public RamObjectFilterModel
{
    Q_OBJECT
public:
    enum SortMode {
        Default = 1,
        ShortName = 2,
        Name = 3,
        Difficulty = 4,
        TimeSpent = 5,
        Estimation = 6,
        Completion = 7
    };

    explicit RamItemFilterModel(QObject *parent = nullptr);

    void freeze();
    void unFreeze();

    void useFilters(bool use = true);

    void hideUser(RamObject *u);
    void showUser(RamObject *u);
    void clearUsers();
    void showUnassigned(bool show);

    void hideState(RamObject *s);
    void showState(RamObject *s);
    void clearStates();

    void setStepType(RamStep::Type t);
    void hideStep(RamObject *s);
    void showStep(RamObject *s);
    void showAllSteps();

    SortMode sortMode() const;
    void setSortMode(SortMode newSortMode);

public slots:
    void resort(int col, Qt::SortOrder order = Qt::AscendingOrder);
    void unsort();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool filterAcceptsColumn(int sourceRow, const QModelIndex &sourceParent) const override;

    QList<RamObject*> m_states;
    QList<RamObject*> m_users;
    QList<RamObject*> m_hiddenSteps;
    bool m_showUnassigned = true;
    RamStep::Type m_stepType = RamStep::All;

private:
    /**
     * @brief step Get a step given a source column
     * @param column The column of the step in the source model
     * @return The RamStep* or nullptr if the step is filtered
     */
    RamStep *step( int column ) const;
    bool m_userFilters = false;

    bool m_frozen = false;

    SortMode m_sortMode = Default;
};

#endif // RAMITEMFILTERMODEL_H
