#include "video_source.h"
#include <iostream>

#define USE_SCREEN_CAPTURE 0

VideoSource::VideoSource() {
    m_srcDecodeCtx = NULL;
    m_srcFmtDecCtx = NULL;
    oldFrame = av_frame_alloc();
    av_init_packet(&pkt);
}

VideoSource::~VideoSource() {
    avcodec_free_context(&m_srcDecodeCtx);
    if (m_srcFmtDecCtx) {
        avformat_flush(m_srcFmtDecCtx);
        avformat_close_input(&m_srcFmtDecCtx);
        m_srcFmtDecCtx = NULL;
    }
    av_frame_free(&oldFrame);
}

bool VideoSource::open() {
    bool res = false;
#ifdef _WIN32
    res = openWin();
#elif __APPLE__
    res = openMacos();
#elif __linux__
  res = openLinux();
#endif
  return res;
}

void VideoSource::close() {
#ifdef _WIN32
    closeWin();
#elif __APPLE__
    closeMacos();
#elif __linux__
    closeLinux();
#endif
}

AVFrame* VideoSource::readFrame() {
    int ret = 0;
    av_init_packet(&pkt);

    if (av_read_frame(m_srcFmtDecCtx, &pkt) < 0) {
        av_packet_unref(&pkt);
        return NULL;
    }
    ret = avcodec_send_packet(m_srcDecodeCtx, &pkt);
    if (ret != 0) {
        std::cout << "avcodec_send_packet failed, ret:" << ret;
        av_packet_unref(&pkt);
        return NULL;
    }
    ret = avcodec_receive_frame(m_srcDecodeCtx, oldFrame);
    if (ret != 0) {
        std::cout << "avcodec_receive_frame failed, ret:" << ret;
        av_packet_unref(&pkt);
        return NULL;
    }
    av_packet_unref(&pkt);
    return oldFrame;
}

int VideoSource::getDecodeHeight() {
    return m_srcDecodeCtx ? m_srcDecodeCtx->height : 0;
}

int VideoSource::getDecodeWidth() {
    return m_srcDecodeCtx ? m_srcDecodeCtx->width : 0;
}

AVPixelFormat VideoSource::getDeocdePixFmt() {
    return m_srcDecodeCtx ? m_srcDecodeCtx->pix_fmt: AV_PIX_FMT_NONE;
}

bool VideoSource::openMacos() {
    int err = 0;
    const AVCodec* decoder = NULL;
    AVDictionary* options = NULL;

    auto devFamily = getDeviceFamily();
    const AVInputFormat *iformat = av_find_input_format(devFamily);
    if(iformat == NULL) {
        std::cout << "getDeviceFamily == NULL";
        return false;
    }
    av_dict_set(&options, "video_size","1280x720", 0);
    av_dict_set(&options, "pixel_format","uyvy422",0);
    av_dict_set(&options, "framerate","30",0);

    m_srcFmtDecCtx = avformat_alloc_context();

#if USE_SCREEN_CAPTURE == 1
    av_dict_set(&options, "capture_cursor","1",0);
    av_dict_set(&options, "capture_mouse_clicks","1",0);
    err = avformat_open_input((AVFormatContext**)&m_srcFmtDecCtx,"1", (AVInputFormat*)iformat, &options);
#else
    err = avformat_open_input((AVFormatContext**)&m_srcFmtDecCtx,"0", (AVInputFormat*)iformat, &options);
#endif
    if(err < 0) {
        std::cout << "avformat_open_input returned <0";
        return false;
    }
    if (avformat_find_stream_info(m_srcFmtDecCtx, NULL) < 0) {
        std::cout << "couldn't find stream information";
        return false;
    }
    if (m_srcFmtDecCtx->streams[0]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        for(unsigned int i=0; i < m_srcFmtDecCtx->nb_streams; ++i) {
            auto stream = m_srcFmtDecCtx->streams[i];
            if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                decoder = avcodec_find_decoder(stream->codecpar->codec_id);
                if (decoder == NULL) {
                    std::cout << "couldn't find stream information";
                    return false;
                }
                m_srcDecodeCtx = avcodec_alloc_context3(decoder);
                if ((err = avcodec_parameters_to_context(m_srcDecodeCtx, stream->codecpar)) < 0) {
                    std::cout << "Video avcodec_parameters_to_context failed,error code";
                    return false;
                }
                break;
            }
        }
    }
    if (avcodec_open2(m_srcDecodeCtx, decoder, &options) < 0) {
        std::cout << "avcodec_open2 failed";
        return false;
    }
    av_dict_free(&options);
    return true;
}

bool VideoSource::openWin() {
    // TODO:
    return false;
}

bool VideoSource::openLinux() {
    // TODO:
    return false;
}

bool VideoSource::closeMacos() {
    if (m_srcFmtDecCtx) {
        avformat_flush(m_srcFmtDecCtx);
        avformat_close_input(&m_srcFmtDecCtx);
        m_srcFmtDecCtx = NULL;
    }
    return true;
}

bool VideoSource::closeWin() {
    // TODO:
    return true;
}

bool VideoSource::closeLinux() {
    // TODO:
    return true;
}

const char* VideoSource::getDeviceFamily() {
#ifdef _WIN32
  const char *device_family = "dshow";
#elif __APPLE__
  const char *device_family = "avfoundation";
#elif __linux__
  const char *device_family = "v4l2";
#endif
  return device_family;
}
