#ifndef MUXTHREAD_H
#define MUXTHREAD_H
extern"C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libavutil/avassert.h"
#include "libswresample/swresample.h"
}
#include <QThread>
#include "log.h"
#include <sstream>


#define OUT_VIDEO_FILENAME "out.mkv"
#define STREAM_FRAME_RATE 5 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */
#define WIDTH 1920
#define HEIGHT 1080
#define STREAM_DURATION 5.0
#define SCALE_FLAGS SWS_BICUBIC
#define AUDIO_SAMPLE_RATE 48000



typedef struct OutputStream {
    AVStream* st;
    AVCodecContext* enc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame* frame;
    AVFrame* tmp_frame;

    AVPacket* tmp_pkt;

    float t, tincr, tincr2;

    struct SwsContext* sws_ctx;
    struct SwrContext* swr_ctx;
} OutputStream;

class MuxThread : public QThread
{
    Q_OBJECT
public:
    explicit MuxThread(int isRecord,QString pushAddr,int fps,int vedioBitRate,QString outVideoFileName,QObject *parent = nullptr);
    bool muxStart=true;
protected:
    void run();
private:
    static AVFrame *picture;
    void add_stream(OutputStream* ost, AVFormatContext* oc,const AVCodec** codec,enum AVCodecID codec_id);
    void add_audio_stream(OutputStream* ost, AVFormatContext* oc,const AVCodec** codec,enum AVCodecID codec_id);
    void open_video(AVFormatContext* oc, const AVCodec* codec,OutputStream* ost, AVDictionary* opt_arg);
    AVFrame* alloc_frame(enum AVPixelFormat pix_fmt, int width, int height);
    void open_audio(AVFormatContext* oc, const AVCodec* codec,OutputStream* ost, AVDictionary* opt_arg);
    AVFrame* alloc_audio_frame(enum AVSampleFormat sample_fmt,const AVChannelLayout* channel_layout,int sample_rate, int nb_samples);
    int write_video_frame(AVFormatContext* oc, OutputStream* ost);
    AVFrame* get_video_frame(OutputStream* ost);
    int fill_yuv_image(AVFrame* pict, int frame_index,int width, int height);
    int write_frame(AVFormatContext* fmt_ctx, AVCodecContext* c,AVStream* st, AVFrame* frame, AVPacket* pkt);
    int write_audio_frame(AVFormatContext* oc, OutputStream* ost);
    AVFrame* get_audio_frame(OutputStream* ost,int outFrameBufSize);
    void close_stream(AVFormatContext* oc, OutputStream* ost);
    int reFrameSize(unsigned char* re_nb_smaples_frameBuf,int outFrameBufSize);
private:
    QString m_outVideoFileName;
    int m_fps=0;
    int m_vedioBitRate=0;
    Log m_log;
    std::ostringstream logInfo;
    std::string strLogInfo;
    int m_isRecord;
    QString m_pushAddr;
signals:
    void mfinish(int isRecord);
};

#endif // MUXTHREAD_H
