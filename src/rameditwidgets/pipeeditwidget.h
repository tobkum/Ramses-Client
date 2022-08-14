#ifndef PIPEEDITWIDGET_H
#define PIPEEDITWIDGET_H

#include <QComboBox>
#include <QMenu>

#include "objecteditwidget.h"
#include "objectlisteditwidget.h"
#include "ramobjectlistcombobox.h"
#include "rampipe.h"

/**
 * @brief The PipeEditWidget class is used to edit a RamPipe and is usually shown in the MainWindow Dock
 */
class PipeEditWidget : public ObjectEditWidget
{
public:
    PipeEditWidget(QWidget *parent = nullptr);
    PipeEditWidget(RamPipe *pipe, QWidget *parent = nullptr);

    RamPipe *pipe() const;

protected:
    virtual void reInit(RamObject *o) override;

private slots:
    void appChanged();
    void createPipeFile();
    void setInputStep(int i);
    void setOutputStep(int i);

private:
    RamPipe *m_pipe = nullptr;

    void setupUi();
    void connectEvents();
    void populateMenus(RamProject *project);

    RamObjectListComboBox *ui_fromBox;
    RamObjectListComboBox *ui_toBox;
    ObjectListEditWidget *ui_pipeFileList;
};

#endif // PIPEEDITWIDGET_H
