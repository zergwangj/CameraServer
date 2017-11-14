/*
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/
#ifndef _CAMERA_DEVICE_HPP
#define _CAMERA_DEVICE_HPP

#ifdef __cplusplus
extern "C" {
#endif

#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>

#ifdef __cplusplus
}
#endif

#include <string>

class CameraDevice {
public:
    CameraDevice();
    virtual ~CameraDevice();

    int Create(const char* format, const char* device, int width, int height, int fps);
    void Close();

    int Read(char* buffer, size_t bufferSize, size_t *truncatedSize);
    bool IsReady() { return m_isReady; }

protected:
    AVFormatContext *m_formatContext;
    AVInputFormat *m_inputFormat;
    AVPacket *m_packetRaw, *m_packetH26X;
    AVCodecContext *m_codecContextRaw, *m_codecContextH26X;
    AVCodec *m_codecRaw, *m_codecH26X;
    AVFrame *m_frameYuyv422, *m_frameYuyv422PlusDate, *m_frameYuv420p;
    AVFilterGraph *m_filterGraph;
    AVFilterContext *m_filterContextSrc;
    AVFilterContext *m_filterContextSink;
    AVFilter *m_filterSrc;
    AVFilter *m_filterSink;
    AVFilterInOut * m_filterInOutIn;
    AVFilterInOut * m_filterInOutOut;
    struct SwsContext* m_swsContext;

    bool m_isReady;
};

#endif //CAMERASERVER_CAMERADEVICE_HPP
