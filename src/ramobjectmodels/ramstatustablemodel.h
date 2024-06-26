#ifndef RAMSTATUSTABLEMODEL_H
#define RAMSTATUSTABLEMODEL_H

#include "dbtablemodel.h"
#include "ramstatus.h"

struct StepEstimation {
    float estimation = 0; // Days
    int total = 0; // Days
    int completionRatio = 0; // [0, 100] ratio

    // Updates the completion after having cached all status
    void updateCompletionRatio() {
        if (this->total > 0)
            this->completionRatio = this->completionRatio / total;
        else
            this->completionRatio = 100;

        this->completionRatio = std::min(100, this->completionRatio);
    }
};

class RamStatusTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit RamStatusTableModel(DBTableModel *steps, DBTableModel *items, QObject *parent = nullptr);

    void load();

    // === QAbstractTableModel Reimplementation ===

    // Data access
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Status Access

    // With the actual item and step, slightly better perf than with Uuids if they already exist
    RamStatus *getStatus(RamObject *itemObj, RamStep *step) const;
    RamStatus *getStatus(QString itemUuid, QString stepUuid) const;
    QSet<RamStatus*> getStatus(const QDate &date, const QString userUuid = "") const;
    QSet<RamStatus*> getItemStatus(QString itemUuid) const;
    QSet<RamStatus*> getStepStatus(QString stepUuid) const;

    // Estimations
    void suspendEstimations(bool s);
    float stepEstimation(const QString &stepUuid, const QString &userUuid = "") const;
    int stepCompletionRatio(const QString &stepUuid, const QString &userUuid = "") const;

signals:
    void stepEstimationChanged(QString stepUuid);
    void estimationsChanged();

private slots:
    // We need to insert/remove rows and columns
    // and monitor changes

    void stepsInserted(const QModelIndex &parent, int first, int last);
    void stepsRemoved(const QModelIndex &parent, int first, int last);
    void stepsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void stepsDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());

    void itemsInserted(const QModelIndex &parent, int first, int last);
    void itemsRemoved(const QModelIndex &parent, int first, int last);
    void itemsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destination, int row);
    void itemsDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());

    void statusDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());

    // Cache estimations
    void cacheStepEstimation(QString stepUuid);
    void cacheEstimations();

private:
    DBTableModel *m_status;
    DBTableModel *m_steps;
    DBTableModel *m_items;

    // Estimation cache
    bool m_cacheSuspended = false;
    bool m_cacheIsOutdated = false;
    // Estimations cache per step
    QHash<QString, StepEstimation> m_estimations;
    // Estimations cache per step and user
    QHash<QPair<QString,QString>, StepEstimation> m_userestimations;

    bool m_loaded = false;

    // Utils
    void updateStepEstimCache(RamStatus *status, const QString &stepUuid);
};

#endif // RAMSTATUSTABLEMODEL_H
