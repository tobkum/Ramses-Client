#ifndef SHOTSMANAGERWIDGET_H
#define SHOTSMANAGERWIDGET_H

#include "listmanagerwidget.h"
#include "shoteditwidget.h"

class ShotsManagerWidget : public ListManagerWidget
{
    Q_OBJECT
public:
    ShotsManagerWidget(QWidget *parent = nullptr);

protected slots:
    void currentDataChanged(QVariant data) Q_DECL_OVERRIDE;
    void createItem() Q_DECL_OVERRIDE;
    void removeItem(QVariant data) Q_DECL_OVERRIDE;

private slots:
    void changeProject(RamProject *project);
    void newShot(RamShot *shot);
    void shotChanged();
    void shotRemoved(QString uuid);
    void filter(QString sequenceUuid);
    void addShots(RamSequence *sequence);
    void sequenceChanged();
    void sequenceRemoved(QString uuid);
    void newSequence(RamSequence *sequence);

private:
    ShotEditWidget *shotWidget;
    QList<QMetaObject::Connection> _projectConnections;
    QList<QMetaObject::Connection> _sequencesConnections;
};

#endif // SHOTSMANAGERWIDGET_H
