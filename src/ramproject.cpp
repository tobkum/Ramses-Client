#include "ramproject.h"

RamProject::RamProject(QString shortName, QString name, QString uuid, QObject *parent):
    RamObject(shortName, name, uuid, parent)
{
    _dbi->createProject(_shortName, _name, _uuid);
}

RamProject::~RamProject()
{
    _dbi->removeProject(_uuid);
}

QString RamProject::folderPath() const
{
    return _folderPath;
}

void RamProject::setFolderPath(const QString &folderPath)
{
    _folderPath = folderPath;
    emit changed(this);
}

void RamProject::update()
{
    QString path = _folderPath;
    if (path == "") path = "auto";
    _dbi->updateProject(_uuid, _shortName, _name, path);
}

void RamProject::stepRemoved(RamObject *o)
{
    removeStep(o);
}

void RamProject::assetGroupRemoved(RamObject *o)
{
    removeAssetGroup(o);
}

void RamProject::sequenceRemoved(RamObject *o)
{
    removeSequence(o);
}

QList<RamAssetGroup *> RamProject::assetGroups() const
{
    return _assetGroups;
}

RamAssetGroup *RamProject::assetGroup(QString uuid)
{
    foreach (RamAssetGroup *ag, _assetGroups)
    {
        if (ag->uuid() == uuid) return ag;
    }
    return nullptr;
}

void RamProject::addAssetGroup(RamAssetGroup *assetGroup)
{
    _assetGroups << assetGroup;
    connect(assetGroup, SIGNAL(removed(RamObject*)), this, SLOT(assetGroupRemoved(RamObject*)));
    emit newAssetGroup(assetGroup);
}

void RamProject::assignAssetGroup(RamAssetGroup *templateAssetGroup)
{
    RamAssetGroup *assetGroup = templateAssetGroup->createFromTemplate(_uuid);
    addAssetGroup(assetGroup);
}

void RamProject::createAssetGroup(QString shortName, QString name)
{
    RamAssetGroup *assetGroup = new RamAssetGroup(shortName, name, _uuid);
    addAssetGroup(assetGroup);
}

void RamProject::removeAssetGroup(QString uuid)
{
    for (int i = _assetGroups.count() -1; i >= 0; i--)
    {
        RamAssetGroup *ag = _assetGroups[i];
        if (ag->uuid() == uuid)
        {
            _assetGroups.removeAt(i);
            ag->remove();
            emit assetGroupRemoved(uuid);
        }
    }
}

void RamProject::removeAssetGroup(RamObject *assetGroup)
{
    removeAssetGroup(assetGroup->uuid());
}

bool assetGroupSorter(RamAssetGroup *a, RamAssetGroup *b)
{
    return a->shortName() < b->shortName();
}

void RamProject::sortAssetGroups()
{
    std::sort(_assetGroups.begin(), _assetGroups.end(), assetGroupSorter);
}

RamAsset *RamProject::asset(QString uuid)
{
    foreach(RamAssetGroup *ag, _assetGroups)
    {
        foreach(RamAsset *a, ag->assets())
        {
            if (a->uuid() == uuid) return a;
        }
    }

    return nullptr;
}

void RamProject::removeAsset(QString uuid)
{
    foreach(RamAssetGroup *ag, _assetGroups) ag->removeAsset(uuid);
}

QList<RamSequence *> RamProject::sequences() const
{
    return _sequences;
}

RamSequence *RamProject::sequence(QString uuid)
{
    foreach (RamSequence *s, _sequences)
    {
        if (s->uuid() == uuid) return s;
    }
    return nullptr;
}

void RamProject::addSequence(RamSequence *seq)
{
    _sequences << seq;
    connect(seq, SIGNAL(removed(RamObject*)), this, SLOT(sequenceRemoved(RamObject*)));
    emit newSequence(seq);
}

void RamProject::createSequence(QString shortName, QString name)
{
    RamSequence *seq = new RamSequence(shortName, name, _uuid);
    addSequence(seq);
}

void RamProject::removeSequence(QString uuid)
{
    for (int i = _sequences.count() -1; i >= 0; i--)
    {
        RamSequence *s = _sequences[i];
        if (s->uuid() == uuid)
        {
            _sequences.removeAt(i);
            emit sequenceRemoved(s);
            s->remove();
        }
    }
}

void RamProject::removeSequence(RamObject *seq)
{
    removeSequence(seq->uuid());
}

bool sequenceSorter(RamSequence *a, RamSequence *b)
{
    return a->shortName() < b->shortName();
}

void RamProject::sortSequences()
{
    std::sort(_sequences.begin(), _sequences.end(), sequenceSorter);
}

QList<RamPipe *> RamProject::pipeline()
{
    return _pipeline;
}

RamPipe *RamProject::pipe(QString uuid)
{
    foreach(RamPipe *pipe, _pipeline)
        if (pipe->uuid() == uuid) return pipe;

    return nullptr;
}

RamPipe *RamProject::pipe(RamStep *o, RamStep *i)
{
    foreach(RamPipe *p, _pipeline)
        if (p->outputStep()->uuid() == o->uuid() && p->inputStep()->uuid() == i->uuid()) return p;
    return nullptr;
}

RamPipe *RamProject::createPipe(RamStep *output, RamStep *input)
{
    RamPipe *p = new RamPipe(output, input);
    addPipe(p);
    return p;
}

void RamProject::addPipe(RamPipe *pipe)
{
    pipe->setProjectUuid( _uuid );
    _pipeline << pipe;
    emit newPipe(pipe);
}

void RamProject::removePipe(QString uuid)
{
    for(int i = _pipeline.count(); i >= 0; i--)
    {
        RamPipe *p = _pipeline.at(i);
        if (p->uuid() == uuid)
        {
            _pipeline.removeAt(i);
            emit pipeRemoved(p);
            p->remove();
        }
    }
}

void RamProject::removePipe(RamPipe *pipe)
{
    removePipe(pipe->uuid());
}

QList<RamStep *> RamProject::steps() const
{
    return _steps;
}

RamStep *RamProject::step(QString uuid) const
{
    foreach (RamStep *s, _steps)
    {
        if (s->uuid() == uuid) return s;
    }
    return nullptr;
}

void RamProject::addStep(RamStep *step)
{
    _steps << step;
    connect(step,SIGNAL(removed(RamObject*)), this,SLOT(stepRemoved(RamObject*)));
    emit newStep(step);
}

void RamProject::assignStep(RamStep *templateStep)
{
    RamStep *step = templateStep->createFromTemplate(_uuid);
    addStep(step);
}

void RamProject::createStep(QString shortName, QString name)
{
    RamStep *step = new RamStep(shortName, name, _uuid);
    addStep(step);
}

void RamProject::removeStep(QString uuid)
{
    for (int i = _steps.count() -1; i >= 0; i--)
    {
        RamStep *s = _steps[i];
        if (s->uuid() == uuid)
        {
            _steps.removeAt(i);
            s->remove();
            emit stepRemoved(uuid);
        }
    }
}

void RamProject::removeStep(RamObject *step)
{
    removeStep(step->uuid());
}

bool stepsSorter( RamStep *a, RamStep *b)
{
    return a->order() < b->order();
}

void RamProject::sortSteps()
{
    std::sort(_steps.begin(), _steps.end(), stepsSorter);
}
