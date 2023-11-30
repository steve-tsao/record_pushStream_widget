#include "muxthread.h"
#include "framebuf.h"
#include <qDebug>
#include <QMutex>
#include <QQueue>
#include <QTime>
#include <QSemaphore>
#include <windows.h>



extern QMutex videoQueueMutex;
extern QMutex audioQueueMutex;
extern QSemaphore vfreeSpace;
extern QSemaphore vusedSpace;
extern QSemaphore afreeSpace;
extern QSemaphore ausedSpace;
extern QQueue<VFrameBuf*> videoQueue;
extern QQueue<AFrameBuf*> audioQueue;

MuxThread::MuxThread(int isRecord,QString pushAddr,int fps,int vedioBitRate,QString outVideoFileName,QObject *parent)
    : QThread{parent},m_outVideoFileName{outVideoFileName},m_fps{fps},m_vedioBitRate{vedioBitRate},m_isRecord{isRecord},m_pushAddr{pushAddr}
{

}

void MuxThread::run()
{
    qDebug()<<"mux thread is running";
    m_log.writeLog("**************************mux thread is running*******************************************");

    OutputStream video_st = { 0 }, audio_st = { 0 };
    const AVOutputFormat* fmt;
    QByteArray tmpFileName = m_outVideoFileName.toLatin1();
    const char* filename=tmpFileName;
    QByteArray tmpPushAddr = m_pushAddr.toLatin1();
    const char* cPushAddr=tmpPushAddr;
    AVFormatContext* oc;
    const AVCodec* audio_codec=NULL, * video_codec = NULL;
    int ret;
    int have_video = 0, have_audio = 0;
    int encode_video = 0, encode_audio = 0;
    AVDictionary* opt = NULL;

    /* allocate the output media context */
    if(m_isRecord==1)
    {
        avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    }
    else
    {
        avformat_alloc_output_context2(&oc, NULL, "rtsp", cPushAddr);
    }


    if (!oc)
    {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        qDebug()<<"[mux error!] Could not deduce output format from file extension: using MPEG.";
        m_log.writeLog("[mux error!] Could not deduce output format from file extension: using MPEG.");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    if (!oc)
    {
        printf("Could not avformat_alloc_output_context2");
        qDebug()<<"[mux error!] Could not avformat_alloc_output_context2";
        m_log.writeLog("[mux error!] Could not avformat_alloc_output_context2");
        exit(1);
    }

    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
         * and initialize the codecs. */
     if (fmt->video_codec != AV_CODEC_ID_NONE) {
        add_stream(&video_st, oc, &video_codec, fmt->video_codec);
        have_video = 1;
        encode_video = 1;
    }
    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
        add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
        have_audio = 1;
        encode_audio = 1;
    }

    /* Now that all the parameters are set, we can open the audio and
         * video codecs and allocate the necessary encode buffers. */
    if (have_video)
        open_video(oc, video_codec, &video_st, opt);

    if (have_audio)
        open_audio(oc, audio_codec, &audio_st, opt);

    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE))
    {
        if(m_isRecord==1)
        {
            ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        }
        else
        {
             ret = avio_open(&oc->pb, cPushAddr, AVIO_FLAG_WRITE);
        }

        if (ret < 0)
        {
            fprintf(stderr, "Could not open '%s': %s\n", filename,av_err2str(ret));
            qDebug()<<"[mux error!] Could not open "<<filename<<": "<<av_err2str(ret);
            logInfo<<"[mux error!] Could not open "<<filename<<": "<<av_err2str(ret);
            strLogInfo=logInfo.str();
            m_log.writeLog(strLogInfo);
            logInfo.str("");
            strLogInfo.clear();
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0)
    {
        fprintf(stderr, "Error occurred when opening output file: %s\n",av_err2str(ret));
        qDebug()<<"[mux error!] Error occurred when opening output file:"<<av_err2str(ret);
        logInfo<<"[mux error!] Error occurred when opening output file:"<<av_err2str(ret);
        strLogInfo=logInfo.str();
        m_log.writeLog(strLogInfo);
        logInfo.str("");
        strLogInfo.clear();
    }

    int muxTotalFrame=0;
    while (encode_video || encode_audio)
    {
        //qDebug()<<video_st.next_pts/(float)video_st.enc->time_base.den<<audio_st.next_pts/(float)audio_st.enc->time_base.den;
        /* select the stream to encode */
        // -1 if `ts_a` is before `ts_b`
        // 1 if `ts_a` is after `ts_b`
        // 0 if they represent the same position
        if (encode_video &&
                (!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
                                                audio_st.next_pts, audio_st.enc->time_base) <= 0)) {
            encode_video = !write_video_frame(oc, &video_st);
        }
        else {
            encode_audio = !write_audio_frame(oc, &audio_st);
        }
        muxTotalFrame++;
    }
    qDebug()<<"[mux total frame] "<<muxTotalFrame;
    logInfo<<"[mux total frame] "<<muxTotalFrame;
    strLogInfo=logInfo.str();
    m_log.writeLog(strLogInfo);
    logInfo.str("");
    strLogInfo.clear();


    av_write_trailer(oc);

    /* Close each codec. */
    if (have_video)
        close_stream(oc, &video_st);
    if (have_audio)
        close_stream(oc, &audio_st);

    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);

    emit mfinish(m_isRecord);
}


