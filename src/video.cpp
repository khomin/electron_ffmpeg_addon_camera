#include "video.h"
#include <chrono>

Video::Video() {
    m_state = VideoState::Stopped;

    avdevice_register_all();

    m_video_cap_thread = NULL;
    m_video_dispather_thread = NULL;
    m_video_cap_tr_run = false;
    m_video_dispather_tr_run = false;
    m_dimention_height = DEFAULT_HEIGHT;
    m_dimention_width = DEFAULT_WIDTH;
    m_outFrameBusSize = 0;
    m_errors = 0;
    m_frames_cnt = 0;
    // start dispatcher
    m_video_dispather_thread = procDispatcherThread();
    m_video_dispather_thread->detach();
}

Video::~Video() {
    m_state = VideoState::Destruction;
    waitToStop();
}

void Video::startVideoCamera() {
    Command command;
    command.type = CommandType::Stop;
    m_command_queue.push(command);
    command.type = CommandType::StartCamera;
    m_command_queue.push(command);
}

void Video::stopVideo() {
    Command command;
    command.type = CommandType::Stop;
    m_command_queue.push(command);
}

void Video::setResolution(int width, int height) {
    Command command;
    command.type = CommandType::Stop;
    m_command_queue.push(command);
    //
    m_dimention_width = width;
    m_dimention_height = height;
    //
    command.type = CommandType::StartCamera;
    m_command_queue.push(command);
}

void Video::setFrameCallBack(std::function<void(AVFrame*,uint32_t)> cb) {
    m_frame_callback = cb;
}

void Video::setStatusCallBack(std::function<void(VideStats)> cb) {
    m_status_callback = cb;
}

bool Video::isStarted() {
    return m_state != VideoState::Stopped && m_state != VideoState::Destruction;
}

uint32_t Video::getPacketCount() {
    return m_frames_cnt;
}

uint32_t Video::getErrorCount() {
    return m_errors;
}

std::thread* Video::procDispatcherThread() {
    return new std::thread([&] {
        m_video_dispather_tr_run = true;
        AVCodecContext* rxDecodeCtx = NULL;
        SwsContext* scaleToScreenCtx = NULL;
        AVFrame* rxRawFrame = NULL;
        AVFrame* outToScreenFrame = NULL;
        uint8_t* outToScreenFrameBuf = NULL;

        auto clearBeforeExit([&] {
            if(rxRawFrame != NULL) {
                av_frame_free(&rxRawFrame);
            }
            if(outToScreenFrame != NULL) {
                av_frame_free(&outToScreenFrame);
            }
            if(outToScreenFrameBuf != NULL) {
                av_free(outToScreenFrameBuf);
            }
            if(rxDecodeCtx != NULL) {
                avcodec_free_context(&rxDecodeCtx);
            }
            sws_freeContext(scaleToScreenCtx);
            m_video_dispather_tr_run = false;
        });

        while(m_state != VideoState::Destruction) {
            //
            // gather statistic, handle commands
            //
            if(!m_command_queue.empty()) {
                Command command = m_command_queue.front();
                m_command_queue.pop();
                if(command.type == CommandType::StartCamera) {
                    m_state = VideoState::Active;
                    // reset stats
                    m_errors = 0;
                    m_frames_cnt = 0;
                    // start video capture thread
                    m_video_cap_thread = procVideoCaptureThread();
                    m_video_cap_thread->detach();
                } else if(command.type == CommandType::Stop) {
                    m_state = VideoState::Stopped;
                    // notify and wait the treads to finish
                    m_cvNotEmpty.notify_all();
                    m_cvDone.notify_all();
                    while(m_video_cap_tr_run && m_state != VideoState::Destruction) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_KILL_THREAD));
                    }
                }
            }
            updateStats();
            std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_DISPATCHER_THREAD));
        }
        clearBeforeExit();
    });
}

