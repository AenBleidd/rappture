/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ======================================================================
 *  Rappture::AVTranslate
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2013  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */


#include "nvconf.h"

#if defined(HAVE_LIBAVCODEC) || defined(HAVE_LIBAVFORMAT)
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <assert.h>

extern "C" {
#ifdef HAVE_FFMPEG_MEM_H
#include <ffmpeg/mem.h>
#endif
#ifdef HAVE_LIBAVUTIL_MEM_H
#include <libavutil/mem.h>
#endif
}

#include "RpAVTranslate.h"

#ifndef HAVE_AVMEDIA_TYPE_VIDEO
#define AVMEDIA_TYPE_VIDEO	CODEC_TYPE_VIDEO
#endif	/* HAVE_AVMEDIA_TYPE_VIDEO */

#ifndef AV_PKT_FLAG_KEY
#define AV_PKT_FLAG_KEY		PKT_FLAG_KEY		
#endif

#ifndef HAVE_AV_GUESS_FORMAT
#define av_guess_format		guess_format
#endif	/*HAVE_AV_GUESS_FORMAT*/

#ifndef HAVE_AV_DUMP_FORMAT
#define av_dump_format		dump_format
#endif	/*HAVE_AV_DUMP_FORMAT*/

#ifndef HAVE_AVIO_OPEN
#define avio_open		url_fopen	
#endif	/*HAVE_AVIO_OPEN*/

#ifndef HAVE_AVIO_CLOSE
#define avio_close		url_fclose	
#endif	/*HAVE_AVIO_CLOSE*/

#ifndef AVIO_FLAG_WRITE
#define AVIO_FLAG_WRITE         URL_WRONLY
#endif

#ifndef HAVE_AVFORMAT_NEW_STREAM
#define avformat_new_stream     av_new_stream
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace Rappture;

AVTranslate::AVTranslate(size_t width, size_t height) : 
    _width (width),
    _height (height),
    _bitRate(400000),
    _frameRate(25.0f),
    _videoOutbufSize(200000),
    _videoOutbuf(NULL),
    _fmtPtr(NULL),
    _ocPtr(NULL),
    _avStreamPtr(NULL),
    _pictPtr(NULL),
    _rgbPictPtr(NULL)
{
}

AVTranslate::AVTranslate(size_t width, size_t height, size_t bitRate,
			 float frameRate) :
    _width (width),
    _height (height),
    _bitRate(bitRate),
    _frameRate(frameRate),
    _videoOutbufSize(200000),
    _videoOutbuf(NULL),
    _fmtPtr(NULL),
    _ocPtr(NULL),
    _avStreamPtr(NULL),
    _pictPtr(NULL),
    _rgbPictPtr(NULL)
{
}

/**
 * Copy constructor
 * @param AVTranslate object to copy
 */
 /*
AVTranslate::AVTranslate(const AVTranslate& o)
{}
*/

AVTranslate::~AVTranslate()
{
#ifdef notdef
    /* FIXME:: This can't be right.  Don't want to automatically write out
     * trailer etc. on destruction of this object. */
    done();
#endif
}


bool
AVTranslate::init(Outcome &status, const char *filename)
{
    status.addContext("Rappture::AVTranslate::init()");
    /* Initialize libavcodec, and register all codecs and formats */
#ifdef HAVE_AVCODEC_REGISTER_ALL
    avcodec_register_all();
#else
    avcodec_init();
#endif
    av_register_all();

    /* Auto detect the output format from the name. default is mpeg. */
    _fmtPtr = av_guess_format(NULL, filename, NULL);
    if (_fmtPtr == NULL) {
        /*
	  TRACE(  "Could not deduce output format from"
                 "file extension: using MPEG.\n");
        */
        _fmtPtr = av_guess_format("mpeg", NULL, NULL);
    }
    if (_fmtPtr == NULL) {
        status.addError("Could not find suitable output format");
        return false;
    }

#ifdef HAVE_AVFORMAT_ALLOC_CONTEXT
    /* Allocate the output media context. */
    _ocPtr = avformat_alloc_context();
#else 
    _ocPtr = av_alloc_format_context();
#endif
    if (!_ocPtr) {
        status.addError("Memory error while allocating format context");
        return false;
    }
    _ocPtr->oformat = _fmtPtr;
    snprintf(_ocPtr->filename, sizeof(_ocPtr->filename), "%s", filename);

    /* Add the video stream using the default format codecs and initialize the
       codecs. */
    _avStreamPtr = NULL;
    if (_fmtPtr->video_codec != CODEC_ID_NONE) {
        if ( (!addVideoStream(status, _fmtPtr->video_codec, &_avStreamPtr)) ) {
            return false;
        }
    }

#if defined(HAVE_AV_SET_PARAMETERS) && !defined(HAVE_AVFORMAT_WRITE_HEADER)
    /* Set the output parameters (must be done even if no parameters). */
    if (av_set_parameters(_ocPtr, NULL) < 0) {
        status.addError("Invalid output format parameters");
        return false;
    }
#endif
    av_dump_format(_ocPtr, 0, filename, 1);

    /* Now that all the parameters are set, we can open the video codec and
       allocate the necessary encode buffers */
    if (_avStreamPtr) {
        if (!openVideo(status)) {
            return false;
        }
    }

    /* Open the output file, if needed. */
    if (!(_fmtPtr->flags & AVFMT_NOFILE)) {
        if (avio_open(&_ocPtr->pb, filename, AVIO_FLAG_WRITE) < 0) {
            status.addError("Could not open '%s'", filename);
            return false;
        }
    }

    /* write the stream header, if any */
#ifdef HAVE_AVFORMAT_WRITE_HEADER
    avformat_write_header(_ocPtr, NULL);
#else
    av_write_header(_ocPtr);
#endif
    return true;
}

