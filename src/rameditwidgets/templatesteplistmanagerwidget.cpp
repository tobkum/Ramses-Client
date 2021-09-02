#include "templatesteplistmanagerwidget.h"

TemplateStepListManagerWidget::TemplateStepListManagerWidget(QWidget *parent) :
    ObjectListManagerWidget(
        Ramses::instance()->templateSteps(),
        new TemplateStepEditWidget(),
        "Template steps",
        QIcon(":icons/step"),
        parent )
{
    m_listEditWidget->setEditMode(ObjectListEditWidget::RemoveObjects);
}

RamObject *TemplateStepListManagerWidget::createObject()
{
    RamStep *ts = new RamStep(
                "NEW",
                "New Template Step"
                );

    Ramses::instance()->templateSteps()->append(ts);

    editObject(ts);
    return ts;
}
