#ifndef VIDEO_SRC_H
#define VIDEO_SRC_H

extern "C" {
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/timestamp.h"
#include "libavformat/avformat.h"
#include "libavutil/frame.h"
#include "libavutil/mem.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avio.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavdevice/avdevice.h"
#include "libavutil/dict.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
#include <unistd.h>
}

class VideoSource
{
public:
    explicit VideoSource();
    ~VideoSource();

    bool open();
    void close();

    AVFrame* readFrame();

    int getDecodeHeight();
    int getDecodeWidth();
    AVPixelFormat getDeocdePixFmt();

private:
    bool openMacos();
    bool openWin();
    bool openLinux();

    bool closeMacos();
    bool closeWin();
    bool closeLinux();

    const char* getDeviceFamily();

    AVCodecContext*     m_srcDecodeCtx;
    AVFormatContext*    m_srcFmtDecCtx;
    AVPacket pkt;
    AVFrame* oldFrame;

    static constexpr const char* const TAG = "VideoSource";
};

#endif // VIDEO_SRC_H
