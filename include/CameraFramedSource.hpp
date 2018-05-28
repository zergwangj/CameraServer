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

#ifndef __CAMERA_FRAMED_SOURCE_HPP__
#define __CAMERA_FRAMED_SOURCE_HPP__

#include <liveMedia.hh>
#include <queue>
#include <boost/pool/object_pool.hpp>
#include "CameraDevice.hpp"

//#define MAX_FRAME_SIZE (100 * 1024)
#define MAX_FRAME_SIZE 150000

struct FrameBuffer {
    char data[MAX_FRAME_SIZE];
    int size;
    int truncatedSize;
};

class CameraFramedSource : public FramedSource {
public:
    static CameraFramedSource *createNew(UsageEnvironment &env,
                                         const char *format, const char *device, int width, int height, int fps);

    static void getNextFrame(void *ptr);

    void getNextFrame1();

protected:
    CameraFramedSource(UsageEnvironment &env, CameraDevice *cameraDevice);

    ~CameraFramedSource();

    static void CameraCapture(void *param, void *packet, size_t bytes);

    virtual void doGetNextFrame();

    virtual unsigned int maxFrameSize() const;

private:
    void *taskToken_;
    CameraDevice *cameraDevice_;
    boost::object_pool<FrameBuffer> frameBufferPool_;
    typedef std::queue<FrameBuffer *> FrameBufferQueue;
    FrameBufferQueue frameBufferQueue_;
};

#endif // __CAMERA_FRAMED_SOURCE_HPP__
