#include "widget.h"
#include "ui_widget.h"
#include <qmutex>
#include <QQueue>
#include <QFileDialog>
#include <QDesktopServices>
#include <QDateTime>
#include <QMessageBox>
#include <QTimer>
#include "framebuf.h"
#include <QSemaphore>
#include <QButtonGroup>


QMutex audioMutex;
QMutex videoMutex;
QMutex logMutex;
QMutex videoQueueMutex;
QMutex audioQueueMutex;
QSemaphore vfreeSpace(INT_MAX);//INT_MAX
QSemaphore vusedSpace(0);
QSemaphore afreeSpace(INT_MAX);
QSemaphore ausedSpace(0);
QQueue<VFrameBuf*> videoQueue;
QQueue<AFrameBuf*> audioQueue;


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    //radioButton分组23互斥
    QButtonGroup *block=new QButtonGroup(this);
    block->addButton(ui->radioButton_2,0);
    block->addButton(ui->radioButton_3,1);

    //推流地址初始化
    ui->lineEdit_2->setText("rtsp://127.0.0.1:8554/stream");

    //保存路径初始化
    QString curPath=QCoreApplication::applicationDirPath();
    ui->lineEdit->setText(curPath);

    //帧率设置初始化
    ui->spinBox->setMaximum(30);
    ui->spinBox->setMinimum(1);
    ui->spinBox->setSuffix(" fps");
    ui->spinBox->setValue(30);

    //码率设置初始化
    ui->horizontalSlider_2->setValue(4000);
}

Widget::~Widget()
{
    delete ui;
}




void Widget::on_pushButton_clicked()
{
    if( ui->pushButton->text()=="开始")
    {
        if(ui->radioButton_2->isChecked()||ui->radioButton_3->isChecked())
        {
            //确保所有线程已释放
            if(m_videoThread!=nullptr||m_audioThread!=nullptr||m_muxThread!=nullptr)
                return;

            //修改按钮为结束
            ui->pushButton->setText("结束");

            //获取当前路径以及时间拼接为包含路径的完整文件名称
            QString filename=ui->lineEdit->text();
            filename+="/";
            QDateTime dateTime= QDateTime::currentDateTime();
            filename+= dateTime .toString("yyyy-MM-dd-hh-mm-ss");
            filename+=".";
            filename+=ui->comboBox->currentText();

            //构造音视频线程
            //参数：是否解复用，保存路径，帧率
            m_audioThread=new AudioThread(ui->radioButton->isChecked(),ui->lineEdit->text(),this);
            m_videoThread=new VideoThread(ui->radioButton->isChecked(),ui->lineEdit->text(),ui->spinBox->value(),this);

            //构造复用线程
            //参数：录制或推流，推流地址，帧率，码率，文件名
            m_muxThread=new  MuxThread(ui->radioButton_2->isChecked(),ui->lineEdit_2->text(), ui->spinBox->value(),ui->horizontalSlider_2->value(),filename,this);

            //连接线程结束信号槽
            connect(m_muxThread,&MuxThread::mfinish,this,&Widget::solt_freeMuxThread);
            connect(m_audioThread,&AudioThread::mfinish,this,&Widget::solt_freeAudioThread);
            connect(m_videoThread,&VideoThread::mfinish,this,&Widget::solt_freeVideoThread);

            //启动线程
            m_muxThread->start();
            m_audioThread->start();
            m_videoThread->start();

            //构造时间对象，连接刷新时间信号槽，开始记时
            timer=new QTimer(this);
            connect(timer,SIGNAL(timeout()),this,SLOT(solt_msTimerUpdata()));
            timer->start(1);
        }
        else
        {
            QMessageBox::information(this,"提示","请选择录制或推流");
        }

    }
    else//表示当前按钮为结束
    {
        ui->pushButton->setText("开始");

        //通知线程结束
        m_audioThread->audiostart=false;
        m_videoThread->videoStart=false;
        m_muxThread->muxStart=false;

        //重置计时器
        timer->stop();
        msTime=0;
        sTime=0;
        minTime=0;
        ui->lcdNumber->display(0);
        ui->lcdNumber_2->display(0);
        ui->lcdNumber_3->display(0);
    }
}

void Widget::solt_freeAudioThread()
{
    delete m_audioThread;
    m_audioThread=nullptr;
}

void Widget::solt_freeVideoThread()
{
    delete m_videoThread;
    m_videoThread=nullptr;
}

void Widget::solt_freeMuxThread(int isRecord)
{
    delete m_muxThread;
    m_muxThread=nullptr;

    if(isRecord==1)
    {
        //复用完成弹窗提示
        QString info="录像已保存至";
        info+=ui->lineEdit->text();
        QMessageBox::information(this,"提示",info);
    }
    else
    {
        QMessageBox::information(this,"提示","已结束推流");
    }

}

//修改目录
void Widget::on_pushButton_2_clicked()
{
    QString curPath=QCoreApplication::applicationDirPath();
    QString dlgTitle="选择一个目录";
    QString selectDir=QFileDialog::getExistingDirectory(this,dlgTitle,curPath);
    if(!selectDir.isEmpty())
    {
        ui->lineEdit->setText(selectDir);
    }
}

//浏览当前目录
void Widget::on_pushButton_3_clicked()
{
    QString path="file:";
    path+=ui->lineEdit->text();
    path.replace("/", "\\");
    QDesktopServices::openUrl(QUrl(path, QUrl::TolerantMode));
}

//刷新计时器
void Widget::solt_msTimerUpdata()
{
    if(msTime<999)
    {
        ui->lcdNumber->display(++msTime);
    }
    else
    {
        msTime=0;
        ui->lcdNumber->display(msTime);
        if(sTime<59)
        {
            ui->lcdNumber_2->display(++sTime);

        }
        else
        {
            sTime=0;
            ui->lcdNumber_2->display(sTime);
            ui->lcdNumber_3->display(++minTime);
        }
    }

}

//解复用
void Widget::on_radioButton_clicked()
{
    QMessageBox::StandardButton result=QMessageBox::Yes;
    if(ui->radioButton->isChecked())
        result=QMessageBox::question(this,"提示","解复用将输出未经压缩的pcm 48000HZ 16bit 音频和yuv 420p 视频占用大量磁盘空间！");
    if(result==QMessageBox::No)
        ui->radioButton->click();
}

