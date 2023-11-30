#ifndef AUDIOTHREAD_H
#define AUDIOTHREAD_H

#include <QThread>
#include "log.h"
#include <sstream>

class AudioThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioThread(int isDemux,QString filePath, QObject *parent = nullptr);
    bool audiostart=true;
protected:
    void run();
private:
    int m_isDemux=0;
    QString m_filePath;
    Log m_log;
    std::ostringstream logInfo;
    std::string strLogInfo;
signals:
    void mfinish();


};

#endif // AUDIOTHREAD_H
