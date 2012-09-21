/*
 * ----------------------------------------------------------------------
 *  TkFFMPEG:  video
 *
 *  These routines support the methods in the "video" class, which is
 *  a video stream that can be read from or written to.  The class
 *  itself is defined in itcl, but when methods are called, execution
 *  jumps down to this level.
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#include <stdlib.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_FFMPEG_AVCODEC_H
# include <ffmpeg/avcodec.h>
#endif

#ifdef HAVE_LIBAVCODEC_AVCODEC_H
# include <libavcodec/avcodec.h>
#endif

#ifdef HAVE_FFMPEG_AVFORMAT_H
# include <ffmpeg/avformat.h>
#endif

#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
# include <libavformat/avformat.h>
#endif

#ifdef HAVE_FFMPEG_AVUTIL_H
# include <ffmpeg/avutil.h>
#endif

#ifdef HAVE_LIBAVUTIL_AVUTIL_H
# include <libavutil/avutil.h>
#endif

#ifdef HAVE_FFMPEG_SWSCALE_H
# include <ffmpeg/swscale.h>
#endif

#ifdef HAVE_LIBSWSCALE_SWSCALE_H
# include <libswscale/swscale.h>
#endif

#include "RpVideo.h"

#ifndef HAVE_AVMEDIA_TYPE_VIDEO
#define AVMEDIA_TYPE_VIDEO	CODEC_TYPE_VIDEO
#endif	/* HAVE_AVMEDIA_TYPE_VIDEO */

#ifndef AV_PKT_FLAG_KEY
#define AV_PKT_FLAG_KEY		PKT_FLAG_KEY		
#endif

#ifndef HAVE_AVIO_CLOSE
#define avio_close		url_fclose
#endif

/*
 * Each video object is represented by the following data:
 */
struct VideoObjRec {
    int magic;

    /* video input */
    AVFormatContext *pFormatCtx;
    int videoStream;
    int frameNumber;
    int atEnd;

    /* video output */
    AVFormatContext *outFormatCtx;
    AVStream *outVideoStr;

    /* used for both input/output */
    AVFrame *pFrameYUV;
    uint8_t *yuvbuffer;
    int yuvw, yuvh;
    AVFrame *pFrameRGB;
    uint8_t *rgbbuffer;
    int rgbw, rgbh;
    struct SwsContext *scalingCtx;

    char *fileName;
    char mode[64];
    char fmt[64];
    int lastframe;

    /* tmp buffer to give images back to user */
    void *img;
    int imgHeaderLen;
    int imgWidth;
    int imgHeight;
};

/* magic stamp for VideoObj, to make sure data is valid */
#define VIDEO_OBJ_MAGIC 0x0102abcd

static VideoObj *VideoSetData ();

static int VideoModeRead (VideoObj *vidPtr);
// static int VideoModeWrite (Tcl_Interp *interp, int w, int h);

static int VideoTime2Frame (AVStream *streamPtr, int64_t tval);
static int64_t VideoFrame2Time (AVStream *streamPtr, int fval);
static void VideoNextFrame (VideoObj *vidPtr);

uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;
static int VideoAvGetBuffer (struct AVCodecContext *c, AVFrame *fr);
static void VideoAvReleaseBuffer (struct AVCodecContext *c, AVFrame *fr);
static int VideoWriteFrame (VideoObj *vidPtr, AVFrame *framePtr);

static int VideoAllocImgBuffer (VideoObj *vidPtr, int width, int height);
static int VideoFreeImgBuffer (VideoObj *vidPtr);
static double VideoTransformFrames2Duration (VideoObj *vidPtr, int frame);
static int VideoTransformDuration2Frames (VideoObj *vidPtr, double duration);

/*
 * ------------------------------------------------------------------------
 *  VideoSetData()
 *
 *  Saves VideoObj data in the "_videodata" slot in the current object
 *  context.  The data can be retrieved later by calling VideoGetData().
 * ------------------------------------------------------------------------
 */
VideoObj *
VideoSetData()
{
    VideoObj* vid = NULL;

    vid = malloc(sizeof(VideoObj));

    if (vid == NULL) {
        return NULL;
    }

    vid->magic = VIDEO_OBJ_MAGIC;
    vid->pFormatCtx = NULL;
    vid->videoStream = 0;
    vid->frameNumber = -1;
    vid->atEnd = 0;

    vid->outFormatCtx = NULL;
    vid->outVideoStr = NULL;

    vid->pFrameYUV = NULL;
    vid->yuvbuffer = NULL;
    vid->yuvw = 0;
    vid->yuvh = 0;
    vid->pFrameRGB = NULL;
    vid->rgbbuffer = NULL;
    vid->rgbw = 0;
    vid->rgbh = 0;
    vid->scalingCtx = NULL;

    vid->fileName = NULL;
    *vid->mode = '\0';
    *vid->fmt = '\0';
    vid->lastframe = 0;

    vid->img = NULL;
    vid->imgHeaderLen = 0;
    vid->imgWidth = 0;
    vid->imgHeight = 0;

    return vid;
}

/*
 * ------------------------------------------------------------------------
 *  VideoFindLastFrame()
 *
 *  Find the last readable frame.
 * ------------------------------------------------------------------------
 */
int
VideoFindLastFrame(vidPtr,lastframe)
    VideoObj *vidPtr;
    int *lastframe;
{
    int f = 0;
    int nframe = 0;
    int cur = 0;
    AVStream *vstreamPtr;

    if (vidPtr == NULL) {
        return -1;
    }

    if (lastframe == NULL) {
        return -1;
    }

    if (VideoModeRead(vidPtr) != 0) {
        return -1;
    }

    // calculate an estimate of the last frame
    vstreamPtr = vidPtr->pFormatCtx->streams[vidPtr->videoStream];
    nframe = VideoTime2Frame(vstreamPtr,
        vstreamPtr->start_time + vstreamPtr->duration);

    // get the real last readable frame
    // is 50 frames far enough to go back
    // to be outside of the last key frame?
    f = vidPtr->frameNumber;
    cur = VideoGoToN(vidPtr,nframe-50);
    while (cur != nframe) {
        cur = nframe;
        nframe = VideoGoNext(vidPtr);
    }
    *lastframe = nframe;
    VideoGoToN(vidPtr,f);

    return 0;
}


