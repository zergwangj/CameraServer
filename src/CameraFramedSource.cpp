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

#include <iostream>
#include "Logger.hpp"
#include "CameraFramedSource.hpp"

using namespace std;

CameraFramedSource *CameraFramedSource::createNew(UsageEnvironment &env,
                                                  const char *format, const char *device, int width, int height,
                                                  int fps) {
    CameraDevice *cameraDevice = new CameraDevice();
    if (NULL == cameraDevice) {
        return NULL;
    }

    CameraFramedSource *newSource = new CameraFramedSource(env, cameraDevice);

    if (!cameraDevice->Create(newSource, CameraFramedSource::CameraCapture, format, device, width, height, fps)) {
        delete cameraDevice;
        delete newSource;
        return NULL;
    }

    return newSource;
}

CameraFramedSource::CameraFramedSource(UsageEnvironment &env, CameraDevice *cameraDevice)
        : FramedSource(env), cameraDevice_(cameraDevice) {
}

CameraFramedSource::~CameraFramedSource() {
    delete cameraDevice_;
    envir().taskScheduler().unscheduleDelayedTask(taskToken_);
}

void CameraFramedSource::doGetNextFrame() {
    taskToken_ = envir().taskScheduler().scheduleDelayedTask(0,
                                                             getNextFrame, this);
}

void CameraFramedSource::getNextFrame(void *ptr) {
    ((CameraFramedSource *) ptr)->getNextFrame1();
}

void CameraFramedSource::getNextFrame1() {
    if (!frameBufferQueue_.empty()) {
        FrameBuffer *frameBuffer = frameBufferQueue_.front();
        memmove(fTo, frameBuffer->data, frameBuffer->size);
        fFrameSize = frameBuffer->size;
        fNumTruncatedBytes = frameBuffer->truncatedSize;
        frameBufferQueue_.pop();
        frameBufferPool_.free(frameBuffer);
    } else {
        cameraDevice_->Capture();
    }

    // notify  
    afterGetting(this);
}

unsigned int CameraFramedSource::maxFrameSize() const {
    return MAX_FRAME_SIZE;
}

void CameraFramedSource::CameraCapture(void *param, void *packet, size_t bytes) {
    CameraFramedSource *framedSource = (CameraFramedSource *) param;
    int frameSize = 0, truncatedSize = 0;

    FrameBuffer *frameBuffer = framedSource->frameBufferPool_.malloc();
    if (bytes > MAX_FRAME_SIZE) {
        frameSize = MAX_FRAME_SIZE;
        truncatedSize = bytes - MAX_FRAME_SIZE;
    } else {
        frameSize = bytes;
        truncatedSize = 0;
    }
    memcpy(frameBuffer->data, packet, frameSize);
    frameBuffer->size = frameSize;
    frameBuffer->truncatedSize = truncatedSize;
    framedSource->frameBufferQueue_.push(frameBuffer);
}
