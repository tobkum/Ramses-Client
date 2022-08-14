﻿#ifndef STEPEDITWIDGET_H
#define STEPEDITWIDGET_H

#include <QComboBox>
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QToolButton>
#include <QListWidget>
#include <QFormLayout>
#include <QMenu>

#include "duqf-widgets/autoselectdoublespinbox.h"
#include "objecteditwidget.h"
#include "objectlisteditwidget.h"
#include "duqf-widgets/duqffolderdisplaywidget.h"
#include "data-views/ramobjectlistcombobox.h"
#include "duqf-widgets/duqfcolorselector.h"

class StepEditWidget : public ObjectEditWidget
{
    Q_OBJECT
public:
    StepEditWidget(QWidget *parent = nullptr);
    StepEditWidget(RamStep *s, QWidget *parent = nullptr);

    RamStep *step() const;

protected:
    virtual void reInit(RamObject *o) override;

private slots:
    void createApplication();
    void updateEstimationSuffix();

    void setType(int t);
    void setPublishSettings();
    void setColor(QColor c);
    void setEstimationType(int t);
    void setVeryEasy(double e);
    void setEasy(double e);
    void setMedium(double e);
    void setHard(double e);
    void setVeryHard(double e);
    void activateMultiplier(bool a);
    void setMultiplier(RamAssetGroup* ag);

private:
    void setupUi();
    void connectEvents();

    RamStep *m_step;

    QComboBox *ui_typeBox;
    QWidget *ui_estimationWidget;
    QComboBox *ui_estimationTypeBox;
    QLabel *ui_estimationTypeLabel;
    DuQFTextEdit *ui_publishSettingsEdit;
    QTabWidget *ui_tabWidget;
    AutoSelectDoubleSpinBox *ui_veryEasyEdit;
    AutoSelectDoubleSpinBox *ui_easyEdit;
    AutoSelectDoubleSpinBox *ui_mediumEdit;
    AutoSelectDoubleSpinBox *ui_hardEdit;
    AutoSelectDoubleSpinBox *ui_veryHardEdit;
    DuQFFolderDisplayWidget *ui_folderWidget;
    QCheckBox *ui_estimationMultiplierCheckBox;
    RamObjectListComboBox *ui_estimationMultiplierBox;
    DuQFColorSelector *ui_colorSelector;

    ObjectListEditWidget *m_applicationList;

};

#endif // STEPEDITWIDGET_H
