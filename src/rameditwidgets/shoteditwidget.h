#ifndef SHOTEDITWIDGET_H
#define SHOTEDITWIDGET_H

#include <QDoubleSpinBox>
#include <QSpinBox>

#include "objecteditwidget.h"
#include "duqf-widgets/duqffolderdisplaywidget.h"
#include "duqf-widgets/autoselectdoublespinbox.h"
#include "duqf-widgets/autoselectspinbox.h"
#include "objectlisteditwidget.h"
#include "ramobjectlistcombobox.h"
#include "ramshot.h"

class ShotEditWidget : public ObjectEditWidget
{
    Q_OBJECT
public:
    ShotEditWidget(QWidget *parent = nullptr);
    ShotEditWidget(RamShot *shot, QWidget *parent = nullptr);

    RamShot *shot();

protected:
    virtual void reInit(RamObject *o) override;

private slots:
    void setDuration();
    void setSequence(RamSequence *seq);
    void framesChanged();
    void secondsChanged();

private:
    RamShot *m_shot = nullptr;

    void setupUi();
    void connectEvents();

    DuQFFolderDisplayWidget *ui_folderWidget;
    AutoSelectDoubleSpinBox *ui_secondsBox;
    AutoSelectSpinBox *ui_framesBox;
    RamObjectListComboBox<RamSequence*> *ui_sequencesBox;
    ObjectListEditWidget<RamItem*, RamAssetGroup*> *ui_assetList;
};

#endif // SHOTEDITWIDGET_H