void MuxThread::add_stream(OutputStream* ost, AVFormatContext* oc,const AVCodec** codec,enum AVCodecID codec_id)
{
    AVCodecContext* c;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        qDebug()<<"[mux error!] Could not find encoder for"<<avcodec_get_name(codec_id);
        logInfo<<"[mux error!] Could not find encoder for"<<avcodec_get_name(codec_id);
        strLogInfo=logInfo.str();
        m_log.writeLog(strLogInfo);
        logInfo.str("");
        strLogInfo.clear();
        exit(1);
    }

    ost->tmp_pkt = av_packet_alloc();
    if (!ost->tmp_pkt) {
        fprintf(stderr, "Could not allocate AVPacket\n");
        qDebug()<<"[mux error!] Could not allocate AVPacket.";
        m_log.writeLog("[mux error!] Could not allocate AVPacket.");
        exit(1);
    }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) {
        fprintf(stderr, "Could not allocate stream\n");
        qDebug()<<"[mux error!] Could not allocate stream.";
        m_log.writeLog("[mux error!] Could not allocate stream.");
        exit(1);
    }
    ost->st->id = oc->nb_streams - 1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        qDebug()<<"[mux error!] Could not alloc an encoding context.";
        m_log.writeLog("[mux error!] Could not alloc an encoding context.");
        exit(1);
    }
    ost->enc = c;
    AVChannelLayout tempAVChannelLayout=AV_CHANNEL_LAYOUT_STEREO;
    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt = (*codec)->sample_fmts ?
                    (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        //c->sample_fmt = AV_SAMPLE_FMT_S16;
        c->bit_rate = 64000;
        c->sample_rate = AUDIO_SAMPLE_RATE;
        if ((*codec)->supported_samplerates) {
            c->sample_rate = (*codec)->supported_samplerates[0];
            for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == AUDIO_SAMPLE_RATE)
                    c->sample_rate = AUDIO_SAMPLE_RATE;
            }
        }
        if(av_channel_layout_copy(&c->ch_layout, &tempAVChannelLayout)!=0)
        {
            qDebug()<<"[mux error!] Could not av_channel_layout_copy.";
            m_log.writeLog("[mux error!] Could not av_channel_layout_copy.");
            exit(1);
        }
        ost->st->time_base = { 1, c->sample_rate };
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;

        c->bit_rate = m_vedioBitRate*1000;
        c->bit_rate_tolerance = 0;
        /* Resolution must be a multiple of two. */
        c->width = WIDTH;
        c->height = HEIGHT;
       
        /* timebase: This is the fundamental unit of time (in seconds) in terms
             * of which frame timestamps are represented. For fixed-fps content,
             * timebase should be 1/framerate and timestamp increments should be
             * identical to 1. */
        ost->st->time_base = { 1, m_fps };
        c->time_base = ost->st->time_base;

        c->gop_size = 12; /* emit one intra frame every twelve frames at most */
        c->pix_fmt = STREAM_PIX_FMT;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* just for testing, we also add B-frames */
            c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
                 * This does not happen with normal video, it just happens here as
                 * the motion of the chroma plane does not match the luma plane. */
            c->mb_decision = 2;
        }
        break;

    default:
        break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

