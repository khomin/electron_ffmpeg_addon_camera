#ifndef VIDEO_H
#define VIDEO_H

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

#include <fstream>
#include <iostream>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

#include "video_source.h"
#include "video_state.h"
#include "video_stats.h"

class Video
{
public:
    Video();
    ~Video();

    void startVideoCamera();
    void stopVideo();
    void setResolution(int width, int height);

    void setFrameCallBack(std::function<void(AVFrame*,uint32_t)> cb);
    void setStatusCallBack(std::function<void(VideStats)> cb);

    std::function<void(AVFrame*,uint32_t)> m_frame_callback;
    std::function<void(VideStats)> m_status_callback;

    bool isStarted();

    uint32_t getPacketCount();
    uint32_t getErrorCount();
    
private:
    void waitToStop();

    void setStatsError(std::string error);
    void clearStatsError();
    void incStatsEncodePacket();
    void incStatsEncodeErrPacket();
    void setStatsBitrate(uint64_t bitrate);

    void incStatsDecodePacket();
    void incStatsDecodeErrPacket();

    void updateStats();

    std::thread* procVideoCaptureThread();
    std::thread* procDispatcherThread();

    std::thread* m_video_cap_thread;
    std::thread* m_video_dispather_thread;

    std::atomic_bool m_video_cap_tr_run;
    std::atomic_bool m_video_dispather_tr_run;

    enum class CommandType { StartCamera, Stop };

    typedef struct Command {
        CommandType type;
    }Command;

    std::queue<Command> m_command_queue;

    int               m_dimention_height;
    int               m_dimention_width;
    int m_outFrameBusSize;

    std::condition_variable m_cvNotEmpty;
    std::condition_variable m_cvDone;
    std::mutex				m_mtx;

    std::atomic<VideoState> m_state;

    uint32_t m_frames_cnt;
    uint32_t m_errors;

    static constexpr const int DELAY_KILL_THREAD            = 50;
    static constexpr const int DELAY_DISPATCHER_THREAD      = 500;
    static constexpr const int DEFAULT_HEIGHT               = 1280;
    static constexpr const int DEFAULT_WIDTH                = 1024;
    static constexpr const char* const TAG  = "Video";
};

#endif // VIDEO_H