std::thread* Video::procVideoCaptureThread() {
    return new std::thread([&] {
        int ret = -1;
        AVFrame* newFrame = NULL;
        AVFrame* outToScreenMirFrame = NULL;
        SwsContext* swsToScreenMirrorCtx = NULL;
        uint8_t* newFrameBuf = NULL;
        uint8_t* outToScreenMirFrameBuf = NULL;
        VideoSource* video_src = NULL;
        m_video_cap_tr_run = true;

        auto clearBeforeExit([&] {
            av_free(newFrameBuf);
            av_frame_free(&newFrame);
            av_frame_free(&outToScreenMirFrame);
            av_free(outToScreenMirFrameBuf);
            sws_freeContext(swsToScreenMirrorCtx);
            if(video_src != NULL) {
                video_src->close();
                delete video_src;
            }
            m_video_cap_tr_run = false;
        });
        m_outFrameBusSize = av_image_get_buffer_size(AV_PIX_FMT_RGB32,
                                                        m_dimention_width,
                                                        m_dimention_height, 1);
        outToScreenMirFrameBuf = (uint8_t *)av_malloc(m_outFrameBusSize);
        outToScreenMirFrame = av_frame_alloc();

        ret = av_image_alloc(outToScreenMirFrame->data,
                             outToScreenMirFrame->linesize,
                             m_dimention_width,
                             m_dimention_height,
                             AV_PIX_FMT_RGB32, 1);
        if (ret < 0) {
             memset(outToScreenMirFrame, 0, m_outFrameBusSize);
         }

        av_image_fill_arrays(outToScreenMirFrame->data, outToScreenMirFrame->linesize,
                            outToScreenMirFrameBuf, AV_PIX_FMT_RGB32,
                            m_dimention_width, m_dimention_height, 1);

        video_src = new VideoSource();
        if(!video_src->open()) {
            m_state = VideoState::Stopped;
            clearBeforeExit();
            return;
        }
        swsToScreenMirrorCtx = sws_getContext(video_src->getDecodeWidth(),
                                              video_src->getDecodeHeight(),
                                              video_src->getDeocdePixFmt(),
                                              m_dimention_width,
                                              m_dimention_height,
                                              AV_PIX_FMT_RGB32,
                                              SWS_BICUBIC, NULL, NULL, NULL);
        
        auto last_frame_time = std::chrono::steady_clock::now();
        auto next_frame_time = last_frame_time;

        while(m_state != VideoState::Stopped && m_state != VideoState::Destruction) {
            auto oldFrame = video_src->readFrame();
            if(oldFrame == NULL)  {
                continue;
            }
            // out this frame on the screen
            sws_scale(swsToScreenMirrorCtx,
                      oldFrame->data,
                      oldFrame->linesize,
                      0,
                      video_src->getDecodeHeight(),
                      outToScreenMirFrame->data,
                      outToScreenMirFrame->linesize);
            outToScreenMirFrame->width = m_dimention_width;
            outToScreenMirFrame->height = m_dimention_height;
            outToScreenMirFrame->format = AV_PIX_FMT_RGB32;
            m_frames_cnt++;
            
            if(std::chrono::steady_clock::now() >= next_frame_time) {
                if(m_frame_callback != NULL) {
                    m_frame_callback(outToScreenMirFrame, m_outFrameBusSize);
                }
                last_frame_time = std::chrono::steady_clock::now();
                next_frame_time = last_frame_time + std::chrono::milliseconds(30);
            }
        }
        clearBeforeExit();
    });
}

void Video::updateStats() {
    if(m_status_callback != NULL) {
        VideStats stats;
        stats.err_cnt = getErrorCount();
        stats.packet_cnt = getPacketCount();
        stats.is_active = m_state == VideoState::Active;
        m_status_callback(stats);
    }
}

void Video::waitToStop() {
    // notify and wait the treads to finish
    m_cvNotEmpty.notify_all();
    m_cvDone.notify_all();
    // wait until the threads finish
    while(m_video_cap_tr_run) {
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_KILL_THREAD));
    }
    if(m_state == VideoState::Destruction) {
        while(m_video_dispather_tr_run) {
            std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_KILL_THREAD));
        }
    }
}