#pragma once

#ifndef WIDGET_H
#define WIDGET_H

#include "qobjectdefs.h"
#include <QWidget>
#include "audiothread.h"
#include "videothread.h"
#include "muxthread.h"
#include "log.h"



QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget* parent = nullptr);
    ~Widget();

private slots:
    void on_pushButton_clicked();
    void solt_freeAudioThread();
    void solt_freeVideoThread();
    void solt_freeMuxThread(int isRecord);
    void solt_msTimerUpdata();


    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_radioButton_clicked();

private:
    Ui::Widget* ui;
    AudioThread* m_audioThread = nullptr;
    VideoThread* m_videoThread = nullptr;
    MuxThread* m_muxThread = nullptr;
    int msTime = 0;
    int sTime = 0;
    int minTime = 0;
    QTimer* timer;
    Log m_log;
};
#endif // WIDGET_H

