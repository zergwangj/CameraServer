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
#include "CameraServerMediaSubsession.hpp"
#include "CameraFramedSource.hpp"

using namespace std;

CameraServerMediaSubsession* CameraServerMediaSubsession::createNew(UsageEnvironment& env, const char* format,
    const char* device, int width, int height, int fps)
{
    return new CameraServerMediaSubsession(env, format, device, width, height, fps);
}

CameraServerMediaSubsession::CameraServerMediaSubsession(UsageEnvironment& env, const char* format,
    const char* device, int width, int height, int fps)
    : OnDemandServerMediaSubsession(env, True),
      mAuxSDPLine(NULL), mDoneFlag(0), mDummyRTPSink(NULL),
      mWidth(width), mHeight(height), mFps(fps)
{
    mFormat = strDup(format);
    mDevice = strDup(device);
}

CameraServerMediaSubsession::~CameraServerMediaSubsession()
{
    if (mAuxSDPLine)
    {
        delete[] mAuxSDPLine;
    }

    if (mDevice)
    {
        delete[] mDevice;
    }
}

static void afterPlayingDummy(void* clientData) 
{
    CameraServerMediaSubsession* subsess = (CameraServerMediaSubsession*)clientData;
    subsess->afterPlayingDummy1();
}

void CameraServerMediaSubsession::afterPlayingDummy1()
{
    // Unschedule any pending 'checking' task:
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    // Signal the event loop that we're done:
    setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) 
{
    CameraServerMediaSubsession* subsess = (CameraServerMediaSubsession*)clientData;
    subsess->checkForAuxSDPLine1();
}

void CameraServerMediaSubsession::checkForAuxSDPLine1()
{
    char const* dasl;

    if (mAuxSDPLine != NULL) 
    {
        // Signal the event loop that we're done:
        setDoneFlag();
    } 
    else if (mDummyRTPSink != NULL && (dasl = mDummyRTPSink->auxSDPLine()) != NULL) 
    {

        mAuxSDPLine = strDup(dasl);
        mDummyRTPSink->stopPlaying();
        mDummyRTPSink = NULL;

        // Signal the event loop that we're done:
        setDoneFlag();
    } 
    else if (!mDoneFlag) 
    {
        // try again after a brief delay:
        double delay = 10;  // ms  
        int uSecsToDelay = delay * 1000;  // us  
        nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
            (TaskFunc*)checkForAuxSDPLine, this);
    }
}

char const* CameraServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource)
{
    if (mAuxSDPLine != NULL) return mAuxSDPLine; // it's already been set up (for a previous client)

    if (mDummyRTPSink == NULL) 
    { 
        // we're not already setting it up for another, concurrent stream
        // Note: For H264 video files, the 'config' information ("profile-level-id" and "sprop-parameter-sets") isn't known
        // until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
        // and we need to start reading data from our file until this changes.
        mDummyRTPSink = rtpSink;

        // Start reading the file:
        //mDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);
        mDummyRTPSink->startPlaying(*inputSource, NULL, NULL);

        // Check whether the sink's 'auxSDPLine()' is ready:
        checkForAuxSDPLine(this);
    }

    envir().taskScheduler().doEventLoop(&mDoneFlag);

    return mAuxSDPLine;
}

FramedSource* CameraServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
#ifdef ENCODE_H265
    estBitrate = 1000;

    // Create the video source:
    CameraFramedSource* cameraSource = CameraFramedSource::createNew(envir(),
                                                                     mFormat, mDevice, mWidth, mHeight, mFps);
    if (cameraSource == NULL) return NULL;

    return H265VideoStreamFramer::createNew(envir(), cameraSource);
#else
    estBitrate = 500;  

    // Create the video source:
    CameraFramedSource* cameraSource = CameraFramedSource::createNew(envir(),
                                                                     mFormat, mDevice, mWidth, mHeight, mFps);
    if (cameraSource == NULL) return NULL;

    return H264VideoStreamFramer::createNew(envir(), cameraSource);
#endif
}

RTPSink* CameraServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
#ifdef ENCODE_H265
    return H265VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
#else
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
#endif
}
