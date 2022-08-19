#ifndef RAMSTATUS_H
#define RAMSTATUS_H

#include <QDateTime>

#include "ramobject.h"

class RamAbstractItem;
class RamUser;
class RamStep;
class RamState;
class RamWorkingFolder;

class RamStatus : public RamObject
{
    Q_OBJECT
public:

    // ENUMS //

    enum Difficulty {
        VeryEasy = 0,
        Easy = 1,
        Medium = 2,
        Hard = 3,
        VeryHard = 4
    };
    Q_ENUM(Difficulty)

    // STATIC METHODS //

    static RamStatus *get(QString uuid);
    static RamStatus *c(RamObject *o);

    static RamStatus *copy(RamStatus *other, RamUser *user);

    static RamStatus *noStatus(RamAbstractItem *item, RamStep *step);

    static float hoursToDays(int hours);
    static int daysToHours(float days);

    // METHODS //

    RamStatus(RamUser *user, RamAbstractItem *item, RamStep *step, bool isVirtual = false);

    RamUser *user() const;
    RamStep *step() const;
    RamAbstractItem *item() const;

    bool isNoState() const;

    int completionRatio() const;
    void setCompletionRatio(int completionRatio);

    RamState *state() const;
    void setState(RamState *newState);

    int version() const;
    void setVersion(int version);

    QDateTime date() const;
    void setDate(const QDateTime &date);

    bool isPublished() const;
    void setPublished(bool published);

    RamUser *assignedUser() const;
    void assignUser(RamObject *assignedUser);

    qint64 timeSpent() const; // seconds
    void setTimeSpent(const float &ts);
    bool isTimeSpentManual() const;

    Difficulty difficulty() const;
    void setDifficulty(Difficulty newDifficulty);

    float goal() const; // days
    void setGoal(float newGoal);

    float estimation() const; // days
    float estimation(int difficulty) const; // days

    void setUseAutoEstimation(bool newAutoEstimation);
    bool useAutoEstimation() const;

    float latenessRatio() const;

    RamWorkingFolder workingFolder() const;

    QString createFileFromTemplate(QString filePath) const;
    QString createFileFromResource(QString filePath) const;

    QString restoreVersionFile(QString fileName) const;

    virtual QString details() const override;
    virtual QString subDetails() const override;

public slots:
    virtual void edit(bool show = true) override;

protected:
    static QMap<QString, RamStatus*> m_existingObjects;
    RamStatus(QString uuid);
    virtual QString folderPath() const override;

private slots:
    void stateRemoved();
    void userRemoved();
    void assignedUserRemoved();

private:  
    void construct();
    void connectEvents();

    RamUser *m_user;
    RamAbstractItem *m_item;
    RamStep *m_step;
};

#endif // RAMSTATUS_H