void MuxThread::open_video(AVFormatContext* oc, const AVCodec* codec,OutputStream* ost, AVDictionary* opt_arg)
{
    int ret;
    AVCodecContext* c = ost->enc;
    AVDictionary* opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
        qDebug()<<"[mux error!] Could not open video codec:"<<av_err2str(ret);
        logInfo<<"[mux error!] Could not open video codec:"<<av_err2str(ret);
        strLogInfo=logInfo.str();
        m_log.writeLog(strLogInfo);
        logInfo.str("");
        strLogInfo.clear();
        exit(1);
    }

    /* allocate and init a re-usable frame */
    ost->frame = alloc_frame(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        qDebug()<<"[mux error!] Could not allocate video frame.";
        m_log.writeLog("[mux error!] Could not allocate video frame.");
        exit(1);
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ost->tmp_frame = alloc_frame(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ost->tmp_frame) {
            fprintf(stderr, "Could not allocate temporary video frame\n");
            qDebug()<<"[mux error!] Could not allocate temporary video frame.";
            m_log.writeLog("[mux error!] Could not allocate temporary video frame.");
            exit(1);
        }
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        qDebug()<<"[mux error!] Could not copy the stream parameters.";
        m_log.writeLog("[mux error!] Could not copy the stream parameters.");
        exit(1);
    }
}


AVFrame* MuxThread::alloc_frame(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame* frame;
    int ret;

    frame = av_frame_alloc();
    if (!frame)
        return NULL;

    frame->format = pix_fmt;
    frame->width = width;
    frame->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0)
    {
        fprintf(stderr, "Could not allocate frame data.\n");
        qDebug()<<"[mux error!] Could not allocate frame data.";
        m_log.writeLog("[mux error!] Could not allocate frame data.");
        exit(1);
    }

    return frame;
}


void MuxThread::open_audio(AVFormatContext* oc, const AVCodec* codec,OutputStream* ost, AVDictionary* opt_arg)
{
    AVCodecContext* c;
    int nb_samples;
    int ret;
    AVDictionary* opt = NULL;

    c = ost->enc;

    /* open it */
    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0)
    {
        fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
        qDebug()<<"[mux error!] Could not open audio codec:"<<av_err2str(ret);
        logInfo<<"[mux error!] Could not open audio codec:"<<av_err2str(ret);
        strLogInfo=logInfo.str();
        m_log.writeLog(strLogInfo);
        logInfo.str("");
        strLogInfo.clear();
        exit(1);
    }

    /* init signal generator */
    ost->t = 0;
    ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    ost->frame = alloc_audio_frame(c->sample_fmt, &c->ch_layout,
        c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, &c->ch_layout,
        c->sample_rate, nb_samples);

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0)
    {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }

    /* create resampler context */
    ost->swr_ctx = swr_alloc();
    if (!ost->swr_ctx)
    {
        fprintf(stderr, "Could not allocate resampler context\n");
        qDebug()<<"[mux error!] Could not allocate resampler context.";
        m_log.writeLog("[mux error!] Could not allocate resampler context.");
        exit(1);
    }

    /* set options */
    av_opt_set_chlayout(ost->swr_ctx, "in_chlayout", &c->ch_layout, 0);
    av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_chlayout(ost->swr_ctx, "out_chlayout", &c->ch_layout, 0);
    av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
    av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

    /* initialize the resampling context */
    if ((ret = swr_init(ost->swr_ctx)) < 0)
    {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        qDebug()<<"[mux error!] Failed to initialize the resampling context.";
        m_log.writeLog("[mux error!] Failed to initialize the resampling context.");
        exit(1);
    }
}

AVFrame* MuxThread::alloc_audio_frame(enum AVSampleFormat sample_fmt,const AVChannelLayout* channel_layout,int sample_rate, int nb_samples)
{
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Error allocating an audio frame\n");
        qDebug()<<"[mux error!] Error allocating an audio frame.";
        m_log.writeLog("[mux error!] Error allocating an audio frame.");
        exit(1);
    }

    frame->format = sample_fmt;
    if(av_channel_layout_copy(&frame->ch_layout, channel_layout)<0)
    {
        qDebug()<<"[mux error!] Error av_channel_layout_copy.";
        m_log.writeLog("[mux error!] Error av_channel_layout_copy.");
        exit(1);
    }
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples)
    {
        if (av_frame_get_buffer(frame, 0) < 0)
        {
            fprintf(stderr, "Error allocating an audio buffer\n");
            qDebug()<<"[mux error!] Error allocating an audio buffer.";
            m_log.writeLog("[mux error!] Error allocating an audio buffer.");
            exit(1);
        }
    }

    return frame;
}

