cmake_minimum_required(VERSION 3.0)

set(PROJECT web_ffmpeg_cpp)

project(${PROJECT} VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_C_FLAGS_DEBUG "-O0")
add_compile_options(-g -fPIC -D__DEBUG__)

find_package(Qt5 COMPONENTS
    Core
    Network
    Svg
    Gui
    Quick
    LinguistTools
    Multimedia
    Charts
    Concurrent
    QuickControls2
    REQUIRED
)
# Qt libraries
set(LIBRARIES ${LIBRARIES}
    Qt5::Core
    Qt5::Network
    Qt5::Svg
    Qt5::Gui
    Qt5::Quick
    Qt5::Multimedia
    Qt5::Charts
    Qt5::Concurrent
    Qt5::QuickControls2
)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(APPLE)
    add_compile_options(-D__STDC_CONSTANT_MACROS)
endif()

set(SOURCES    
    ./main.cpp
    ./video.cpp
    ./video_source.cpp
)

if(APPLE)
    SET(LIBRARIES ${LIBRARIES}
        "-lbz2 \
        -llzma \
        -framework AppKit \
        -framework CoreFoundation \
        -framework AVFoundation \
        -framework CoreMedia \
        -framework CoreVideo \
        -framework CoreImage \
        -framework CoreAudio \
        -framework VideoToolbox \
        -framework AudioToolbox \
        -framework OpenGL"
    )
endif()

if(APPLE)
    find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
    find_library(AVCODEC_LIBRARY avcodec)
    find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
    find_library(AVFORMAT_LIBRARY avformat)
    find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
    find_library(AVUTIL_LIBRARY avutil)
    find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
    find_library(AVDEVICE_LIBRARY avdevice)
    find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
    find_library(SWSCALE_LIBRARY swscale)
endif()

add_executable(
    ${PROJECT}
    ${SOURCES}
    ${HEADERS}
)

target_compile_options(${PROJECT} PRIVATE -Wformat)

if(APPLE)
    target_include_directories(${PROJECT} PUBLIC
        ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVDEVICE_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR}
    )
    target_link_libraries(
        ${PROJECT}
        ${LIBRARIES}
        ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${AVDEVICE_LIBRARY} ${SWSCALE_LIBRARY}
    )
endif()