bool
AVTranslate::append(Outcome &status, uint8_t *rgbData, size_t linePad)
{
    status.addContext("Rappture::AVTranslate::append()");
    if (rgbData == NULL) {
        status.addError("rdbData pointer is NULL");
        return false;
    }
    /* Copy the data into the picture without the padding and reversing the
     * rows. Note that the origin of the GL image is the lower-left while for
     * the movie it's upper-left. */
    size_t bytesPerRow = _width * 3;
    size_t bytesPerLine = bytesPerRow + linePad;
    uint8_t *srcRowPtr = rgbData + ((_height - 1) * bytesPerLine);
    uint8_t *destPtr = _rgbPictPtr->data[0];
    for (size_t y = 0; y < _height; y++) {
	uint8_t *sp, *send;
	
        for (sp = srcRowPtr, send = sp + bytesPerRow; sp < send; sp++, destPtr++) {
            *destPtr = *sp;
        }
        srcRowPtr -= bytesPerLine;
    }

#ifdef HAVE_IMG_CONVERT
    // Use img_convert instead of sws_scale because img_convert is LGPL and
    // sws_scale is GPL
    img_convert((AVPicture *)_pictPtr, PIX_FMT_YUV420P,
                (AVPicture *)_rgbPictPtr, PIX_FMT_RGB24,
                _width, _height);
#endif	/*HAVE_IMG_CONVERT*/
    writeVideoFrame(status);

    return true;
}


bool
AVTranslate::done(Outcome &status)
{
    size_t i = 0;

    /* Close each codec */
    if (_avStreamPtr) {
        closeVideo(status);
    }

    /* Write the trailer, if any */
    av_write_trailer(_ocPtr);

    /* Free the streams */
    for(i = 0; i < _ocPtr->nb_streams; i++) {
        av_freep(&_ocPtr->streams[i]->codec);
        // _ocPtr->streams[i]->codec = NULL;

        av_freep(&_ocPtr->streams[i]);
        // _ocPtr->streams[i] = NULL;
    }

    if (!(_fmtPtr->flags & AVFMT_NOFILE)) {
        /* close the output file */
        avio_close(_ocPtr->pb);
    }

    /* Free the stream */
    av_free(_ocPtr);
    _ocPtr = NULL;
    return true;
}


