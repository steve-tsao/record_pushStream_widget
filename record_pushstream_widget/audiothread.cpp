#include "audiothread.h"
#include "framebuf.h"
#include <qDebug>
#include <QMutex>
#include <QQueue>
#include <QDateTime>
#include <QSemaphore>
#include <algorithm>


extern"C"
{
#include "libavdevice/avdevice.h"
#include "libavcodec/avcodec.h"
}

extern QMutex audioMutex;
extern QMutex videoMutex;
extern QSemaphore afreeSpace;
extern QSemaphore ausedSpace;
extern QMutex audioQueueMutex;
extern QQueue<AFrameBuf*> audioQueue;


AudioThread::AudioThread(int isDemux,QString filePath,QObject *parent)
    : QThread{parent},m_isDemux{isDemux},m_filePath{filePath}
{
    m_filePath+="/";
    QDateTime dateTime= QDateTime::currentDateTime();
    m_filePath+= dateTime .toString("yyyy-MM-dd-hh-mm-ss");
    m_filePath+=".pcm";
}

void AudioThread::run()
{
    qDebug()<<"audio thread is running";
    m_log.writeLog("**************************audio thread is running*******************************************");

    audioMutex.lock();

    //注册所有的设备
    avdevice_register_all();

    //查找openal或dshow设备，dshow需要安装插件https://github.com/rdp/screen-capture-recorder-to-video-windows-free/releases/tag/0.13.1
    const AVInputFormat* ifmt=av_find_input_format("dshow");
    if(ifmt==NULL)
    {
        qDebug()<<"[error!] Couldn't find audio input format.";
        m_log.writeLog("[error!] Couldn't find audio input format.");
    }

    //音视频格式上下文，包含了读取、解析和处理音视频文件所需的各种信息和数据
    AVFormatContext *pAudioFormatCtx = avformat_alloc_context();
    if(pAudioFormatCtx==NULL)
    {
        qDebug()<<"[error!] Couldn't alloc audio Format Ctx.";
        m_log.writeLog("[error!] Couldn't alloc audio Format Ctx.");
    }

    //打开设备
    if(avformat_open_input(&pAudioFormatCtx,"audio=virtual-audio-capturer",ifmt,NULL))
    {
        qDebug()<<"[error!] Couldn't open audio input stream.";
        m_log.writeLog("[error!] Couldn't open audio input stream.");
    }

    //获取音频流
    if(avformat_find_stream_info(pAudioFormatCtx,NULL)<0)
    {
        qDebug()<<"[error!] Couldn't find audio stream information.";
        m_log.writeLog("[error!] Couldn't find audio stream information.");
    }

    //查找音频流
    int audioindex=-1;
    for (unsigned int i = 0; i < pAudioFormatCtx->nb_streams; i++)
    {
        if (pAudioFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioindex = i;
            break;
        }
    }
    if (audioindex==-1)
    {
        qDebug()<<"[error!] Couldn't find a audio stream.";
        m_log.writeLog("[error!] Couldn't find a audio stream.");
    }

    //解码音频流
    //AVCodecParameters结构体包含音视频编解码器的参数信息
    AVCodecParameters* audioCodecParameters=pAudioFormatCtx->streams[audioindex]->codecpar;

    //根据参数信息获取编码器，AVCodec结构体包含了编解码器的各种属性和方法，用于实现音视频的编码和解码功能
    const AVCodec* audioCodec=avcodec_find_decoder(audioCodecParameters->codec_id);
    if (!audioCodec)
    {
        qDebug()<<"[error!] Couldn't found audio Codec.";
        m_log.writeLog("[error!] Couldn't found audio Codec.");
    }

    //AVCodecContext结构体包含了编解码器的参数、状态以及相关的信息，用于配置和控制编解码过程
    AVCodecContext *audioCodecContext = avcodec_alloc_context3(audioCodec);
    if (!audioCodecContext) {
        qDebug()<<"[error!] Couldn't alloc audio avcodec.";
        m_log.writeLog("[error!] Couldn't alloc audio avcodec.");
    }

    //根据参数信息填充编解码器上下文
    if (avcodec_parameters_to_context(audioCodecContext, audioCodecParameters) < 0)
    {
        qDebug()<<"[error!] Couldn't fill parameters to audio codec context.";
        m_log.writeLog("[error!] Couldn't fill parameters to audio codec context.");
    }

    //根据编解码器初始化编解码器上下文
    if (avcodec_open2(audioCodecContext, audioCodec, NULL) < 0)
    {
        qDebug()<<"[error!] Couldn't initialize the audio AVCodecContext.";
        m_log.writeLog("[error!] Couldn't initialize the audio AVCodecContext.");
    }

    qDebug()<<"[audio stream info] bit_rate_"<<audioCodecContext->bit_rate<<" sample_rate_"<<audioCodecContext->sample_rate<<" sample_channels_"<<audioCodecContext->ch_layout.nb_channels;
    logInfo<<"[audio stream info] bit_rate_"<<audioCodecContext->bit_rate<<" sample_rate_"<<audioCodecContext->sample_rate<<" sample_channels_"<<audioCodecContext->ch_layout.nb_channels;
    strLogInfo=logInfo.str();
    m_log.writeLog(strLogInfo);
    logInfo.str("");
    strLogInfo.clear();

    //音频帧
    AVFrame *aFrame=av_frame_alloc();
    AVPacket apacket;

    //如果解复用创建pcm文件
    FILE *fp_pcm;
    if(m_isDemux==1)
    {
        QByteArray tmpFileName = m_filePath.toLatin1();
        const char* filename=tmpFileName;
        fp_pcm=fopen(filename,"wb");
        if(fp_pcm==nullptr)
        {
            qDebug()<<"[error!] Couldn't open pcm file.";
            m_log.writeLog("[error!] Couldn't open pcm file.");
        }
    }

    int totalFrame=0;
    audioMutex.unlock();


    videoMutex.lock();

    clock_t start,end;
    start = clock();
    qDebug()<<"[audio record start time] "<<double(start)/CLOCKS_PER_SEC<<"s";
    logInfo<<"[audio record start time] "<<double(start)/CLOCKS_PER_SEC<<"s";
    strLogInfo=logInfo.str();
    m_log.writeLog(strLogInfo);
    logInfo.str("");
    strLogInfo.clear();

    while (1)
    {
        //获取音频流的下一帧
        if(av_read_frame(pAudioFormatCtx, &apacket) <0)
        {
            qDebug()<<"[error!] Couldn't read audio frame.";
            m_log.writeLog("[error!] Couldn't read audio frame.");
        }

        if (apacket.stream_index == audioindex)
        {
            //发送给解码器
            if (avcodec_send_packet(audioCodecContext, &apacket) < 0)
            {
                qDebug()<<"[error!] Couldn't send to audio codec.";
                m_log.writeLog("[error!] Couldn't send to audio codec.");
            }

            //从解码器获取解码帧
            if(avcodec_receive_frame(audioCodecContext, aFrame) <0)
            {
                qDebug()<<"[error!] Couldn't receive frame from audio codec.";
                m_log.writeLog("[error!] Couldn't receive frame from audio codec.");
            }

            //申请音频队列节点
            AFrameBuf* pAFrameBuf=new AFrameBuf;
            if(pAFrameBuf==nullptr)
            {
                qDebug()<<"[error!] Couldn't alloc audio queue node.";
                m_log.writeLog("[error!] Couldn't alloc audio queue node.");
                exit(1);
            }

            //设置节点大小，返回值单位是字节
            pAFrameBuf->pcmSize = aFrame->linesize[0];
            //av_samples_get_buffer_size(NULL,audioCodecContext->ch_layout.nb_channels, aFrame->nb_samples,audioCodecContext->sample_fmt, 1);
            //qDebug()<<"send audio size:"<<pAFrameBuf->pcmSize;

            //如果解复用写入pcm文件
            if(m_isDemux==1)
                fwrite(aFrame->data[0],1,pAFrameBuf->pcmSize,fp_pcm);

            //申请队列节点音频帧缓冲区
            pAFrameBuf->paFrame=new unsigned char[pAFrameBuf->pcmSize];
            if(pAFrameBuf->paFrame==NULL)
            {
                qDebug()<<"[error!] Couldn't alloc audio Frame queueBuf.";
                m_log.writeLog("[error!] Couldn't alloc audio Frame queueBuf.");
            }

            if(memcpy_s(pAFrameBuf->paFrame,pAFrameBuf->pcmSize,aFrame->data[0],pAFrameBuf->pcmSize)!=0)
            {
                qDebug()<<"[error!] Couldn't copy aFrame->data[0] to pAFrameBuf->paFrame.";
                m_log.writeLog("[error!] Couldn't copy aFrame->data[0] to pAFrameBuf->paFrame.");
            }


            audioQueueMutex.lock();
            //afreeSpace.acquire();
            audioQueue.enqueue(pAFrameBuf);
            //ausedSpace.release();
            audioQueueMutex.unlock();
            totalFrame++;
        }
        av_packet_unref(&apacket);
        if(audiostart==false)
        {
            break;
        }
    }

    //计时结束
    end = clock();
    qDebug()<<"[audio record finish time] "<<double(end)/CLOCKS_PER_SEC<<"s";
    logInfo<<"[audio record finish time] "<<double(end)/CLOCKS_PER_SEC<<"s";
    strLogInfo=logInfo.str();
    m_log.writeLog(strLogInfo);
    logInfo.str("");
    strLogInfo.clear();
    qDebug()<<"[audio record total time] "<<double(end-start)/CLOCKS_PER_SEC<<"s";
    logInfo<<"[audio record total time] "<<double(end-start)/CLOCKS_PER_SEC<<"s";
    strLogInfo=logInfo.str();
    m_log.writeLog(strLogInfo);
    logInfo.str("");
    strLogInfo.clear();
    qDebug()<<"[audio stream total frame] "<<totalFrame;
    logInfo<<"[audio stream total frame] "<<totalFrame;
    strLogInfo=logInfo.str();
    m_log.writeLog(strLogInfo);
    logInfo.str("");
    strLogInfo.clear();

    //释放空间
    if(m_isDemux==1)
        fclose(fp_pcm);
    av_frame_free(&aFrame);
    avcodec_free_context(&audioCodecContext);
    avformat_close_input(&pAudioFormatCtx);
    avformat_free_context(pAudioFormatCtx);

    videoMutex.unlock();
    emit mfinish();
}
