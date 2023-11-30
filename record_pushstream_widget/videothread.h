#ifndef VIDEOTHREAD_H
#define VIDEOTHREAD_H

#include <QThread>
#include "log.h"
#include <sstream>




class VideoThread : public QThread
{
    Q_OBJECT
public:
    VideoThread(int isDemux,QString filePath,int fps,QObject *parent = nullptr);
    bool videoStart=true;
protected:
    void run();
private:
    int m_fps=0;
    int m_isDemux=0;
    QString m_filePath;
    Log m_log;
    std::ostringstream logInfo;
    std::string strLogInfo;


signals:
    void mfinish();
};

#endif // VIDEOTHREAD_H