int
VideoOpenFile(vidPtr, fileName, mode)
    VideoObj *vidPtr;
    const char *fileName;
    const char *mode;
{
    int fnlen = 0;
    int err = 0;
    int lastframe = 0;

    if (fileName == NULL) {
        // missing value for fileName
        // return TCL_ERROR;
        return -1;
    }
    if (fileName == '\0') {
        /* no file name set -- do nothing */
        return 0;
    }

    fnlen = strlen(fileName);
    if (vidPtr->fileName != NULL) {
        free(vidPtr->fileName);
    }
    vidPtr->fileName = (char *) malloc((fnlen+1)*sizeof(char));
    if (vidPtr->fileName == NULL) {
        // trouble mallocing space
        return -1;
    }
    strncpy(vidPtr->fileName,fileName,fnlen);
    vidPtr->fileName[fnlen] = '\0';

    // FIXME: remove this constraint when we support
    // the modes: r, r+, w, w+, a, a+, b and combinations
    if (strlen(mode) > 1) {
        return -1;
    }

    if (*mode == 'r') {
        /* we're now in "input" mode */
        err = VideoModeRead(vidPtr);
        if (err) {
            return err;
        }

        VideoFindLastFrame(vidPtr,&lastframe);
        vidPtr->lastframe = lastframe;
    } else if (*mode == 'w') {
        /* we're now in "input" mode */
        // VideoModeWrite(vidPtr);
    } else {
        // unrecognized mode
        return -1;
    }

    return 0;
}


/*
 * ------------------------------------------------------------------------
 *  VideoModeRead()
 *
 *  Tries to force this video stream into "read" mode.  If the current
 *  mode is "", then the -file is opened for reading.  If the current
 *  mode is "write", then the stream is closed and then opened for
 *  reading.  If the current mode is "read", then this call does nothing.
 *  Usually called just before a "read" operation (get, go, etc.) is
 *  performed.
 *
 *  Returns TCL_OK if successful, and TCL_ERROR if there is a problem
 *  opening or closing the stream.
 *
 *  Error Codes
 *  -1
 *  -2      missing file name
 *  -3      couldn't open file
 *  -4      couldn't find streams in file
 *  -5      couldn't find video stream in file
 *  -6      unsupported codec for file
 *  -7      couldn't open codec for file
 *  -8      couldn't allocate frame space
 *  -9      strcpy input to vidPtr->mode failed
 * ------------------------------------------------------------------------
 */
