// #pragma once
#include <napi.h>
#include "video.h"

// using namespace Napi;

// class WebCamApi : public AsyncWorker {
    
//     public:
//         WebCamApi(Function& callback, int runTime);
//         virtual ~WebCamApi() {};

//         void Execute();
//         void OnOK();

//     private:
//         int runTime;
// };

// class NativeAddon : public Napi::ObjectWrap<NativeAddon> {
//     public:
//         static Napi::Object Init(Napi::Env env, Napi::Object exports);
//         NativeAddon(const Napi::CallbackInfo& info);
        
//     private:
//         static Napi::FunctionReference constructor;
//         Napi::FunctionReference jsFnRef;
//         Napi::Function jsFn;

//         void TryCallByStoredReference(const Napi::CallbackInfo& info);
//         void TryCallByStoredFunction(const Napi::CallbackInfo& info);
// };