#ifndef PROCREAPER_H
#define PROCREAPER_H
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <set>

class ProcReaper : public QThread
{
public:
    ProcReaper();
    ~ProcReaper();
public:
    virtual void run() override;
    void stop(const std::set<int64_t> & excludedPids);
private:
    bool mShouldRun;
    QMutex mMutex;
    QWaitCondition mWait;
};

#endif // PROCREAPER_H