int MuxThread::write_video_frame(AVFormatContext* oc, OutputStream* ost)
{
    return write_frame(oc, ost->enc, ost->st, get_video_frame(ost), ost->tmp_pkt);
}

AVFrame* MuxThread::get_video_frame(OutputStream* ost)
{
    AVCodecContext* c = ost->enc;

    /* check if we want to generate more frames */
    /*if (av_compare_ts(ost->next_pts, c->time_base,
        STREAM_DURATION, (AVRational) { 1, 1 }) > 0)
        return NULL;*/

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(ost->frame) < 0)
    {
        fprintf(stderr,"Could not make frame writable.");
        qDebug()<<"[mux error!] Could not make frame writable.";
        m_log.writeLog("[mux error!] Could not make frame writable.");
        exit(1);
    }


    int res;

    if (c->pix_fmt != AV_PIX_FMT_YUV420P)
    {
        /* as we only generate a YUV420P picture, we must convert it
         * to the codec pixel format if needed */
        if (!ost->sws_ctx)
        {
            ost->sws_ctx = sws_getContext(c->width, c->height,
                AV_PIX_FMT_YUV420P,
                c->width, c->height,
                c->pix_fmt,
                SCALE_FLAGS, NULL, NULL, NULL);
            if (!ost->sws_ctx) {
                fprintf(stderr,"Could not initialize the conversion context\n");
                qDebug()<<"[mux error!] Could not initialize the conversion context.";
                m_log.writeLog("[mux error!] Could not initialize the conversion context.");
                exit(1);
            }
        }
        qDebug()<<"convert it to the codec pixel";
        res=fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);
        sws_scale(ost->sws_ctx, (const uint8_t* const*)ost->tmp_frame->data,
            ost->tmp_frame->linesize, 0, c->height, ost->frame->data,
            ost->frame->linesize);
    }
    else
    {
        res=fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
    }
    if(res==0)
    {
        ost->frame->pts = ost->next_pts++;
        return ost->frame;
    }
    else
    {
        return NULL;
    }


}

int MuxThread::fill_yuv_image(AVFrame* pict, int frame_index,int width, int height)
{

    VFrameBuf* pvFrameBuf=nullptr;
    while(1)
    {
        Sleep(20);//防止核心满载
        if (!videoQueue.isEmpty())
        {
            videoQueueMutex.lock();
            pvFrameBuf=videoQueue.dequeue();
            videoQueueMutex.unlock();
            break;
        }
        else if(videoQueue.isEmpty()&&muxStart==false)
        {
            return 1;
        }
    }

//    if(vusedSpace.available()==0&&muxStart==false)
//        return 1;

//    vusedSpace.acquire();
//    pvFrameBuf=videoQueue.dequeue();
//    vfreeSpace.release();

    /* Y */
    if(memcpy_s(pict->data[0],pict->linesize[0]*pict->height,pvFrameBuf->yChannel,sizeof(pvFrameBuf->yChannel))!=0)
    {
        qDebug()<<"[mux error!] Could not memcpy pvFrameBuf->yChannel to pict->data[0].";
        m_log.writeLog("[mux error!] Could not memcpy pvFrameBuf->yChannel to pict->data[0].");
        exit(1);
    }

    /* Cb and Cr */
    if(memcpy_s(pict->data[1],pict->linesize[1]*pict->height,pvFrameBuf->uChannel,sizeof(pvFrameBuf->uChannel))!=0)
    {
        qDebug()<<"[mux error!] Could not memcpy pvFrameBuf->uChannel to pict->data[1].";
        m_log.writeLog("[mux error!] Could not memcpy pvFrameBuf->uChannel to pict->data[1].");
        exit(1);
    }
    if(memcpy_s(pict->data[2],pict->linesize[2]*pict->height,pvFrameBuf->vChannel,sizeof(pvFrameBuf->vChannel))!=0)
    {
        qDebug()<<"[mux error!] Could not memcpy pvFrameBuf->vChannel to pict->data[2].";
        m_log.writeLog("[mux error!] Could not memcpy pvFrameBuf->vChannel to pict->data[2].");
        exit(1);
    }
    delete pvFrameBuf;
    return 0;
}

