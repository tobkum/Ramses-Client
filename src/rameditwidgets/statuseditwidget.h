#ifndef STATUSEDITWIDGET_H
#define STATUSEDITWIDGET_H

#include <QPlainTextEdit>
#include <QToolButton>
#include <QFormLayout>
#include <QCheckBox>
#include <QListWidget>
#include <QShortcut>
#include <QMenu>
#include <QSplitter>

#include "duqf-widgets/duqfspinbox.h"
#include "duqf-widgets/autoselectspinbox.h"
#include "duqf-widgets/autoselectdoublespinbox.h"
#include "data-views/ramobjectlistcombobox.h"
#include "duqf-widgets/duqffolderdisplaywidget.h"
#include "duqf-widgets/duqffilelist.h"
#include "objecteditwidget.h"
#include "statebox.h"
#include "ramses.h"

/**
 * @brief The StatusEditWidget class is used to edit a RamStatus.
 */
class StatusEditWidget : public ObjectEditWidget
{
    Q_OBJECT
public:
    StatusEditWidget(QWidget *parent = nullptr);
    StatusEditWidget(RamStatus *status, QWidget *parent = nullptr);

    RamStatus::Difficulty difficulty() const;

public slots:
     void setObject(RamObject *statusObj) override;

protected slots:
    void update() override;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void currentStateChanged(RamObject *stateObj);
    void revert();
    void checkPublished(int v);

    void mainFileSelected();
    void openMainFile();
    void removeSelectedMainFile();

    void createFromTemplate();

    void loadPublishedFiles();
    void publishedFileSelected();
    void openPublishedFile();
    void removeSelectedPublishedFile();

    void previewFileSelected();
    void openPreviewFile();
    void removeSelectedPreviewFile();

    void autoEstimationClicked(bool useAutoEstimation);
    void difficultyBoxChanged();
    void estimateDays(int hours);

private:
    void setupUi();
    void connectEvents();
    StateBox *ui_stateBox;
    DuQFSpinBox *ui_completionBox;
    AutoSelectSpinBox *ui_versionBox;
    QPlainTextEdit *ui_statusCommentEdit;
    QToolButton *ui_revertButton;
    QCheckBox *ui_publishedBox;
    RamObjectListComboBox *ui_userBox;
    AutoSelectSpinBox *ui_timeSpent;
    DuQFFileList *ui_mainFileList;
    DuQFFileList *ui_previewFileList;
    DuQFFileList *ui_publishedFileList;
    QComboBox *ui_versionFileBox;
    QComboBox *ui_versionPublishBox;
    QToolButton *ui_openMainFileButton;
    QToolButton *ui_createMainFileButton;
    QMenu *ui_createFromTemplateMenu;
    QToolButton *ui_openPublishedFileButton;
    QToolButton *ui_openPreviewFileButton;
    DuQFFolderDisplayWidget *ui_folderWidget;
    QComboBox *ui_difficultyBox;
    AutoSelectDoubleSpinBox *ui_estimationEdit;
    QCheckBox *ui_autoEstimationBox;
    QLabel *ui_estimationLabel;

    QSplitter *ui_commentSplitter;

    RamStatus *m_status;
};

#endif // STATUSEDITWIDGET_H
