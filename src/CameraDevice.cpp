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

#include "Logger.hpp"
#include <cstddef>

#include "CameraDevice.hpp"

static char s_filterDesc[]
        = "drawtext=fontfile=/System/Library/Fonts/STHeiti Medium.ttc:x=5:y=5:fontcolor=white:fontsize=24:shadowcolor=black:shadowx=2:shadowy=2:text=\\'%{localtime\\:%Y-%m-%d %H\\\\:%M\\\\:%S}\\'";

using namespace std;

string GenerateLocalDateTimeString()
{
    string ret;
    struct tm *local;
    time_t now;

    time(&now);
    local = localtime(&now);

    ret += string("drawtext=fontfile=/System/Library/Assets/com_apple_MobileAsset_Font3/06722e9d680cf43caeeaf24d40f37ebdddca37e2.asset/AssetData/Lantinghei.ttc:x=5:y=5:fontcolor=white:fontsize=24:shadowcolor=black:shadowx=2:shadowy=2:text=\\'");
    ret += string("%{localtime\\:%Y-%m-%d} ");
    string wday;
    switch (local->tm_wday)
    {
        case 0:
            wday = "星期日";
            break;
        case 1:
            wday = "星期一";
            break;
        case 2:
            wday = "星期二";
            break;
        case 3:
            wday = "星期三";
            break;
        case 4:
            wday = "星期四";
            break;
        case 5:
            wday = "星期五";
            break;
        case 6:
            wday = "星期六";
            break;
    }
    ret += wday + string(" ");
    ret += string("%{localtime\\:%H\\\\:%M\\\\:%S} ");
    ret += string("黄龙世纪广场");

    return ret;
}

CameraDevice::CameraDevice() :
        m_isReady(false),
        m_filterGraph(NULL),
        m_filterContextSrc(NULL),
        m_filterContextSink(NULL),
        m_filterSrc(NULL),
        m_filterSink(NULL),
        m_filterInOutIn(NULL),
        m_filterInOutOut(NULL),
        m_swsContext(NULL),
        m_frameYuv420p(NULL),
        m_frameYuyv422(NULL),
        m_codecRaw(NULL),
        m_codecContextRaw(NULL),
        m_codecH26X(NULL),
        m_codecContextH26X(NULL),
        m_packetRaw(NULL),
        m_packetH26X(NULL),
        m_inputFormat(NULL),
        m_formatContext(NULL)
{

}

CameraDevice::~CameraDevice()
{
    if (m_isReady)
    {
        Close();
    }
}