int MuxThread::write_frame(AVFormatContext* fmt_ctx, AVCodecContext* c,AVStream* st, AVFrame* frame, AVPacket* pkt)
{
    int ret;

    // send the frame to the encoder
    ret = avcodec_send_frame(c, frame);
    if (ret < 0)
    {
        fprintf(stderr, "Error sending a frame to the encoder: %s\n",av_err2str(ret));
        qDebug()<<"[mux error!] Error sending a frame to the encoder:"<<av_err2str(ret);
        logInfo<<"[mux error!] Error sending a frame to the encoder:"<<av_err2str(ret);
        strLogInfo=logInfo.str();
        m_log.writeLog(strLogInfo);
        logInfo.str("");
        strLogInfo.clear();
        exit(1);
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_packet(c, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0)
        {
            fprintf(stderr, "Error encoding a frame: %s\n", av_err2str(ret));
            qDebug()<<"[mux error!] Error encoding a frame:"<<av_err2str(ret);
            logInfo<<"[mux error!] Error encoding a frame:"<<av_err2str(ret);
            strLogInfo=logInfo.str();
            m_log.writeLog(strLogInfo);
            logInfo.str("");
            strLogInfo.clear();
            exit(1);
        }

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(pkt, c->time_base, st->time_base);
        pkt->stream_index = st->index;

        /* Write the compressed frame to the media file. */
        //log_packet(fmt_ctx, pkt);
        ret = av_interleaved_write_frame(fmt_ctx, pkt);
        /* pkt is now blank (av_interleaved_write_frame() takes ownership of
         * its contents and resets pkt), so that no unreferencing is necessary.
         * This would be different if one used av_write_frame(). */
        if (ret < 0)
        {
            fprintf(stderr, "Error while writing output packet: %s\n", av_err2str(ret));
            qDebug()<<"[mux error!] Error while writing output packet:"<<av_err2str(ret);
            logInfo<<"[mux error!] Error while writing output packet:"<<av_err2str(ret);
            strLogInfo=logInfo.str();
            m_log.writeLog(strLogInfo);
            logInfo.str("");
            strLogInfo.clear();
            exit(1);
        }
    }
    return ret == AVERROR_EOF ? 1 : 0;
}

int MuxThread::reFrameSize(unsigned char* re_nb_smaples_frameBuf,int outFrameBufSize)
{
    AFrameBuf* paFrameBuf=nullptr;
    int bufSize=0;

    while(bufSize<outFrameBufSize)
    {
        while(1)
        {
            if (!audioQueue.empty())
            {
               audioQueueMutex.lock();
               paFrameBuf=audioQueue.head();
               audioQueueMutex.unlock();
                break;
            }
            else if(audioQueue.empty()&&muxStart==false)
            {
                return 1;
            }
        }

//        if(ausedSpace.available()==0&&muxStart==false)
//        {
//            qDebug()<<"###########"<<bufSize;
//            return bufSize;
//        }
//        ausedSpace.acquire();
//        paFrameBuf=audioQueue.head();
//        afreeSpace.release();

        //qDebug()<<paFrameBuf->pcmSize<<outFrameBufSize;
        if(outFrameBufSize-bufSize>=paFrameBuf->pcmSize)
        {
            if(memcpy_s(re_nb_smaples_frameBuf+ bufSize,outFrameBufSize-bufSize,paFrameBuf->paFrame+ paFrameBuf->seek,paFrameBuf->pcmSize)!=0)
            {
                qDebug()<<"[mux error!] Could not audio memcpy 1.";
                m_log.writeLog("[mux error!] Could not audio memcpy 1.");
                exit(1);
            }
            delete[] paFrameBuf->paFrame; 
            paFrameBuf->paFrame = NULL;
            bufSize+=paFrameBuf->pcmSize;
            delete paFrameBuf;
            paFrameBuf = NULL;
            audioQueueMutex.lock();
            audioQueue.dequeue();
            audioQueueMutex.unlock();
            
        }
    
       
        

        else
        {
           if(memcpy_s(re_nb_smaples_frameBuf+bufSize,outFrameBufSize-bufSize,paFrameBuf->paFrame + paFrameBuf->seek,outFrameBufSize-bufSize)!=0)
            {
                qDebug()<<"[mux error!] Could not audio memcpy 2.";
                m_log.writeLog("[mux error!] Could not audio memcpy 2.");
                exit(1);
            }
            //unsigned char* newBuf=new unsigned char[paFrameBuf->pcmSize-outFrameBufSize+bufSize];
            qDebug() << paFrameBuf->pcmSize - outFrameBufSize + bufSize;
            /*if (newBuf == NULL)
            {
                qDebug()<<"[mux error!] Could not alloc newBuf in function reFrameSize().";
                m_log.writeLog("[mux error!] Could not alloc newBuf in function reFrameSize().");
                exit(1);
            }
            if(memcpy_s(newBuf,paFrameBuf->pcmSize-outFrameBufSize+bufSize,paFrameBuf->paFrame+outFrameBufSize-bufSize,paFrameBuf->pcmSize-outFrameBufSize+bufSize)!=0)
            {
                qDebug()<<"[mux error!] Could not audio memcpy 3.";
                m_log.writeLog("[mux error!] Could not audio memcpy 3.");
                exit(1);
            }
            delete[] paFrameBuf->paFrame;
            paFrameBuf->paFrame=newBuf;
            newBuf = NULL;*/
            paFrameBuf->pcmSize=paFrameBuf->pcmSize-outFrameBufSize+bufSize;
            paFrameBuf->seek += outFrameBufSize - bufSize;
            paFrameBuf = NULL;
            bufSize+=outFrameBufSize-bufSize;
        }

    }
    return 0;
}