int
VideoModeRead(vidPtr)
    VideoObj *vidPtr;
{
    int i;
    const char *fmt;
    AVCodecContext *vcodecCtx;
    AVCodec *vcodec;

    if (vidPtr == NULL) {
        return -1;
    }

    if (vidPtr->fileName == NULL) {
        // Tcl_AppendResult(interp, "missing value for -file", (char*)NULL);
        // return TCL_ERROR;

        // missing file name
        return -2;
    }
    if (*vidPtr->fileName == '\0') {
        /* no file name set -- do nothing */
        return 0;
    }

    if (strcmp(vidPtr->mode,"input") == 0) {
        return 0;
    } else if (strcmp(vidPtr->mode,"output") == 0) {
        if (VideoClose(vidPtr) != 0) {
            return -1;
        }
    }

    /*
     * Open the video stream from that file.
     */
#ifdef HAVE_AVFORMAT_OPEN_INPUT
    if (avformat_open_input(&vidPtr->pFormatCtx, vidPtr->fileName, NULL, 
	NULL) != 0) {
	return -3;
    }
#else
    if (av_open_input_file(&vidPtr->pFormatCtx, vidPtr->fileName,
            NULL, 0, NULL) != 0) {
        return -3;
    }
#endif
#ifdef HAVE_AVFORMAT_FIND_STREAM_INFO
    if (avformat_find_stream_info(vidPtr->pFormatCtx, NULL) < 0) {
#else
    if (av_find_stream_info(vidPtr->pFormatCtx) < 0) {
#endif
        // Tcl_AppendResult(interp, "couldn't find streams in file \"",
        //     fileName, "\"", (char*)NULL);
        // return TCL_ERROR;

        // couldn't find streams in file
        return -4;
    }

    /*
     * Search for a video stream and its codec.
     */
    vidPtr->videoStream = -1;
    for (i=0; i < vidPtr->pFormatCtx->nb_streams; i++) {
        if (vidPtr->pFormatCtx->streams[i]->codec->codec_type 
	    == AVMEDIA_TYPE_VIDEO) {
            vidPtr->videoStream = i;
            break;
        }
    }
    if (vidPtr->videoStream < 0) {
        // Tcl_AppendResult(interp, "couldn't find video stream in file \"",
        //     fileName, "\"", (char*)NULL);
        // return TCL_ERROR;

        // couldn't find video stream in file
        return -5;
    }

    vcodecCtx = vidPtr->pFormatCtx->streams[vidPtr->videoStream]->codec;
    vcodec = avcodec_find_decoder(vcodecCtx->codec_id);
    if (vcodec == NULL) {
        // Tcl_AppendResult(interp, "unsupported codec for file \"",
        //     fileName, "\"", (char*)NULL);
        // return TCL_ERROR;

        // unsupported codec for file
        return -6;
    }
#ifdef HAVE_AVCODEC_OPEN2
    if (avcodec_open2(vcodecCtx, vcodec, NULL) < 0) {
#else
    if (avcodec_open(vcodecCtx, vcodec) < 0) {
#endif
        // Tcl_AppendResult(interp, "couldn't open codec for file \"",
        //     fileName, "\"", (char*)NULL);
        // return TCL_ERROR;

        // couldn't open codec for file
        return -7;
    }

    vcodecCtx->get_buffer = VideoAvGetBuffer;
    vcodecCtx->release_buffer = VideoAvReleaseBuffer;

    vidPtr->pFrameYUV = avcodec_alloc_frame();
    vidPtr->pFrameRGB = avcodec_alloc_frame();
    if (vidPtr->pFrameYUV == NULL || vidPtr->pFrameRGB == NULL) {
        // Tcl_AppendResult(interp, "couldn't allocate frame space",
        //     " for file \"", fileName, "\"", (char*)NULL);
        // return TCL_ERROR;

        // couldn't allocate frame space
        return -8;
    }

    /* save the name of the codec as the -format option */
    fmt = "?";
    if (vcodecCtx->codec && vcodecCtx->codec->name) {
        fmt = vcodecCtx->codec->name;
        strcpy(vidPtr->fmt,fmt);
    }
//
//    sprintf(buffer, "%d", vcodecCtx->width);
//    if (Tcl_SetVar(interp, "width", buffer, TCL_LEAVE_ERR_MSG) == NULL) {
//        return TCL_ERROR;
//    }
//    sprintf(buffer, "%d", vcodecCtx->height);
//    if (Tcl_SetVar(interp, "height", buffer, TCL_LEAVE_ERR_MSG) == NULL) {
//        return TCL_ERROR;
//    }
//

    if (strcpy(vidPtr->mode,"input") == NULL) {
        // strcpy input to vidPtr->mode failed
        return -9;
    }

    return 0;
}


// FIXME: get this function working.
///*
// * ------------------------------------------------------------------------
// *  VideoModeWrite()
// *
// *  Tries to force this video stream into "write" mode.  If the current
// *  mode is "", then the -file is opened for writing.  If the current
// *  mode is "read", then the stream is closed and then opened for
// *  writing.  If the current mode is "write", then this call does nothing.
// *  Usually called just before a "write" operation (put, etc.) is
// *  performed.
// *
// *  Returns TCL_OK if successful, and TCL_ERROR if there is a problem
// *  opening or closing the stream.
// * ------------------------------------------------------------------------
// */
//int
//VideoModeWrite(vidPtr, fileName, width, height, fmt)
//    VideoObj *vidPtr;      /* video object to write */
//    CONST84 char *fileName;
//    int width;             /* native width of each frame */
//    int height;            /* native height of each frame */
//    CONST84 char *fmt
//{
//    char c;
//    int numBytes, pixfmt, iwd, iht;
//    CONST84 char *size;
//    AVCodecContext *codecCtx;
//    AVCodec *vcodec;
//
//    if (vidPtr == NULL) {
//        return -1;
//    }
//
//    /*
//     * Get the current mode.  If we're already in "output", then we're
//     * done.  Otherwise, close the stream if necessary and prepare to
//     * open the file for write.
//     */
//    if (vidPtr->mode == NULL) {
//        return -1;
//    }
//
//    c = *vidPtr->mode;
//    if (c == 'o' && strcmp(vidPtr->mode,"output") == 0) {
//        return 0;
//    }
//    else if (c == 'i' && strcmp(vidPtr->mode,"input") == 0) {
//        if (VideoClose(vidPtr) != 0) {
//            return -1;
//        }
//    }
//
//    /*
//     * Get the file name from the -file variable.
//     */
//    if ((fileName == NULL) || (*filename == '\0')) {
//        /* no file name set -- do nothing */
//        return 0;
//    }
//
//    /*
//     * Get the -width and -height of each frame.  If these are set
//     * to 0 (default), then use the incoming width/height from an
//     * actual frame.
//     */
//     iwd = width;
//     iht = height;
//
//    /*
//     * Get the format argument.
//     */
//    if (fmt == NULL) {
////        Tcl_AppendResult(interp, "missing value for -format", (char*)NULL);
////        return TCL_ERROR;
//        return -1;
//    }
//    if (strcmp(fmt,"mpeg1video") == 0) {
//        vidPtr->outFormatCtx = av_alloc_format_context();
//        vidPtr->outFormatCtx->oformat = guess_format("mpeg", NULL, NULL);
//    }
//    else if (strcmp(fmt,"flv") == 0) {
//        vidPtr->outFormatCtx = av_alloc_format_context();
//        vidPtr->outFormatCtx->oformat = guess_format("flv", NULL, NULL);
//    }
//    else if (strcmp(fmt,"mov") == 0) {
//        vidPtr->outFormatCtx = av_alloc_format_context();
//        vidPtr->outFormatCtx->oformat = guess_format("mov", NULL, NULL);
//        /* MOV normally uses MPEG4, but that may not be installed */
//        vidPtr->outFormatCtx->oformat->video_codec = CODEC_ID_FFV1;
//    }
//    else if (strcmp(fmt,"avi") == 0) {
//        vidPtr->outFormatCtx = av_alloc_format_context();
//        vidPtr->outFormatCtx->oformat = guess_format("avi", NULL, NULL);
//        /* AVI normally uses MPEG4, but that may not be installed */
//        vidPtr->outFormatCtx->oformat->video_codec = CODEC_ID_FFV1;
//    }
//    else {
////        Tcl_AppendResult(interp, "bad format \"", fmt, "\": should be",
////            " avi, flv, mpeg1video, mov", (char*)NULL);
////        return TCL_ERROR;
//        return -1;
//    }
//
//    /*
//     * Open the video stream for writing.
//     */
//    strncpy(vidPtr->outFormatCtx->filename, fileName,
//        sizeof(vidPtr->outFormatCtx->filename));
//
//    vidPtr->outVideoStr = av_new_stream(vidPtr->outFormatCtx, 0);
//    if (vidPtr->outVideoStr == NULL) {
////        Tcl_AppendResult(interp, "internal error:",
////            " problem opening stream", (char*)NULL);
////        return TCL_ERROR;
//        retunr -1;
//    }
//    codecCtx = vidPtr->outVideoStr->codec;
//
//    codecCtx->codec_id = vidPtr->outFormatCtx->oformat->video_codec;
//    codecCtx->codec_type = CODEC_TYPE_VIDEO;
//
//    /* put sample parameters */
//    codecCtx->bit_rate = 400000;
//    /* resolution must be a multiple of two */
//    codecCtx->width = (iwd/2)*2;
//    codecCtx->height = (iht/2)*2;
//    codecCtx->time_base.den = 24;
//    codecCtx->time_base.num = 1;
//    codecCtx->gop_size = 12; /* emit one intra frame every so often */
//    codecCtx->pix_fmt = PIX_FMT_YUV420P;
//    if (codecCtx->codec_id == CODEC_ID_MPEG2VIDEO) {
//        codecCtx->max_b_frames = 2;
//    }
//
//    /* find the video encoder */
//    vcodec = avcodec_find_encoder(codecCtx->codec_id);
//    if (!vcodec || avcodec_open(codecCtx, vcodec) < 0) {
//        // Tcl_AppendResult(interp, "internal error:",
//        //     " problem opening codec", (char*)NULL);
//        // return TCL_ERROR;
//        return -1;
//    }
//
//    if (av_set_parameters(vidPtr->outFormatCtx, NULL) < 0) {
//        // Tcl_AppendResult(interp, "internal error:",
//        //     " problem in av_set_parameters()", (char*)NULL);
//        // return TCL_ERROR;
//        return -1;
//    }
//
//    if (url_fopen(&vidPtr->outFormatCtx->pb, fileName, URL_WRONLY) < 0) {
//        // Tcl_AppendResult(interp, "can't open file \"", fileName,
//        //     "\"", (char*)NULL);
//        // return TCL_ERROR;
//        return -1;
//    }
//    av_write_header(vidPtr->outFormatCtx);
//
//    vidPtr->pFrameYUV = avcodec_alloc_frame();
//    vidPtr->pFrameRGB = avcodec_alloc_frame();
//    if (vidPtr->pFrameYUV == NULL || vidPtr->pFrameRGB == NULL) {
//        // Tcl_AppendResult(interp, "couldn't allocate frame space",
//        //     " for file \"", fileName, "\"", (char*)NULL);
//        // return TCL_ERROR;
//        return -1;
//    }
//
//    vidPtr->yuvw = vidPtr->outVideoStr->codec->width;
//    vidPtr->yuvh = vidPtr->outVideoStr->codec->height;
//    pixfmt = vidPtr->outVideoStr->codec->pix_fmt;
//
//    numBytes = avpicture_get_size(pixfmt, vidPtr->yuvw, vidPtr->yuvh);
//    vidPtr->yuvbuffer = (uint8_t*)av_malloc(numBytes*sizeof(uint8_t));
//
//    avpicture_fill((AVPicture*)vidPtr->pFrameYUV, vidPtr->yuvbuffer,
//        pixfmt, vidPtr->yuvw, vidPtr->yuvh);
//
//
//    if (strcpy(vid->mode,"output") == NULL) {
//        return -1;
//    }
//
//    return 0;
//}


/*
 * ------------------------------------------------------------------------
 *  VideoTime2Frame()
 *
 *  Converts a time value (as defined by the FFMPEG package) into an
 *  integer frame number in the range 0-end for the stream.
 * ------------------------------------------------------------------------
 */
int
VideoTime2Frame(streamPtr, tval)
    AVStream *streamPtr;   /* scale values according to this stream */
    int64_t tval;          /* time value as defined by stream */
{
    AVRational one, factor;
    one.num = 1;
    one.den = 1;
    factor.num = streamPtr->time_base.num * streamPtr->r_frame_rate.num;
    factor.den = streamPtr->time_base.den * streamPtr->r_frame_rate.den;

    if (tval > streamPtr->start_time) {
        tval -= streamPtr->start_time;
    } else {
        tval = 0;
    }
    tval = av_rescale_q(tval, factor, one);
    return (int)tval;
}

/*
 * ------------------------------------------------------------------------
 *  VideoFrame2Time()
 *
 *  Converts a frame number 0-end to the corresponding time value
 *  (as defined by FFMPEG) for the given stream.
 * ------------------------------------------------------------------------
 */
int64_t
VideoFrame2Time(streamPtr, fval)
    AVStream *streamPtr;   /* scale values according to this stream */
    int fval;              /* frame value in the range 0-end */
{
    int64_t tval;
    AVRational one, factor;
    one.num = 1;
    one.den = 1;

    factor.num = streamPtr->time_base.num * streamPtr->r_frame_rate.num;
    factor.den = streamPtr->time_base.den * streamPtr->r_frame_rate.den;

    tval = av_rescale_q((int64_t)fval, one, factor) + streamPtr->start_time;
    return tval;
}

/*
 * ------------------------------------------------------------------------
 *  VideoNextFrame()
 *
 *  Decodes a series of video packets until the end of the frame
 *  is reached.  Updates the frameNumber and atEnd to maintain the
 *  current status for this video stream.
 * ------------------------------------------------------------------------
 */
void
VideoNextFrame(vidPtr)
    VideoObj *vidPtr;   /* get a frame from this video stream */
{
    int frameFinished;
    uint64_t pts;
    AVCodecContext *vcodecCtx;
    AVStream *vstreamPtr;
    AVPacket packet;

    if (vidPtr->pFormatCtx) {
        vstreamPtr = vidPtr->pFormatCtx->streams[vidPtr->videoStream];
        vcodecCtx = vstreamPtr->codec;

        /*
         * Decode as many packets as necessary to get the next frame.
         */
        pts = 0;
        while (1) {
            if (av_read_frame(vidPtr->pFormatCtx, &packet) >= 0) {
                if (packet.stream_index == vidPtr->videoStream) {
                    /* save pts so we can grab it again in VideoAvGetBuffer */
                    global_video_pkt_pts = packet.pts;

#ifdef HAVE_AVCODEC_DECODE_VIDEO2
                    // new avcodec decode video function
                    avcodec_decode_video2(vcodecCtx, vidPtr->pFrameYUV,
                        &frameFinished, &packet);
#else
                    // old avcodec decode video function
                    avcodec_decode_video(vcodecCtx, vidPtr->pFrameYUV,
                        &frameFinished, packet.data, packet.size);
#endif
                    if (packet.dts == AV_NOPTS_VALUE
                          && vidPtr->pFrameYUV->opaque
                          && *(uint64_t*)vidPtr->pFrameYUV->opaque != AV_NOPTS_VALUE) {
                        pts = *(uint64_t*)vidPtr->pFrameYUV->opaque;
                    } else if (packet.dts != AV_NOPTS_VALUE) {
                        pts = packet.dts;
                    } else {
                        pts = 0;
                    }

                    if (frameFinished) {
                        vidPtr->frameNumber = VideoTime2Frame(vstreamPtr, pts);
                        break;
                    }
                }
            } else {
                vidPtr->atEnd = 1;
                break;
            }
        }
        av_free_packet(&packet);
    }
}

/*
 * ------------------------------------------------------------------------
 *  These two routines are called whenever a frame buffer is allocated,
 *  which means that we're starting a new frame.  Grab the global pts
 *  counter and squirrel it away in the opaque slot of the frame.  This
 *  will give us a pts value that we can trust later.
 * ------------------------------------------------------------------------
 */
int
VideoAvGetBuffer(c,fr)
    AVCodecContext *c;  /* codec doing the frame decoding */
    AVFrame *fr;        /* frame being decoded */
{
    int rval = avcodec_default_get_buffer(c, fr);
    uint64_t *ptsPtr = av_malloc(sizeof(uint64_t));
    *ptsPtr = global_video_pkt_pts;
    fr->opaque = ptsPtr;
    return rval;
}

void
VideoAvReleaseBuffer(c,fr)
    AVCodecContext *c;  /* codec doing the frame decoding */
    AVFrame *fr;        /* frame being decoded */
{
    if (fr && fr->opaque) {
        av_freep(&fr->opaque);
    }
    avcodec_default_release_buffer(c,fr);
}

/*
 * ------------------------------------------------------------------------
 *  VideoInit()
 *
 *  Implements the body of the _ffmpeg_init method in the "video" class.
 *  Initializes the basic data structure and stores it in the _videodata
 *  variable within the class.
 * ------------------------------------------------------------------------
 */
VideoObj *
VideoInit()
{
    /*
     * Create an object to represent this video stream.
     */

    /* Register all codecs and formats */
    av_register_all();

    return VideoSetData();
}

/*
 * ------------------------------------------------------------------------
 *  VideoCleanup()
 *
 *  Implements the body of the _ffmpeg_cleanup method in the "video" class.
 *  Accesses the data structure stored in the _videodata variable and
 *  frees up the data.
 * ------------------------------------------------------------------------
 */
int
VideoCleanup(vidPtr)
    VideoObj *vidPtr;
{
    /*
     *  Nothing much to do here.  Just close the file in case it is
     *  still open.  Don't free vidPtr itself; that is cleaned up by
     *  the ByteArrayObj in the class data member.
     */
    int ret = 0;

    ret -= VideoClose(vidPtr);

    if (vidPtr != NULL) {
        VideoFreeImgBuffer(vidPtr);
        if (vidPtr->fileName != NULL) {
            free(vidPtr->fileName);
            vidPtr->fileName = NULL;
        }
        free(vidPtr);
        vidPtr = NULL;
// FIXME: need a test to make sure vidPtr is null after the function returns.
    }

    return ret;
}

/*
 * ------------------------------------------------------------------------
 *  VideoSize()
 *
 *  Implements the body of the "size" method in the "video" class.
 *  Returns the size of each frame in this video stream as a list {w h}.
 * ------------------------------------------------------------------------
 */
int
VideoSize(vidPtr, width, height)
    VideoObj *vidPtr;
    int *width;
    int *height;
{
    AVCodecContext *vcodecCtx;

    if (vidPtr == NULL) {
        return -1;
    }

    if (vidPtr->pFormatCtx == NULL) {
        // "internal error: video stream is not open",
        return -1;
    }

    if (vidPtr->pFormatCtx) {
        vcodecCtx = vidPtr->pFormatCtx->streams[vidPtr->videoStream]->codec;
        if (width != NULL) {
            *width = vcodecCtx->width;
        }
        if (height != NULL) {
            *height = vcodecCtx->height;
        }
    }
    return 0;
}

/*
 * ------------------------------------------------------------------------
 *  VideoGo()
 *
 *  Implements the body of the "go" method in the "video" class.
 *  Advances by one or more frames, or seeks backward in the stream.
 *  Handles the following syntax:
 *    obj go next ...... go to next frame (same as +1)
 *    obj go +n ........ advance by n frames
 *    obj go -n ........ go back by n frames
 *    obj go n ......... go to frame n
 * ------------------------------------------------------------------------
 */
int
VideoGoNext(vidPtr)
    VideoObj *vidPtr;
{
    int nabs;

    if (vidPtr == NULL) {
        return -1;
    }

    nabs = vidPtr->frameNumber + 1;
    return VideoGoToN(vidPtr, nabs);
}

int
VideoGoPlusMinusN(vidPtr, n)
    VideoObj *vidPtr;
    int n;
{
    int nabs;

    if (vidPtr == NULL) {
        return -1;
    }

    nabs = vidPtr->frameNumber + n;
    return VideoGoToN(vidPtr, nabs);
}

int
VideoGoToN(vidPtr, n)
    VideoObj *vidPtr;
    int n;
{
    int nrel, nabs, seekFlags, gotframe;
    int64_t nseek;
    AVCodecContext *vcodecCtx;
    AVStream *vstreamPtr;

    if (vidPtr == NULL) {
        return -1;
    }

    if (vidPtr->pFormatCtx == NULL) {
        // "internal error: video stream is not open",
        return -1;
    }
    vcodecCtx = vidPtr->pFormatCtx->streams[vidPtr->videoStream]->codec;

    nabs = n;

    if (nabs < 0) {
        nabs = 0;
    }

    if (nabs < vidPtr->frameNumber) {
        seekFlags = AVSEEK_FLAG_BACKWARD;
    } else {
        seekFlags = 0;
    }

    /*
     * If we're going to an absolute frame, or if we're going backward
     * or too far forward, then seek the frame.
     */
    nrel = nabs-vidPtr->frameNumber;
    if ((nrel > 50) || (seekFlags&AVSEEK_FLAG_BACKWARD)) {

        vstreamPtr = vidPtr->pFormatCtx->streams[vidPtr->videoStream];
        nseek = VideoFrame2Time(vstreamPtr, nabs);
        // not sure why it is checking against the number 100
        if (nseek > 100) {
            nseek -= 100;
        } else {
            nseek = 0;
        }

        /* first, seek the nearest reference frame for a good starting pt */
        av_seek_frame(vidPtr->pFormatCtx, vidPtr->videoStream,
            nseek, seekFlags);

        // this doesn't seem to give me back the true frame number
        // feels like it is more of a reverse of the VideoFrame2Time call
        // because vidPtr->frameNumber always equals nabs
        vidPtr->frameNumber = VideoTime2Frame(vstreamPtr, nseek);
        vidPtr->atEnd = 0;

        /* read the frame to figure out what the frame number is */
        VideoNextFrame(vidPtr);

        /* then, move forward until we reach the desired frame */
        gotframe = 0;
        while (vidPtr->frameNumber < nabs && !vidPtr->atEnd) {
            VideoNextFrame(vidPtr);
            gotframe = 1;
        }

        /* get at least one frame, unless we're done or at the beginning*/
        if (!gotframe && !vidPtr->atEnd) {
            if (vidPtr->frameNumber > nabs) {
                // we are probably at a key frame, just past
                // the requested frame and need to seek backwards.
                VideoGoToN(vidPtr,n);
            } else {
                VideoNextFrame(vidPtr);
            }
        }
    }
    else {
        while (nrel-- > 0) {
            VideoNextFrame(vidPtr);
        }
    }

    /*
     * Send back the current frame number or "end" as the result.
     */
    return vidPtr->frameNumber;
}

/*
 * ------------------------------------------------------------------------
 *  VideoGet()
 *
 *  Implements the body of the "get" method in the "video" class.
 *  Returns information about the current frame via the following
 *  syntax:
 *    obj get start|position|end
 *    obj get <imageHandle>
 * ------------------------------------------------------------------------
 */
int
VideoGetImage(vidPtr, iw, ih, img, bufSize)
    VideoObj *vidPtr;
    int iw;
    int ih;
    void **img;
    int *bufSize;
{

    int numBytes;
    AVCodecContext *vcodecCtx;

    if (vidPtr == NULL) {
        return -1;
    }

    if (VideoModeRead(vidPtr) != 0) {
        return -1;
    }

    /*
    if (vidPtr->pFormatCtx) {
        vcodecCtx = vidPtr->pFormatCtx->streams[vidPtr->videoStream]->codec;
    } else {
        vcodecCtx = NULL;
    }
    */

    if (vidPtr->pFormatCtx == NULL) {
        // vidPtr->pFormatCtx is NULL, video not open
        return -1;
    }
    vcodecCtx = vidPtr->pFormatCtx->streams[vidPtr->videoStream]->codec;

    /*
     * Query the size for this photo and make sure that we have a
     * buffer of the appropriate size for software scaling and
     * format conversion.
     */

    // if the user's desired size is less then 0,
    // use the default size

    if (iw < 0) {
        iw = vcodecCtx->width;
    }
    if (ih < 0) {
        ih = vcodecCtx->height;
    }


    if (iw != vidPtr->rgbw || ih != vidPtr->rgbh) {
        if (vidPtr->rgbbuffer) {
            av_free(vidPtr->rgbbuffer);
            vidPtr->rgbbuffer = NULL;
        }
        numBytes = avpicture_get_size(PIX_FMT_RGB24, iw, ih);
        vidPtr->rgbbuffer = (uint8_t*)av_malloc(numBytes*sizeof(uint8_t));
        vidPtr->rgbw = iw;
        vidPtr->rgbh = ih;

        avpicture_fill((AVPicture*)vidPtr->pFrameRGB, vidPtr->rgbbuffer,
            PIX_FMT_RGB24, iw, ih);

        vidPtr->scalingCtx = sws_getCachedContext(vidPtr->scalingCtx,
            vcodecCtx->width, vcodecCtx->height, vcodecCtx->pix_fmt,
            iw, ih, PIX_FMT_RGB24, SWS_BICUBIC|SWS_PRINT_INFO, NULL, NULL, NULL);
    }

    /*
     * Rescale the current frame to the desired size, and translate
     * into RGB format so we can copy into the destination image.
     */
    if (vidPtr->pFrameYUV && vidPtr->pFrameYUV->data[0]) {
        sws_scale(vidPtr->scalingCtx, (const uint8_t * const*)
            vidPtr->pFrameYUV->data, vidPtr->pFrameYUV->linesize,
            0, vcodecCtx->height,
            vidPtr->pFrameRGB->data, vidPtr->pFrameRGB->linesize);

/*
        iblock.pixelPtr  = (unsigned char*)vidPtr->pFrameRGB->data[0];
        iblock.width     = iw;
        iblock.height    = ih;
        iblock.pitch     = vidPtr->pFrameRGB->linesize[0];
        iblock.pixelSize = 3;
        iblock.offset[0] = 0;
        iblock.offset[1] = 1;
        iblock.offset[2] = 2;
        iblock.offset[3] = 0;

        Tk_PhotoPutBlock_NoComposite(img, &iblock, 0, 0, iw, ih);
*/

        if (vidPtr->img == NULL) {
            VideoAllocImgBuffer(vidPtr,iw,ih);
        } else {
            if ((vidPtr->imgWidth != iw) && (vidPtr->imgHeight != ih)) {
                // new height or width
                // resize the image buffer
                free(vidPtr->img);
                VideoAllocImgBuffer(vidPtr,iw,ih);
            }
        }

        // Write pixel data
        memcpy(vidPtr->img+vidPtr->imgHeaderLen,
            vidPtr->pFrameRGB->data[0],
            vidPtr->imgWidth*3*vidPtr->imgHeight);
    }
    *img = vidPtr->img;
    *bufSize = (vidPtr->imgWidth*3*vidPtr->imgHeight) + vidPtr->imgHeaderLen;
    return 0;
}

int
VideoFrameRate (vidPtr, fr)
    VideoObj *vidPtr;
    double *fr;
{
    AVStream *vstreamPtr;

    if (vidPtr == NULL) {
        return -1;
    }

    if (fr == NULL) {
        return -1;
    }

    if (vidPtr->pFormatCtx == NULL) {
        // vidPtr->pFormatCtx is NULL, video not open
        return -1;
    }
    vstreamPtr = vidPtr->pFormatCtx->streams[vidPtr->videoStream];

    // http://trac.handbrake.fr/browser/trunk/libhb/decavcodec.c?rev=1490#L684
    // there seems to be some controversy over what structure holds
    // the correct frame rate information for different video codecs.
    // for now we will use the stream's r_frame_rate.
    // from the above post, it looks like this value can be interpreted
    // as frames per second.
    *fr = av_q2d(vstreamPtr->r_frame_rate);

    return 0;
}

int
VideoFileName (vidPtr, fname)
    VideoObj *vidPtr;
    const char **fname;
{
    if (vidPtr == NULL) {
        return -1;
    }

    if (fname == NULL) {
        return -1;
    }

    if (vidPtr->pFormatCtx == NULL) {
        // vidPtr->pFormatCtx is NULL, video not open
        return -1;
    }

    *fname = vidPtr->fileName;

    return 0;
}

int
VideoPixelAspectRatio (vidPtr, num, den)
    VideoObj *vidPtr;
    int *num;
    int *den;
{
    AVCodecContext *vcodecCtx;

    if (vidPtr == NULL) {
        return -1;
    }

    if ((num == NULL) || (den == NULL)) {
        return -1;
    }

    if (vidPtr->pFormatCtx == NULL) {
        // vidPtr->pFormatCtx is NULL, video not open
        return -1;
    }

    vcodecCtx = vidPtr->pFormatCtx->streams[vidPtr->videoStream]->codec;

    *num = vcodecCtx->sample_aspect_ratio.num;
    *den = vcodecCtx->sample_aspect_ratio.den;

    return 0;
}

int
VideoDisplayAspectRatio (vidPtr, num, den)
    VideoObj *vidPtr;
    int *num;
    int *den;
{
    int width = 0;
    int height = 0;
    int64_t gcd = 0;

    if (vidPtr == NULL) {
        return -1;
    }

    if ((num == NULL) || (den == NULL)) {
        return -1;
    }

    if (vidPtr->pFormatCtx == NULL) {
        // vidPtr->pFormatCtx is NULL, video not open
        return -1;
    }

    VideoSize(vidPtr, &width, &height);
    VideoPixelAspectRatio(vidPtr, num, den);

    width = (*num)*width;
    height = (*den)*height;
#ifdef FFMPEG_COMMON_H
    // old gcd function
    gcd = ff_gcd(FFABS(width), FFABS(height));
#else
    // new gcd function
    gcd = av_gcd(FFABS(width), FFABS(height));
#endif


    *num = width/gcd;
    *den = height/gcd;

    if (*den == 0) {
        *num = 0;
        *den = 1;
    }

    return 0;
}

int
VideoAllocImgBuffer(vidPtr, width, height)
    VideoObj *vidPtr;
    int width;
    int height;
{

    char header[64];
    int headerLen = 0;
    int bufsize = 0;

    sprintf(header,"P6\n%d %d\n255\n", width, height);
    headerLen = strlen(header);
    bufsize = headerLen + (width*3*height);
    vidPtr->img = (void*) malloc(bufsize);
    vidPtr->imgHeaderLen = headerLen;
    vidPtr->imgWidth = width;
    vidPtr->imgHeight = height;
    memcpy(vidPtr->img,header,headerLen);

    return 0;
}

int
VideoFreeImgBuffer(vidPtr)
    VideoObj *vidPtr;
{
    if ((vidPtr != NULL) && (vidPtr->img != NULL)) {
        free(vidPtr->img);
        vidPtr->img = NULL;
    }
    return 0;
}

int
VideoGetPositionCur(vidPtr, pos)
    VideoObj *vidPtr;      /* video object to act on */
    int *pos;
{
    int fnum = -1;

    if (vidPtr == NULL) {
        return -1;
    }

    if (pos == NULL) {
        return -1;
    }

    if (VideoModeRead(vidPtr) != 0) {
        return -1;
    }

    if (vidPtr->pFormatCtx) {
        fnum = vidPtr->frameNumber;
    }

    *pos = fnum;
    return 0;
}

int
VideoGetPositionEnd(vidPtr, pos)
    VideoObj *vidPtr;      /* video object to act on */
    int *pos;
{
    if (vidPtr == NULL) {
        return -1;
    }

    if (pos == NULL) {
        return -1;
    }

    if (VideoModeRead(vidPtr) != 0) {
        return -1;
    }

    *pos = vidPtr->lastframe;
    return 0;
}

// FIXME: get this function working
///*
// * ------------------------------------------------------------------------
// *  VideoPut()
// *
// *  Implements the body of the "put" method in the "video" class.
// *  Stores a single frame into the video stream:
// *    obj put <imageHandle>
// * ------------------------------------------------------------------------
// */
//int
//VideoPut(cdata, interp, argc, argv)
//    ClientData cdata;      /* not used */
//    Tcl_Interp *interp;    /* interpreter */
//    int argc;              /* number of arguments */
//    CONST84 char* argv[];  /* argument strings */
//{
//    VideoObj *vidPtr;
//    int iw, ih, numBytes, roffs, goffs, boffs;
//    char buffer[64];
//    unsigned char* photodata;
//    uint8_t* rgbdata;
//    Tk_PhotoHandle img;
//    Tk_PhotoImageBlock iblock;
//    AVCodecContext *codecCtx;
//
//    if (VideoGetData(interp, &vidPtr) != TCL_OK) {
//        return TCL_ERROR;
//    }
//
//    if (argc != 2) {
//        Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
//            " image\"", (char*)NULL);
//        return TCL_ERROR;
//    }
//
//    /*
//     * Get the name of the image and copy from it.
//     */
//    img = Tk_FindPhoto(interp, argv[1]);
//    if (img == NULL) {
//        Tcl_AppendResult(interp, "bad value \"", argv[1],
//            "\": expected photo image", (char*)NULL);
//        return TCL_ERROR;
//    }
//
//    /*
//     * Query the size for this photo and make sure that we have a
//     * buffer of the appropriate size for software scaling and
//     * format conversion.
//     */
//    Tk_PhotoGetImage(img, &iblock);
//    Tk_PhotoGetSize(img, &iw, &ih);
//
//    if (VideoModeWrite(interp, iw, ih) != TCL_OK) {
//        return TCL_ERROR;
//    }
//    codecCtx = vidPtr->outVideoStr->codec;
//
//    if (iw != vidPtr->rgbw || ih != vidPtr->rgbh) {
//        if (vidPtr->rgbbuffer) {
//            av_free(vidPtr->rgbbuffer);
//            vidPtr->rgbbuffer = NULL;
//        }
//        numBytes = avpicture_get_size(PIX_FMT_RGB24, iw, ih);
//        vidPtr->rgbbuffer = (uint8_t*)av_malloc(numBytes*sizeof(uint8_t));
//        vidPtr->rgbw = iw;
//        vidPtr->rgbh = ih;
//
//        avpicture_fill((AVPicture*)vidPtr->pFrameRGB, vidPtr->rgbbuffer,
//            PIX_FMT_RGB24, iw, ih);
//
//        vidPtr->scalingCtx = sws_getCachedContext(vidPtr->scalingCtx,
//            iw, ih, PIX_FMT_RGB24,
//            codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
//            SWS_BICUBIC, NULL, NULL, NULL);
//    }
//
//    /*
//     * Copy the data from the Tk photo block into the RGB frame.
//     */
//    roffs = iblock.offset[0];
//    goffs = iblock.offset[1];
//    boffs = iblock.offset[2];
//
//    for (ih=0; ih < iblock.height; ih++) {
//        rgbdata = vidPtr->pFrameRGB->data[0] + ih*vidPtr->pFrameRGB->linesize[0];
//        photodata = iblock.pixelPtr + ih*iblock.pitch;
//        for (iw=0; iw < iblock.width; iw++) {
//            rgbdata[0] = photodata[roffs];
//            rgbdata[1] = photodata[goffs];
//            rgbdata[2] = photodata[boffs];
//            rgbdata += 3;
//            photodata += iblock.pixelSize;
//        }
//    }
//
//    /*
//     * Rescale the current frame to the desired size, and translate
//     * from RGB to YUV so we can give the frame to the codec.
//     */
//    sws_scale(vidPtr->scalingCtx,
//        vidPtr->pFrameRGB->data, vidPtr->pFrameRGB->linesize,
//        0, ih,
//        vidPtr->pFrameYUV->data, vidPtr->pFrameYUV->linesize);
//
//    numBytes = VideoWriteFrame(vidPtr, vidPtr->pFrameYUV);
//    if (numBytes < 0) {
//        Tcl_AppendResult(interp, "error in av_write_frame()", (char*)NULL);
//        return TCL_ERROR;
//    }
//    sprintf(buffer, "frame %d (%d bytes)", vidPtr->frameNumber++, numBytes);
//    Tcl_SetResult(interp, buffer, TCL_VOLATILE);
//    return TCL_OK;
//}


/*
 * ------------------------------------------------------------------------
 *  VideoWriteFrame()
 *
 *  Used internally to write a single frame out to the output stream.
 *  Returns the number of bytes written to the frame, or -1 if an error
 *  occurred.
 * ------------------------------------------------------------------------
 */
int
VideoWriteFrame(vidPtr, framePtr)
    VideoObj *vidPtr;      /* video object being updated */
    AVFrame *framePtr;     /* picture frame being written out */
{
    int numBytes;
    AVCodecContext *codecCtx;
    AVPacket pkt;

#define OUTBUF_SIZE 500000
    uint8_t outbuf[OUTBUF_SIZE];

    codecCtx = vidPtr->outVideoStr->codec;
    numBytes = avcodec_encode_video(codecCtx, outbuf, OUTBUF_SIZE, framePtr);

    if (numBytes > 0) {
        av_init_packet(&pkt);

        if (codecCtx->coded_frame->pts != AV_NOPTS_VALUE) {
            pkt.pts = av_rescale_q(codecCtx->coded_frame->pts,
                codecCtx->time_base,
                vidPtr->outVideoStr->time_base);
        }
        if (codecCtx->coded_frame->key_frame) {
            pkt.flags |= AV_PKT_FLAG_KEY;
        }
        pkt.stream_index = vidPtr->outVideoStr->index;
        pkt.data = outbuf;
        pkt.size = numBytes;

        /* write the compressed frame in the media file */
        if (av_write_frame(vidPtr->outFormatCtx, &pkt) != 0) {
            return -1;
        }
    }
    return numBytes;
}

/*
 * ------------------------------------------------------------------------
 *  VideoTransform()
 *
 *  Implements the body of the "transform" method in the "video" class.
 *  Translates one value into another--times into frames, etc.  Handles
 *  the following syntax:
 *    obj transform frames2duration <frames>
 *    obj transform duration2frames <duration>
 * ------------------------------------------------------------------------
 */
double
VideoTransformFrames2Duration(vidPtr, frame)
    VideoObj *vidPtr;
    int frame;
{
    double duration;
    AVCodecContext *vcodecCtx;
    AVStream *vstreamPtr;
    AVRational hundred;
    int64_t tval;

    hundred.num = 100;
    hundred.den = 1;

    if (vidPtr == NULL) {
        return -1;
    }

    if (vidPtr->pFormatCtx == NULL) {
//        Tcl_AppendResult(interp, "can't compute transformations:",
//            " stream not opened", (char*)NULL);
//        return TCL_ERROR;
        return -1;
    }

    vcodecCtx = vidPtr->pFormatCtx->streams[vidPtr->videoStream]->codec;
    vstreamPtr = vidPtr->pFormatCtx->streams[vidPtr->videoStream];

    tval = av_rescale_q((int64_t)frame, hundred, vstreamPtr->r_frame_rate);
    duration = 0.01*tval;

    return duration;
}

int
VideoTransformDuration2Frames(vidPtr, duration)
    VideoObj *vidPtr;
    double duration;
{
    int frames;
    AVCodecContext *vcodecCtx;
    AVStream *vstreamPtr;
    AVRational hundred;
    int64_t tval;

    hundred.num = 100;
    hundred.den = 1;

    if (vidPtr == NULL) {
        return -1;
    }
    if (vidPtr->pFormatCtx == NULL) {
//        Tcl_AppendResult(interp, "can't compute transformations:",
//            " stream not opened", (char*)NULL);
//        return TCL_ERROR;
        return -1;
    }

    vcodecCtx = vidPtr->pFormatCtx->streams[vidPtr->videoStream]->codec;
    vstreamPtr = vidPtr->pFormatCtx->streams[vidPtr->videoStream];

    tval = (int64_t)(duration*100);
    frames = av_rescale_q(tval, vstreamPtr->r_frame_rate, hundred);
    // check above for overflow
    // tval = av_rescale_q(tval, vstreamPtr->r_frame_rate, hundred);
    // sprintf(buffer, "%lld", tval);

    return frames;
}

/*
 * ------------------------------------------------------------------------
 *  VideoClose()
 *
 *  Implements the body of the _ffmpeg_close method in the "video" class.
 *  Closes any file opened previously by the open methods for read/write.
 *  If nothing is open, this does nothing.
 * ------------------------------------------------------------------------
 */
int
VideoClose(vidPtr)
    VideoObj *vidPtr;
{
    AVCodecContext *vcodecCtx;
    int i;

    if (vidPtr == NULL) {
        return -1;
    }

    if (vidPtr->yuvbuffer) {
        av_free(vidPtr->yuvbuffer);
        vidPtr->yuvbuffer = NULL;
        vidPtr->yuvw = 0;
        vidPtr->yuvh = 0;
    }
    if (vidPtr->pFrameYUV) {
        av_free(vidPtr->pFrameYUV);
        vidPtr->pFrameYUV = NULL;
    }

    if (vidPtr->rgbbuffer) {
        av_free(vidPtr->rgbbuffer);
        vidPtr->rgbbuffer = NULL;
        vidPtr->rgbw = 0;
        vidPtr->rgbh = 0;
    }
    if (vidPtr->pFrameRGB) {
        av_free(vidPtr->pFrameRGB);
        vidPtr->pFrameRGB = NULL;
    }

    if (vidPtr->scalingCtx) {
        sws_freeContext(vidPtr->scalingCtx);
        vidPtr->scalingCtx = NULL;
    }
    if (vidPtr->pFormatCtx && vidPtr->videoStream >= 0) {
        vcodecCtx = vidPtr->pFormatCtx->streams[vidPtr->videoStream]->codec;
        if (vcodecCtx) {
            avcodec_close(vcodecCtx);
        }
    }
    if (vidPtr->pFormatCtx) {
#ifdef HAVE_AVFORMAT_CLOSE_INPUT
        avformat_close_input(&vidPtr->pFormatCtx);
#else
        av_close_input_file(vidPtr->pFormatCtx);
#endif
        vidPtr->pFormatCtx = NULL;
    }

    if (vidPtr->outFormatCtx) {
        while (VideoWriteFrame(vidPtr, NULL) > 0)
            ; /* write out any remaining frames */

        av_write_trailer(vidPtr->outFormatCtx);

        for (i=0; i < vidPtr->outFormatCtx->nb_streams; i++) {
            avcodec_close(vidPtr->outFormatCtx->streams[i]->codec);
            av_freep(&vidPtr->outFormatCtx->streams[i]->codec);
            av_freep(&vidPtr->outFormatCtx->streams[i]);
        }

        if (vidPtr->outFormatCtx->pb) {
            avio_close(vidPtr->outFormatCtx->pb);
        }

        av_free(vidPtr->outFormatCtx);
        vidPtr->outFormatCtx = NULL;
    }

    /* reset the mode to null */
    *vidPtr->mode = '\0';

    return 0;
}
