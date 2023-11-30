#include "videothread.h"
#include <qDebug>
#include <QMutex>
#include <QQueue>
#include <QDateTime>
#include "framebuf.h"
#include <QSemaphore>
#include <windows.h>



extern"C"
{
#include "libavdevice/avdevice.h"
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}


extern QMutex audioMutex;
extern QMutex videoMutex;
extern QSemaphore vfreeSpace;
extern QSemaphore vusedSpace;
extern QMutex videoQueueMutex;
extern QQueue<VFrameBuf*> videoQueue;


VideoThread::VideoThread(int isDemux,QString filePath,int fps,QObject *parent):QThread(parent),m_fps{fps},m_isDemux{isDemux},m_filePath{filePath}
{
    m_filePath+="/";
    QDateTime dateTime= QDateTime::currentDateTime();
    m_filePath+= dateTime .toString("yyyy-MM-dd-hh-mm-ss");
    m_filePath+=".yuv";
}

void VideoThread::run()
{
    qDebug()<<"video thread is running";
    m_log.writeLog("**************************video thread is running*******************************************");

    videoMutex.lock();
    //注册所有的设备
    avdevice_register_all();

    //查找gdigrab或dshow设备，dshow需要安装插件https://github.com/rdp/screen-capture-recorder-to-video-windows-free/releases/tag/0.13.1
    //AVInputFormat结构体定义了音视频文件的输入格式和相关的参数
    const AVInputFormat* ifmt=av_find_input_format("dshow");
    if(ifmt==NULL)
    {
        qDebug()<<"[error!] Couldn't find video input format.";
        m_log.writeLog("[error!] Couldn't find video input format.");
    }

    //AVFormatContext音视频格式上下文，包含了读取、解析和处理音视频文件所需的各种信息和数据
    AVFormatContext *pVideoFormatCtx = avformat_alloc_context();
    if(pVideoFormatCtx==NULL)
    {
        qDebug()<<"[error!] Couldn't alloc video Format Ctx.";
        m_log.writeLog("[error!] Couldn't alloc video Format Ctx.");
    }
    AVDictionary* options = NULL;

    //设置设备参数
    char fpsChar[3];
    itoa(m_fps, fpsChar, 10);
    av_dict_set(&options,"framerate",fpsChar,0);
    //距离屏幕左边缘的距离，仅gdigrab设备有效，dshow忽略
    //av_dict_set(&options,"offset_x","0",0);
    //距离屏幕上边缘的距离，仅gdigrab设备有效，dshow忽略
    // av_dict_set(&options,"offset_y","0",0);
    //画面尺寸，仅gdigrab设备有效，dshow忽略
    //av_dict_set(&options,"video_size","1920x1080",0);

    //gdigrab使用此参数打开设备，desktop可换成窗口名称指定窗口捕获
//    if(avformat_open_input(&pVideoFormatCtx,"desktop",ifmt,&options))
//    {
//        qDebug()<<"[error!] Couldn't open video input stream.";
//        m_log.writeLog("[error!] Couldn't open video input stream.");
//    }
    //dshow使用此参数打开设备
    if(avformat_open_input(&pVideoFormatCtx,"video=screen-capture-recorder",ifmt,&options))
    {
        qDebug()<<"[error!] Couldn't open video input stream.";
        m_log.writeLog("[error!] Couldn't open video input stream.");
    }

    //获取视频流
    if(avformat_find_stream_info(pVideoFormatCtx,NULL)<0)
    {
        qDebug()<<"[error!] Couldn't find video stream information.";
        m_log.writeLog("[error!] Couldn't find video stream information.");
    }

    //查找视频流
    int videoindex=-1;
    for (unsigned int i = 0; i < pVideoFormatCtx->nb_streams; i++)
    {
        if (pVideoFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    }
    if (videoindex==-1)
    {
        qDebug()<<"[error!] Couldn't find a video stream.";
        m_log.writeLog("[error!] Couldn't find a video stream.");
    }

    //解码视频流
    //AVCodecParameters结构体包含音视频编解码器的参数信息
    AVCodecParameters* videoCodecParameters=pVideoFormatCtx->streams[videoindex]->codecpar;

    //根据参数信息获取编码器，AVCodec结构体包含了编解码器的各种属性和方法，用于实现音视频的编码和解码功能
    const AVCodec* videoCodec=avcodec_find_decoder(videoCodecParameters->codec_id);
    if (!videoCodec)
    {
        qDebug()<<"[error!] Couldn't found video Codec.";
        m_log.writeLog("[error!] Couldn't found video Codec.");
    }

    //AVCodecContext结构体包含了编解码器的参数、状态以及相关的信息，用于配置和控制编解码过程
    AVCodecContext *videoCodecContext = avcodec_alloc_context3(videoCodec);
    if (!videoCodecContext)
    {
        qDebug()<<"[error!] Couldn't alloc video avcodec.";
        m_log.writeLog("[error!] Couldn't alloc video avcodec.");
    }

    //根据参数信息填充编解码器上下文
    if (avcodec_parameters_to_context(videoCodecContext, videoCodecParameters) < 0)
    {
        qDebug()<<"[error!] Couldn't fill parameters to video codec context.";
        m_log.writeLog("[error!] Couldn't fill parameters to video codec context.");
    }

    //根据编解码器初始化编解码器上下文
    if (avcodec_open2(videoCodecContext, videoCodec, NULL) < 0)
    {
        qDebug()<<"[error!] Couldn't initialize the video AVCodecContext.";
        m_log.writeLog("[error!] Couldn't initialize the video AVCodecContext.");
    }

    qDebug()<<"[video stream info] "<<"frame_rate_"<<pVideoFormatCtx->streams[videoindex]->r_frame_rate.num/pVideoFormatCtx->streams[videoindex]->r_frame_rate.den;
    logInfo<<"[video stream info] frame_rate_"<<pVideoFormatCtx->streams[videoindex]->r_frame_rate.num/pVideoFormatCtx->streams[videoindex]->r_frame_rate.den;
    strLogInfo=logInfo.str();
    m_log.writeLog(strLogInfo);
    logInfo.str("");
    strLogInfo.clear();

    //如果解复用创建yuv文件
    FILE *fp_yuv=nullptr;
    if(m_isDemux==1)
    {
        QByteArray tmpFileName = m_filePath.toLatin1();
        const char* filename=tmpFileName;
        fp_yuv=fopen(filename,"wb");
        if(fp_yuv==nullptr)
        {
            qDebug()<<"[error!] Couldn't open yuv file.";
            m_log.writeLog("[error!] Couldn't open yuv file.");
        }
    }

    //分配解码后视频帧，vFrame为图像缩放和格式转换前，vFrameYUV为图像缩放和格式转换为yuv420后
    AVFrame *vFrame,*vFrameYUV;
    vFrame=av_frame_alloc();
    if(vFrame==NULL)
    {
        qDebug()<<"[error!] Couldn't alloc video frame.";
        m_log.writeLog("[error!] Couldn't alloc video frame.");
    }
    vFrameYUV=av_frame_alloc();
    if(vFrameYUV==NULL)
    {
        qDebug()<<"[error!] Couldn't alloc video yuvframe.";
        m_log.writeLog("[error!] Couldn't alloc video yuvframe.");
    }

    //申请一张图片的空间
    uint8_t *out_buffer=(uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, videoCodecContext->width, videoCodecContext->height,1));
    if(out_buffer==NULL)
    {
        qDebug()<<"[error!] Couldn't alloc out buffer.";
        m_log.writeLog("[error!] Couldn't alloc out buffer.");
    }

    //将vFrameYUV中的data指向该图片空间，并将行大小关联起来
    if(av_image_fill_arrays(vFrameYUV->data, vFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, videoCodecContext->width, videoCodecContext->height,1)<0)
    {
        qDebug()<<"[error!] Couldn't fill vedio frame.";
        m_log.writeLog("[error!] Couldn't fill vedio frame.");
    }

    //图像色彩空间转换和图像缩放的上下文
    struct SwsContext *img_convert_ctx;
    img_convert_ctx = sws_getContext(videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt, videoCodecContext->width, videoCodecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
    if(img_convert_ctx==NULL)
    {
        qDebug()<<"[error!] Couldn't get swsContext.";
        m_log.writeLog("[error!] Couldn't get swsContext.");
    }

    //未解码视频数据结构
    AVPacket vpacket;
    int totalFrame=0;

    videoMutex.unlock();



    audioMutex.lock();

    //计时
    clock_t start,end;
    start = clock();
    qDebug()<<"[video record start time] "<<double(start)/CLOCKS_PER_SEC<<"s";
    logInfo<<"[video record start time] "<<double(start)/CLOCKS_PER_SEC<<"s";
    strLogInfo=logInfo.str();
    m_log.writeLog(strLogInfo);
    logInfo.str("");
    strLogInfo.clear();



    //循环接收视频编码帧然后解码
    while (1)
    {     
        //获取视频流的下一帧
        if(av_read_frame(pVideoFormatCtx, &vpacket)<0)
        {
            qDebug()<<"[error!] Couldn't read video frame.";
            m_log.writeLog("[error!] Couldn't read video frame.");
        }

        if (vpacket.stream_index == videoindex )
        {
            //发送给解码器
            if (avcodec_send_packet(videoCodecContext, &vpacket) < 0)
            {
                qDebug()<<"[error!] Couldn't send to video codec.";
                m_log.writeLog("[error!] Couldn't send to video codec.");
            }

            //从解码器获取解码帧
            if(avcodec_receive_frame(videoCodecContext, vFrame)<0)
            {
                qDebug()<<"[error!] Couldn't receive frame from video codec.";
                m_log.writeLog("[error!] Couldn't receive frame from video codec.");
            }

            //根据上下文和提供的源图像数据vFrame，进行图像缩放和格式转换操作，并将结果存储在目标图像数据vFrameYUV中
            if(sws_scale(img_convert_ctx, (const uint8_t* const*)vFrame->data, vFrame->linesize, 0, videoCodecContext->height, vFrameYUV->data, vFrameYUV->linesize)<0)
            {
                qDebug()<<"[error!] Couldn't scale frame .";
                m_log.writeLog("[error!] Couldn't scale frame .");
            }

            //帧像素总数
            int y_size=videoCodecContext->width*videoCodecContext->height;

            //如果解复用输出yuv
            if(m_isDemux==1)
            {
                fwrite(vFrameYUV->data[0],1,y_size,fp_yuv);    //Y
                fwrite(vFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
                fwrite(vFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
            }

            //否则将帧入队
            VFrameBuf* pVFrameBuf=new VFrameBuf;
            if(pVFrameBuf==nullptr)
            {
                qDebug()<<"[error!] Couldn't alloc Video Frame queueBuf.";
                m_log.writeLog("[error!] Couldn't alloc Video Frame queueBuf.");
                exit(1);
            }

            if(memcpy_s(pVFrameBuf->yChannel,sizeof(pVFrameBuf->yChannel),vFrameYUV->data[0],y_size)!=0)
            {
                qDebug()<<"[error!] Couldn't copy vFrameYUV->data[0] to pVFrameBuf->yChannel.";
                m_log.writeLog("[error!] Couldn't copy vFrameYUV->data[0] to pVFrameBuf->yChannel.");
            }

            if(memcpy_s(pVFrameBuf->uChannel,sizeof(pVFrameBuf->uChannel),vFrameYUV->data[1],y_size/4)!=0)
            {
                qDebug()<<"[error!] Couldn't copy vFrameYUV->data[1] to pVFrameBuf->uChannel.";
                m_log.writeLog("[error!] Couldn't copy vFrameYUV->data[1] to pVFrameBuf->uChannel.");
            }
            if(memcpy_s(pVFrameBuf->vChannel,sizeof(pVFrameBuf->uChannel),vFrameYUV->data[2],y_size/4)!=0)
            {
                qDebug()<<"[error!] Couldn't copy vFrameYUV->data[2] to pVFrameBuf->vChannel.";
                m_log.writeLog("[error!] Couldn't copy vFrameYUV->data[2] to pVFrameBuf->vChannel.");
            }

            videoQueueMutex.lock();
            //vfreeSpace.acquire();
            videoQueue.enqueue(pVFrameBuf);
            //vusedSpace.release();
            videoQueueMutex.unlock();
            totalFrame++;
        }
        av_packet_unref(&vpacket);
        if(videoStart==false)
            break;
    }

    //结束计时
    end = clock();
    qDebug()<<"[video record finish time] "<<double(end)/CLOCKS_PER_SEC<<"s";
    logInfo<<"[video record finish time] "<<double(end)/CLOCKS_PER_SEC<<"s";
    strLogInfo=logInfo.str();
    m_log.writeLog(strLogInfo);
    logInfo.str("");
    strLogInfo.clear();
    qDebug()<<"[video record total time] "<<double(end-start)/CLOCKS_PER_SEC<<"s";
    logInfo<<"[video record total time] "<<double(end-start)/CLOCKS_PER_SEC<<"s";
    strLogInfo=logInfo.str();
    m_log.writeLog(strLogInfo);
    logInfo.str("");
    strLogInfo.clear();
    qDebug()<<"[video stream total frame] "<<totalFrame;
    logInfo<<"[video stream total frame] "<<totalFrame;
    strLogInfo=logInfo.str();
    m_log.writeLog(strLogInfo);
    logInfo.str("");
    strLogInfo.clear();

    //释放空间
    if(m_isDemux==1)
        fclose(fp_yuv);
    av_frame_free(&vFrame);
    av_frame_free(&vFrameYUV);
    avcodec_free_context(&videoCodecContext);
    avformat_close_input(&pVideoFormatCtx);
    avformat_free_context(pVideoFormatCtx);

    audioMutex.unlock();
    emit mfinish();
}