/* Add a video output stream */
bool
AVTranslate::addVideoStream(Outcome &status, CodecID codec_id, 
			    AVStream **streamPtrPtr)
{
    status.addContext("Rappture::AVTranslate::add_video_stream()");
    if (streamPtrPtr == NULL) {
        status.addError("AVStream **st is NULL");
        return false;
    }

    AVStream *streamPtr;
    streamPtr = avformat_new_stream(_ocPtr, 0);
    if (streamPtr == NULL) {
        status.addError("Could not alloc stream");
        return false;
    }

    AVCodecContext *codecPtr;
    codecPtr = streamPtr->codec;
    codecPtr->codec_id = codec_id;
    codecPtr->codec_type = AVMEDIA_TYPE_VIDEO;

    /* Put sample parameters */
    codecPtr->bit_rate = _bitRate;
    /* resolution must be a multiple of two */
    codecPtr->width = _width;
    codecPtr->height = _height;
    /* time base: this is the fundamental unit of time (in seconds) in terms
       of which frame timestamps are represented. for fixed-fps content,
       timebase should be 1/framerate and timestamp increments should be
       identically 1. */
    codecPtr->time_base.den = _frameRate;
    codecPtr->time_base.num = 1;
    codecPtr->gop_size = 12;		/* Emit one intra frame every twelve
					 * frames at most */
    codecPtr->pix_fmt = PIX_FMT_YUV420P;
    if (codecPtr->codec_id == CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B frames */
        codecPtr->max_b_frames = 2;
    }
    if (codecPtr->codec_id == CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
           This does not happen with normal video, it just happens here as the
           motion of the chroma plane does not match the luma plane. */
        codecPtr->mb_decision=2;
    }
    /* some formats want stream headers to be separate */
    if((strcmp(_ocPtr->oformat->name, "mp4") == 0) || 
       (strcmp(_ocPtr->oformat->name, "mov") == 0) ||
       (strcmp(_ocPtr->oformat->name, "3gp") == 0)) {
        codecPtr->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    *streamPtrPtr = streamPtr;
    return true;
}

bool
AVTranslate::allocPicture(Outcome &status, PixelFormat pixFmt, 
     AVFrame **framePtrPtr)
{
    status.addContext("Rappture::AVTranslate::allocPicture()");
    if (framePtrPtr == NULL) {
        status.addError("AVFrame **p == NULL");
        return false;
    }

    AVFrame *framePtr;
    framePtr = avcodec_alloc_frame();
    if (framePtr == NULL) {
        status.addError("Memory error: Could not alloc frame");
        return false;
    }

    size_t size;
    size = avpicture_get_size(pixFmt, _width, _height);

    uint8_t *bits;
    bits = (uint8_t *)av_malloc(size);
    if (bits == NULL) {
        av_free(framePtr);
        status.addError("Memory error: Could not alloc picture buffer");
        return false;
    }
    avpicture_fill((AVPicture *)framePtr, bits, pixFmt, _width, _height);
    *framePtrPtr = framePtr;
    return true;
}

bool
AVTranslate::openVideo(Outcome &status)
{
    AVCodec *codec;
    AVCodecContext *c;
    int result;

    status.addContext("Rappture::AVTranslate::openVideo()");
    c = _avStreamPtr->codec;

    /* find the video encoder */
    codec = avcodec_find_encoder(c->codec_id);
    if (codec == NULL) {
        status.addError("can't find codec %d\n", c->codec->id);
        return false;
    }
#ifdef HAVE_AVCODEC_OPEN2
    result = avcodec_open2(c, codec, NULL);
#else 
    result = avcodec_open(c, codec);
#endif
    /* open the codec */
    if (result < 0) {
        status.addError("can't open codec %d", c->codec->id);
        return false;
    }

    _videoOutbuf = NULL;
    if (!(_ocPtr->oformat->flags & AVFMT_RAWPICTURE)) {
        /* allocate output buffer */
        /* XXX: API change will be done */
        /* buffers passed into lav* can be allocated any way you prefer,
           as long as they're aligned enough for the architecture, and
           they're freed appropriately (such as using av_free for buffers
           allocated with av_malloc) */
        _videoOutbuf = (uint8_t *) av_malloc(_videoOutbufSize);
    }
    /* Allocate the encoded raw picture */
    if (!allocPicture(status, c->pix_fmt, &_pictPtr)) {
        return false;
    }
    if (!allocPicture(status, PIX_FMT_RGB24, &_rgbPictPtr)) {
        status.addError("allocPicture: can't allocate picture");
        return false;
    }
    return true;
}

bool
AVTranslate::writeVideoFrame(Outcome &status)
{
    AVCodecContext *contextPtr;

    status.addContext("Rappture::AVTranslate::writeVideoframe()");
    contextPtr = _avStreamPtr->codec;

    /* encode the image */
    int size;
    size = avcodec_encode_video(contextPtr, _videoOutbuf, _videoOutbufSize, 
	_pictPtr);
    if (size < 0) {
        status.addError("Error while writing video frame");
	return false;
    }
    if (size == 0) {
	return true;		/* Image was buffered */
    }
    AVPacket pkt;
    av_init_packet(&pkt);

    pkt.pts = av_rescale_q(contextPtr->coded_frame->pts, contextPtr->time_base,
			   _avStreamPtr->time_base);
    if (contextPtr->coded_frame->key_frame) {
	pkt.flags |= AV_PKT_FLAG_KEY;
    }
    pkt.stream_index = _avStreamPtr->index;
    pkt.data = _videoOutbuf;
    pkt.size = size;
    
    /* write the compressed frame i the media file */
    if (av_write_frame(_ocPtr, &pkt) < 0) {
        status.addError("Error while writing video frame");
	return false;
    }
    return true;
}


bool
AVTranslate::closeVideo(Outcome &status)
{
    if (_avStreamPtr != NULL) {
	avcodec_close(_avStreamPtr->codec);
    }
    if (_pictPtr != NULL) {
	av_free(_pictPtr->data[0]);
	av_free(_pictPtr);
	_pictPtr = NULL;
    }
    if (_rgbPictPtr != NULL) {
	av_free(_rgbPictPtr->data[0]);
	av_free(_rgbPictPtr);
	_rgbPictPtr = NULL;
    }
    if (_videoOutbuf != NULL) {
	av_free(_videoOutbuf);
	_videoOutbuf = NULL;
    }
    return true;
}

/*
status.addError("error while opening file");
status.addContext("Rappture::Buffer::dump()");
return status;
*/

#endif /* HAVE_LIBAVCODEC && HAVE_LIBAVFORMAT */