int CameraDevice::Create(const char* format, const char* device,
                         int width, int height, int fps)
{
    // System initialization
    av_register_all();
    avdevice_register_all();
    avcodec_register_all();
    avfilter_register_all();

    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUYV422, AV_PIX_FMT_NONE };

    // Open and initialize the camera device
    m_formatContext = avformat_alloc_context();
    if (m_formatContext == NULL)
    {
        LOG(ERROR) << "Cannot alloc context";
        return -1;
    }

    m_inputFormat = av_find_input_format(format);
    if (m_inputFormat == NULL)
    {
        LOG(ERROR) << "Cannot find input format:" << format;
        return -1;
    }

    AVDictionary* options = NULL;
    char args[512];
    snprintf(args, sizeof(args), "%d", fps);
    av_dict_set(&options, "framerate", args, 0);
    snprintf(args, sizeof(args), "%dx%d", width, height);
    av_dict_set(&options, "video_size", args, 0);
    av_dict_set(&options, "pixel_format", "yuyv422", 0);
    if (avformat_open_input(&m_formatContext, device, m_inputFormat, &options) < 0)
    {
        LOG(ERROR) << "Cannot open device:" << device;
        return -1;
    }

    // Initializes the video decoder
    if (avformat_find_stream_info(m_formatContext, NULL) < 0)
    {
        LOG(ERROR) << "Cannot find stream info";
        return -1;
    }
    int videoIndex = -1;
    for (int i = 0; i < m_formatContext->nb_streams; i++)
    {
        if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoIndex = i;
            break;
        }
    }
    if (videoIndex == -1)
    {
        LOG(ERROR) << "Cannot find video in media";
        return -1;
    }

    m_codecContextRaw = m_formatContext->streams[videoIndex]->codec;
    m_codecRaw = avcodec_find_decoder(m_codecContextRaw->codec_id);
    if (!m_codecRaw)
    {
        LOG(ERROR) << "Cannot find decoder";
        return -1;
    }
    if (avcodec_open2(m_codecContextRaw, m_codecRaw, NULL) < 0)
    {
        LOG(ERROR) << "Cannot open raw codec context";
        return -1;
    }

    // Initialize video frame structure
    m_frameYuyv422 = av_frame_alloc();
    m_frameYuyv422PlusDate = av_frame_alloc();
    m_frameYuv420p = av_frame_alloc();
    if ((m_frameYuyv422 == NULL) || (m_frameYuyv422PlusDate == NULL) || (m_frameYuv420p == NULL))
    {
        LOG(ERROR) << "Cannot alloc frame";
        return -1;
    }
    if ((avpicture_alloc((AVPicture*)m_frameYuyv422, AV_PIX_FMT_YUYV422, width, height) < 0)
        || (avpicture_alloc((AVPicture*)m_frameYuyv422PlusDate, AV_PIX_FMT_YUYV422, width, height) < 0)
        || (avpicture_alloc((AVPicture*)m_frameYuv420p, AV_PIX_FMT_YUV420P, width, height) < 0))
    {
        LOG(ERROR) << "Cannot alloc frame picture";
        return -1;
    }

    // Initialize video filter graph
    m_filterGraph = avfilter_graph_alloc();
    if (!m_filterGraph)
    {
        LOG(ERROR) << "Cannot alloc filter graph";
        return -1;
    }

    m_filterSrc = avfilter_get_by_name("buffer");
    m_filterSink = avfilter_get_by_name("ffbuffersink");
    if (!m_filterSrc || !m_filterSink)
    {
        LOG(ERROR) << "Cannot get filter";
        return -1;
    }

    m_filterInOutIn = avfilter_inout_alloc();
    m_filterInOutOut = avfilter_inout_alloc();
    if (!m_filterInOutIn || !m_filterInOutOut)
    {
        LOG(ERROR) << "Cannot alloc filter inout";
        return -1;
    }

    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             m_codecContextRaw->width, m_codecContextRaw->height, m_codecContextRaw->pix_fmt,
             m_codecContextRaw->time_base.num, m_codecContextRaw->time_base.den,
             m_codecContextRaw->sample_aspect_ratio.num, m_codecContextRaw->sample_aspect_ratio.den);
    LOG(INFO) << args;

    if ((avfilter_graph_create_filter(&m_filterContextSrc, m_filterSrc, "in", args, NULL, m_filterGraph) < 0)
        || (avfilter_graph_create_filter(&m_filterContextSink, m_filterSink, "out", NULL, NULL, m_filterGraph) < 0))
    {
        LOG(ERROR) << "Cannot alloc filter inout";
        return -1;
    }

    av_opt_set_int_list(m_filterContextSink, "pix_fmts", pix_fmts,
                        AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

    /* Endpoints for the filter graph. */
    m_filterInOutOut->name = av_strdup("in");
    m_filterInOutOut->filter_ctx = m_filterContextSrc;
    m_filterInOutOut->pad_idx = 0;
    m_filterInOutOut->next = NULL;

    m_filterInOutIn->name = av_strdup("out");
    m_filterInOutIn->filter_ctx = m_filterContextSink;
    m_filterInOutIn->pad_idx = 0;
    m_filterInOutIn->next = NULL;

    if (avfilter_graph_parse(m_filterGraph, GenerateLocalDateTimeString().c_str(),
                             m_filterInOutIn, m_filterInOutOut, NULL) < 0)
    {
        LOG(ERROR) << "Cannot parse filter graph";
        return -1;
    }

    if (avfilter_graph_config(m_filterGraph, NULL) < 0)
    {
        LOG(ERROR) << "Cannot config filter graph";
        return -1;
    }

    // Initialize frame scale context
    m_swsContext = sws_getContext(m_codecContextRaw->width, m_codecContextRaw->height,
                                  m_codecContextRaw->pix_fmt, m_codecContextRaw->width, m_codecContextRaw->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    // Initializes video packet structure
    m_packetRaw = (AVPacket *)av_malloc(sizeof(AVPacket));
    m_packetH26X = (AVPacket *)av_malloc(sizeof(AVPacket));
    if ((m_packetRaw == NULL) || (m_packetH26X == NULL))
    {
        LOG(ERROR) << "Cannot malloc packet";
        return -1;
    }

    // Initializes the video encoder
#ifdef ENCODE_H265
    m_codecH26X = avcodec_find_encoder(AV_CODEC_ID_H265);
#else
    m_codecH26X = avcodec_find_encoder(AV_CODEC_ID_H264);
#endif
    if (!m_codecH26X)
    {
        LOG(ERROR) << "Cannot find h264/h265 encoder";
        return -1;
    }
    m_codecContextH26X = avcodec_alloc_context3(m_codecH26X);
    if (!m_codecContextH26X)
    {
        LOG(ERROR) << "Cannot alloc h264/h265 codec context";
        return -1;
    }
#ifdef ENCODE_H265
    /* put sample parameters */
    m_codecContextH26X->bit_rate = 0;
    m_codecContextH26X->codec_type = AVMEDIA_TYPE_VIDEO;
    /* resolution must be a multiple of two */
    m_codecContextH26X->width = width;
    m_codecContextH26X->height = height;
    //m_codecContextH26X->frame_number = 1;
    /* frames per second */
    m_codecContextH26X->time_base = (AVRational){1, fps};
    m_codecContextH26X->gop_size = 1; /* emit one intra frame every ten frames */
    m_codecContextH26X->max_b_frames = 0;
    m_codecContextH26X->pix_fmt = AV_PIX_FMT_YUV420P;
    //av_opt_set(m_codecContextH26X->priv_data, "x265-params", "qp=20", 0);
    av_opt_set(m_codecContextH26X->priv_data, "preset", "ultrafast", 0);
    av_opt_set(m_codecContextH26X->priv_data, "tune", "zerolatency", 0);
#else
    /* put sample parameters */
    m_codecContextH26X->bit_rate = 0;
    m_codecContextH26X->codec_type = AVMEDIA_TYPE_VIDEO;
    /* resolution must be a multiple of two */
    m_codecContextH26X->width = width;
    m_codecContextH26X->height = height;
    //m_codecContextH264->frame_number = 1;
    /* frames per second */
    m_codecContextH26X->time_base = (AVRational){1, fps};
    m_codecContextH26X->gop_size = 1; /* emit one intra frame every ten frames */
    m_codecContextH26X->max_b_frames = 0;
    m_codecContextH26X->pix_fmt = AV_PIX_FMT_YUV420P;
    av_opt_set(m_codecContextH26X->priv_data, "preset", "ultrafast", 0);
    av_opt_set(m_codecContextH26X->priv_data, "tune", "zerolatency", 0);
#endif
    if (avcodec_open2(m_codecContextH26X, m_codecH26X, NULL) < 0)
    {
        LOG(ERROR) << "Cannot open h264/h265 codec context";
        return -1;
    }

    m_isReady = true;
    return 0;
}

void CameraDevice::Close()
{
    if (m_isReady)
    {
        m_isReady = false;
        avfilter_graph_free(&m_filterGraph);
        avcodec_close(m_codecContextRaw);
        avcodec_close(m_codecContextH26X);
        av_free_packet(m_packetRaw);
        av_free_packet(m_packetH26X);
        av_frame_free(&m_frameYuv420p);
        av_frame_free(&m_frameYuyv422);
        avformat_close_input(&m_formatContext);
        avformat_free_context(m_formatContext);
    }
}

int CameraDevice::Read(char *buffer, size_t bufferSize, size_t* truncatedSize)
{
    int size = 0;

    if (!m_isReady)
    {
        return -1;
    }

    // Read video data packet
    if (av_read_frame(m_formatContext, m_packetRaw) < 0)
    {
        LOG(ERROR) << "Cannot read frame";
        return -1;
    }

    // Decode data to frame yuyv422
    int got_frame = 0;
    if (avcodec_decode_video2(m_codecContextRaw, m_frameYuyv422, &got_frame, m_packetRaw) < 0)
    {
        LOG(ERROR) << "Cannot decode video";
        return -1;
    }
    if (!got_frame)
    {
        LOG(ERROR) << "Cannot get frame";
        return -1;
    }

    m_frameYuyv422->pts = av_frame_get_best_effort_timestamp(m_frameYuyv422);
    if (av_buffersrc_add_frame_flags(m_filterContextSrc, m_frameYuyv422, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
    {
        LOG(ERROR) << "Cannot add frame to src buffer";
        return -1;
    }

    if (av_buffersink_get_frame(m_filterContextSink, m_frameYuyv422PlusDate) < 0)
    {
        LOG(ERROR) << "Cannot get frame from sink buffer";
        return -1;
    }

    // Convert frame format from yuyv422 to yuv420p
    if (sws_scale(m_swsContext, (const uint8_t* const*)m_frameYuyv422PlusDate->data, m_frameYuyv422PlusDate->linesize,
                  0, m_codecContextRaw->height, m_frameYuv420p->data, m_frameYuv420p->linesize) < 0)
    {
        LOG(ERROR) << "Cannot convert frame from yuyv422 to yuv420p";
        return -1;
    }

    // Encode frame to h264/h265
    m_frameYuv420p->format = AV_PIX_FMT_YUV420P;
    m_frameYuv420p->width = m_codecContextH26X->width;
    m_frameYuv420p->height = m_codecContextH26X->height;
    int got_packet = 0;
    memset(m_packetH26X, 0, sizeof(AVPacket));
    av_init_packet(m_packetH26X);
    if (avcodec_encode_video2(m_codecContextH26X, m_packetH26X, m_frameYuv420p, &got_packet) < 0)
    {
        LOG(ERROR) << "Cannot encode h264 video";
        return -1;
    }

    if (!got_packet)
    {
        LOG(INFO) << "Cannot get h264 video packet";
        return -1;
    }

    *truncatedSize = 0;
    if (m_packetH26X->size > bufferSize)
    {
        size = bufferSize;
        LOG(WARN) << "Device buffer truncated available:" << bufferSize << " needed:" << m_packetH26X->size;
        *truncatedSize = (size_t)(m_packetH26X->size - bufferSize);
    }
    else
    {
        size = m_packetH26X->size;
    }
    memcpy(buffer, m_packetH26X->data, size);

    return size;
}