int MuxThread::write_audio_frame(AVFormatContext* oc, OutputStream* ost)
{
    AVCodecContext* c;
    AVFrame* frame;
    int ret;
    int dst_nb_samples;
    c = ost->enc;

    int frameBufSize=av_samples_get_buffer_size(NULL, c->ch_layout.nb_channels,c->frame_size,AV_SAMPLE_FMT_S16, 1);
    //qDebug()<<"recive audio size:"<<frameBufSize;

    frame = get_audio_frame(ost,frameBufSize);

    if (frame)
    {
        /* convert samples from native format to destination codec format, using the resampler */
        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                                        c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dst_nb_samples == frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
             * internally;
             * make sure we do not overwrite it here
             */
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
        {
            qDebug()<<"[mux error!] Could not av_frame_make_writable in function write_audio_frame.";
            m_log.writeLog("[mux error!] Could not av_frame_make_writable in function write_audio_frame.");
            exit(1);
        }

        /* convert to destination format */
        ret = swr_convert(ost->swr_ctx,
                          ost->frame->data, dst_nb_samples,
                          (const uint8_t**)frame->data, frame->nb_samples);
        if (ret < 0)
        {
            fprintf(stderr, "Error while converting\n");
            qDebug()<<"[mux error!] Error while converting";
            m_log.writeLog("[mux error!] Error while converting");
            exit(1);
        }
        frame = ost->frame;

        frame->pts = av_rescale_q(ost->samples_count, { 1, c->sample_rate }, c->time_base);
        ost->samples_count += dst_nb_samples;
    }


    return write_frame(oc, c, ost->st, frame, ost->tmp_pkt);
}

AVFrame* MuxThread::get_audio_frame(OutputStream* ost,int outFrameBufSize)
{
    AVFrame* frame = ost->tmp_frame;
    unsigned char* frameBuf=new unsigned char[outFrameBufSize];
    if(frameBuf==NULL)
    {
        qDebug()<<"[mux error!] Error alloc frameBuf in function get_audio_frame().";
        m_log.writeLog("[mux error!] Error alloc frameBuf in function get_audio_frame().");
        exit(1);
    }
    int ret=reFrameSize(frameBuf,outFrameBufSize);
    if(ret==1)
    {
        delete[] frameBuf;
        return NULL;
    }
    if(memcpy_s(frame->data[0],frame->linesize[0],frameBuf,outFrameBufSize)!=0)
    {
        qDebug()<<"[mux error!] Error memcpy frameBuf frame->data[0] .";
        m_log.writeLog("[mux error!] Error memcpy frameBuf frame->data[0] .");
        exit(1);
    }
    delete[] frameBuf;

    frame->pts = ost->next_pts;
    ost->next_pts += frame->nb_samples;

    return frame;
}

void MuxThread::close_stream(AVFormatContext* oc, OutputStream* ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    av_packet_free(&ost->tmp_pkt);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}
