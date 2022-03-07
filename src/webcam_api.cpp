#include "webcam_api.h"
#include <iostream>
#include <queue>
#include <mutex>
#include <assert.h>
#include <stdlib.h>
#define NAPI_EXPERIMENTAL
#include <node_api.h>

Video* m_video = NULL;

enum class DataItemType { DataStats, DataFrame };

class DataItem {
public:
    DataItemType type;
};

class DataItemStats : public DataItem {
public:
    VideStats* stats;
};

class DataItemFrame : public DataItem {
public:
    uint8_t* frame;
    uint32_t frame_buf_size;
    int width;
    int height;
};

struct ThreadCtx {
    ThreadCtx(Napi::Env env) {};
    std::thread nativeThread;
    Napi::ThreadSafeFunction tsfn;
    bool toCancel = false;

    std::queue<DataItem*> m_data_queue;
    std::mutex m_data_lock;
    std::condition_variable m_data_cv;
};

ThreadCtx* threadCtx = NULL;

Napi::Value setStatusCb(const Napi::CallbackInfo& info) {
    auto env = info.Env();
    threadCtx = new ThreadCtx(env);
    threadCtx->tsfn = Napi::ThreadSafeFunction::New(
                            env, 
                            info[0].As<Napi::Function>(),
                            "CallbackMethod", 
                            0, 1 , 
                            threadCtx,
        [&]( Napi::Env, void *finalizeData, ThreadCtx *context ) {
            std::cout << "Thread cleanup-start";
            threadCtx->nativeThread.join();
            std::cout << "Thread cleanup-end";
        },
        (void*)nullptr
    );

    threadCtx->nativeThread = std::thread([&]{
        auto callbackStats = [](Napi::Env env, Napi::Function cb, char* buffer) {
            auto data = (DataItemStats*)buffer;
            if(data == NULL) return;

            Napi::Object obj = Napi::Object::New(env);
            obj.Set("type", std::string("stats"));
            obj.Set("is_active", std::to_string(data->stats->is_active));
            obj.Set("packet_cnt", std::to_string(data->stats->packet_cnt));
            obj.Set("err_cnt", std::to_string(data->stats->err_cnt));
            cb.Call({obj});
            delete data->stats;
            delete data;
        };
        auto callbackFrame = [](Napi::Env env, Napi::Function cb, char* buffer) {
            auto data = (DataItemFrame*)buffer;
            if(data == NULL) return;

            napi_value arrayBuffer;
            void* yourPointer = malloc(data->frame_buf_size);
            // creates your ArrayBuffer
            napi_create_arraybuffer(env, data->frame_buf_size, &yourPointer, &arrayBuffer);

            memcpy((uint8_t*)yourPointer, data->frame, data->frame_buf_size);

            Napi::Object obj = Napi::Object::New(env);
            obj.Set("type", std::string("frame"));
            obj.Set("data", arrayBuffer);
            obj.Set("width", data->width);
            obj.Set("height", data->height);
            cb.Call({obj});
            delete data->frame;
            delete data;
        };
        while(!threadCtx->toCancel) {
            DataItem* data_item = NULL;
            std::unique_lock<std::mutex> lk(threadCtx->m_data_lock);
            threadCtx->m_data_cv.wait(lk, [&] {
                return !threadCtx->m_data_queue.empty();
            });

            while(!threadCtx->m_data_queue.empty()) {
                data_item = threadCtx->m_data_queue.front();
                threadCtx->m_data_queue.pop();
                if(data_item == NULL) continue;

                if(data_item->type == DataItemType::DataStats) {
                    napi_status status = threadCtx->tsfn.BlockingCall((char*)data_item, callbackStats);
                    if (status != napi_ok) {
                        // Handle error
                        break;
                    }
                } else if(data_item->type == DataItemType::DataFrame) {
                    napi_status status = threadCtx->tsfn.BlockingCall((char*)data_item, callbackFrame);
                    if (status != napi_ok) {
                        // Handle error
                        break;
                    }
                }
            }
        }
        threadCtx->tsfn.Release();
    });

    return Napi::String::New(info.Env(), std::string("SimpleAsyncWorker for seconds queued.").c_str());
};

Napi::Boolean StartVideo(const Napi::CallbackInfo& info) {
    std::cout << "Command: startCamera\n";
    if(!m_video->isStarted()) {
        m_video->startVideoCamera();
    }
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, true);
}

Napi::Boolean StopVideo(const Napi::CallbackInfo& info) {
    std::cout << "Command: stopCamera\n";
    if(m_video->isStarted()) {
        m_video->stopVideo();
    }
    Napi::Env env = info.Env();
    return Napi::Boolean::New(env, true);
}

Napi::Value SetDimention(const Napi::CallbackInfo& info) {
    if(m_video == NULL || !m_video->isStarted()) {
        std::cout << "Command: setDimention -camera is not started!\n";
    } else if(info.Length() == 2) {
        int width = info[0].As<Napi::Value>().ToNumber();
        int height = info[1].As<Napi::Value>().ToNumber();;
        std::cout << "Command: setDimention: " << ",width=" << width << ",height=" << height << std::endl;
        m_video->setResolution(width, height);
    } else {
        std::cout << "Command: setDimention missed arguments\n";
    }
    return Napi::Number::New(info.Env(), true);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    m_video = new Video();
    m_video->setStatusCallBack(([&](VideStats stats) {
        if(threadCtx == NULL) return;
        std::lock_guard<std::mutex>lk(threadCtx->m_data_lock);
        auto data = new DataItemStats();
        data->type = DataItemType::DataStats;
        data->stats = new VideStats();
        data->stats->is_active = stats.is_active;
        data->stats->packet_cnt = stats.packet_cnt;
        data->stats->err_cnt = stats.err_cnt;
        threadCtx->m_data_queue.push(data);
        threadCtx->m_data_cv.notify_one();
    }));
    m_video->setFrameCallBack(([&](AVFrame* frame, uint32_t bufSize) {
        if(frame != NULL) {
            std::lock_guard<std::mutex>lk(threadCtx->m_data_lock);
            auto data = new DataItemFrame();
            data->type = DataItemType::DataFrame;
            data->frame = new uint8_t[bufSize];
            data->frame_buf_size = bufSize;
            data->width = frame->width;
            data->height = frame->height;
            memcpy(data->frame, (uint8_t*)frame->data[0], bufSize);
            threadCtx->m_data_queue.push(data);
            threadCtx->m_data_cv.notify_one();
        } else {
            std::cout << "frameCallback: frame == null" << std::endl;
        }
    }));

    exports["setStatusCb"] = Napi::Function::New(env, setStatusCb, std::string("setStatusCb"));
    exports.Set(Napi::String::New(env, "setCameraEnabled"), Napi::Function::New(env, StartVideo));
    exports.Set(Napi::String::New(env, "setCameraDisable"), Napi::Function::New(env, StopVideo));
    exports.Set(Napi::String::New(env, "setDimention"), Napi::Function::New(env, SetDimention));
    return exports;
}

NODE_API_MODULE(addon, Init)