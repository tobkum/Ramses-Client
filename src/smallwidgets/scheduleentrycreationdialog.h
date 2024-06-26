#ifndef SCHEDULEENTRYCREATIONDIALOG_H
#define SCHEDULEENTRYCREATIONDIALOG_H

#include "qdialogbuttonbox.h"
#include <QDialog>

#include "duqf-widgets/dulineedit.h"
#include "duqf-widgets/duqftextedit.h"

class ScheduleEntryCreationDialog : public QDialog
{
    Q_OBJECT
public:
    ScheduleEntryCreationDialog( QWidget *parent = nullptr );

    QString title() const;
    QString comment() const;

private:
    DuLineEdit *ui_titleEdit;
    DuQFTextEdit *ui_commentEdit;
    QDialogButtonBox *ui_buttonBox;
};

#endif // SCHEDULEENTRYCREATIONDIALOG_H
