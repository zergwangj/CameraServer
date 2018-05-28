cmake_minimum_required(VERSION 3.8)
project(CameraServer)

include_directories(/usr/local/include/liveMedia/)
include_directories(/usr/local/include/groupsock/)
include_directories(/usr/local/include/UsageEnvironment/)
include_directories(/usr/local/include/BasicUsageEnvironment/)
include_directories(/usr/local/Cellar/ffmpeg@2.8/2.8.11/include/)
include_directories(/usr/local/Cellar/log4cpp/1.1.3/include/)
include_directories(./include/)

link_directories(/usr/local/lib/)
link_directories(/usr/local/Cellar/ffmpeg@2.8/2.8.11/lib/)
link_directories(/usr/local/Cellar/log4cpp/1.1.3/lib/)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES CameraServer.cpp include/CameraDevice.hpp src/CameraDevice.cpp
        include/CameraFramedSource.hpp src/CameraFramedSource.cpp
        include/CameraServerMediaSubsession.hpp src/CameraServerMediaSubsession.cpp
        include/Logger.hpp)

add_definitions(-DENCODE_H265)

add_executable(CameraServer ${SOURCE_FILES})

target_link_libraries(
        CameraServer
        avcodec
        avdevice
        avfilter
        avformat
        avresample
        avutil
        postproc
        swresample
        swscale
        BasicUsageEnvironment
        UsageEnvironment
        groupsock
        liveMedia
        log4cpp
)