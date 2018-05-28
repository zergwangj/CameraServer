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

#ifndef __CAMERA_DEVICE_HPP__
#define __CAMERA_DEVICE_HPP__

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

typedef void (*CameraCallback)(void *param, void *packet, size_t bytes);

class CameraDevice {
public:
    CameraDevice();

    virtual ~CameraDevice();

    bool Create(void *param, CameraCallback callback,
                const char *format, const char *device, int width, int height, int fps);

    void Close();

    void Capture();

    bool IsReady() { return isReady_; }

private:
    void *param_;
    CameraCallback callback_;

    AVFormatContext *formatContext_;
    AVPacket *packetRaw_, *packetH26X_;
    AVCodecContext *codecContextRaw_, *codecContextH26X_;
    AVFrame *frameYuyv422_, *frameYuyv422Watermark_, *frameYuv420p_;
    AVFilterGraph *filterGraph_;
    AVFilterContext *filterContextSrc_;
    AVFilterContext *filterContextSink_;
    struct SwsContext *swsContext_;

    bool isReady_;
};


#endif // __CAMERA_DEVICE_HPP__
