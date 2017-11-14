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

//#define MAX_FRAME_SIZE (100 * 1024)
#define MAX_FRAME_SIZE 150000

CameraFramedSource* CameraFramedSource::createNew(UsageEnvironment& env,
                                                  const char* format, const char* device, int width, int height, int fps)
{
    CameraDevice* cameraDevice = new CameraDevice();
    if (NULL == cameraDevice)
    {
        return NULL;
    }

    if (cameraDevice->Create(format, device, width, height, fps) != 0)
    {
        delete cameraDevice;
        return NULL;
    }

    CameraFramedSource* newSource = new CameraFramedSource(env, cameraDevice);

    return newSource;
}

CameraFramedSource::CameraFramedSource(UsageEnvironment& env, CameraDevice* cameraDevice)
    : FramedSource(env), m_cameraDevice(cameraDevice)
{
}

CameraFramedSource::~CameraFramedSource()
{
    delete m_cameraDevice;
    envir().taskScheduler().unscheduleDelayedTask(m_taskToken);
}

void CameraFramedSource::doGetNextFrame()
{
    m_taskToken = envir().taskScheduler().scheduleDelayedTask(0,
        getNextFrame, this);
}

void CameraFramedSource::getNextFrame(void* ptr)
{  
    ((CameraFramedSource*)ptr)->getNextFrame1();
} 

void CameraFramedSource::getNextFrame1()
{
    int frameSize;
    size_t truncatedSize;

    char buf[MAX_FRAME_SIZE];

    if ((frameSize = m_cameraDevice->Read(buf, MAX_FRAME_SIZE, &truncatedSize)) < 0)
    {
        LOG(ERROR) << "Cannot read h264 packet from device";
        return;
    }

    memmove(fTo, buf, frameSize);
    fFrameSize = frameSize;
    fNumTruncatedBytes = truncatedSize;

    // notify  
    afterGetting(this); 
}

unsigned int CameraFramedSource::maxFrameSize() const
{
    return MAX_FRAME_SIZE;
}




