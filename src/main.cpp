
#include <thread>
#include <vector>
#include <mutex>
#include <iostream>
#include <functional>

#include "video.h"
#include <QImage>

enum class Command { 
    StartCamera = 1, 
    StopCamera = 2,
    Enable_resolution_640x240 = 3,
    Enable_resolution_800x600 = 4,
    Enable_resolution_1280x1024 = 5
};

int main()
{
    Video* video = NULL;
    bool isDone = false;

    auto statusCallback = ([&](VideStats stats) {
        std::cout << "statusCallback: "
            << "active:" << stats.is_active
            << "pkt:" << stats.packet_cnt
            << "err:" << stats.err_cnt << std::endl;
    });
    auto frameCallback = ([&](AVFrame* frame) {
        if(frame != NULL) {
             QImage image(frame->width, frame->height, QImage::Format_RGB32);
             int x, y;
             int *src = (int*)frame->data[0];
             for (y = 0; y < frame->height; y++) {
                 for (x = 0; x < frame->width; x++) {
                     image.setPixel(x, y, src[x] & 0x00ffffff);
                 }
                 src += frame->width;
             }
             image.save("/Users/khomin/Desktop/out.png");

            // std::cout << "frameCallback: frame\n"
            // << "width," << frame->width
            // << "height" << frame->height
            // << std::endl;
        } else {
            std::cout << "frameCallback: frame == null" << std::endl;
        }
    });

    auto th = std::thread ([&] {
        char command_buf[64];

        video = new Video();
        video->setStatusCallBack(statusCallback);
        video->setFrameCallBack(frameCallback);

        while(!isDone) {
            std::cout << "Enter value:\n";
            
            std::cin.getline (command_buf, sizeof(command_buf));

            auto command = std::atoi(command_buf);

            switch((Command)command) {
                case Command::StartCamera: {
                    std::cout << "Command: startCamera\n";
                    if(!video->isStarted()) {
                        video->startVideoCamera();
                    }
                    std::cout << "Command: startCamera -done\n";
                }
                break;

                case Command::StopCamera: {
                    std::cout << "Command: stopCamera\n";
                    if(video->isStarted()) {
                        video->stopVideo();
                    }
                    std::cout << "Command: stopCamera -done\n";
                }
                break;
                case Command::Enable_resolution_640x240: {
                    std::cout << "Command: enable_resolution_640x240\n";
                    if(video == NULL || !video->isStarted()) {
                        std::cout << "Command: enable_resolution_640x240 - camera is not started!\n";
                    } else {
                        video->setResolution(640, 240);
                        std::cout << "Command: enable_resolution_640x240 -done\n";
                    }
                }
                break;
                case Command::Enable_resolution_800x600: {
                    std::cout << "Command: enable_resolution_800x600\n";
                    if(video == NULL || !video->isStarted()) {
                        std::cout << "Command: enable_resolution_800x600 - camera is not started!\n";
                    } else {
                        video->setResolution(800, 600);
                        std::cout << "Command: enable_resolution_800x600 -done\n";
                    }
                }
                break;
                case Command::Enable_resolution_1280x1024: {
                    std::cout << "Command: enable_resolution_1280x1024\n";
                    if(video == NULL || !video->isStarted()) {
                        std::cout << "Command: enable_resolution_1280x1024 -camera is not started!\n";
                    } else {
                        video->setResolution(1280, 1024);
                        std::cout << "Command: enable_resolution_1280x1024 -done\n";
                    }
                }
                break;
                default:
                    std::cout << "Command: unknown\n";
                break;
            }
        }
    });

    std::cout << "main, foo and bar now execute concurrently...\n";
    th.join();

    std::cout << "foo and bar completed.\n";

    return 1;
}
