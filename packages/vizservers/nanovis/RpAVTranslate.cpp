
/*
 * ======================================================================
 *  Rappture::AVTranslate
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2005-2009  Purdue Research Foundation
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */


#include "nvconf.h"

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
			 float frameRate)
  : _width (width),
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
    /* initialize libavcodec, and register all codecs and formats */
    av_register_all();

    /* auto detect the output format from the name. default is
       mpeg. */
    _fmtPtr = guess_format(NULL, filename, NULL);
    if (_fmtPtr == NULL) {
        /*
        printf(  "Could not deduce output format from"
                 "file extension: using MPEG.\n");
        */
        _fmtPtr = guess_format("mpeg", NULL, NULL);
    }
    if (_fmtPtr == NULL) {
        status.addError("Could not find suitable output format");
        return false;
    }

#ifdef notdef
    /* allocate the output media context */
    _ocPtr = av_alloc_format_context();
#else 
    _ocPtr = avformat_alloc_context();
#endif
    if (!_ocPtr) {
        status.addError("Memory error while allocating format context");
        return false;
    }
    _ocPtr->oformat = _fmtPtr;
    snprintf(_ocPtr->filename, sizeof(_ocPtr->filename), "%s", filename);

    /* add the video stream using the default format codecs
       and initialize the codecs */
    _avStreamPtr = NULL;
    if (_fmtPtr->video_codec != CODEC_ID_NONE) {
        if ( (!addVideoStream(status, _fmtPtr->video_codec,&_avStreamPtr)) ) {
            return false;
        }
    }

    /* set the output parameters (must be done even if no
       parameters). */
    if (av_set_parameters(_ocPtr, NULL) < 0) {
        status.addError("Invalid output format parameters");
        return false;
    }

    dump_format(_ocPtr, 0, filename, 1);

    /* now that all the parameters are set, we can open the
       video codec and allocate the necessary encode buffers */
    if (_avStreamPtr) {
        if (!openVideo(status)) {
            return false;
        }
    }

    /* open the output file, if needed */
    if (!(_fmtPtr->flags & AVFMT_NOFILE)) {
        if (url_fopen(&_ocPtr->pb, filename, URL_WRONLY) < 0) {
            status.addError("Could not open '%s'", filename);
            return false;
        }
    }

    /* write the stream header, if any */
    av_write_header(_ocPtr);
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

    uint8_t *destPtr = _rgbPictPtr->data[0];
    uint8_t *srcPtr = rgbData;
    for (size_t y = 0; y < _height; y++) {
        for (size_t x = 0; x < _width; x++) {
            destPtr[0] = srcPtr[0];
            destPtr[1] = srcPtr[1];
            destPtr[2] = srcPtr[2];
            srcPtr += 3;
            destPtr +=3;
        }
        srcPtr += linePad;
    }

#ifdef HAVE_IMG_CONVERT
    // use img_convert instead of sws_scale because img_convert
    // is lgpl nad sws_scale is gpl
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

    /* close each codec */
    if (_avStreamPtr) {
        closeVideo(status);
    }

    /* write the trailer, if any */
    av_write_trailer(_ocPtr);

    /* free the streams */
    for(i = 0; i < _ocPtr->nb_streams; i++) {
        av_freep(&_ocPtr->streams[i]->codec);
        // _ocPtr->streams[i]->codec = NULL;

        av_freep(&_ocPtr->streams[i]);
        // _ocPtr->streams[i] = NULL;
    }

    if (!(_fmtPtr->flags & AVFMT_NOFILE)) {
        /* close the output file */
        url_fclose(_ocPtr->pb);
    }

    /* free the stream */
    av_free(_ocPtr);
    _ocPtr = NULL;
    return true;
}


/* add a video output stream */
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
    streamPtr = av_new_stream(_ocPtr, 0);
    if (streamPtr == NULL) {
        status.addError("Could not alloc stream");
        return false;
    }

    AVCodecContext *codecPtr;
    codecPtr = streamPtr->codec;
    codecPtr->codec_id = codec_id;
    codecPtr->codec_type = CODEC_TYPE_VIDEO;

    /* put sample parameters */
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
    codecPtr->gop_size = 12;	/* emit one intra frame every twelve frames at
				 * most */
    codecPtr->pix_fmt = PIX_FMT_YUV420P;
    if (codecPtr->codec_id == CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B frames */
        codecPtr->max_b_frames = 2;
    }
    if (codecPtr->codec_id == CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
           This does not happen with normal video, it just happens here as
           the motion of the chroma plane does not match the luma plane. */
        codecPtr->mb_decision=2;
    }
    // some formats want stream headers to be separate
    if((strcmp(_ocPtr->oformat->name, "mp4") == 0) || 
       (strcmp(_ocPtr->oformat->name, "mov") == 0) ||
       (strcmp(_ocPtr->oformat->name, "3gp") == 0)) {
        codecPtr->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    *streamPtrPtr = streamPtr;
    return true;
}

bool
AVTranslate::allocPicture(Outcome &status, int pixFmt, AVFrame **framePtrPtr)
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

    status.addContext("Rappture::AVTranslate::openVideo()");
    c = _avStreamPtr->codec;

    /* find the video encoder */
    codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
        status.addError("codec not found");
        return false;
    }

    /* open the codec */
    if (avcodec_open(c, codec) < 0) {
        status.addError("could not open codec");
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
    AVCodecContext *codecPtr;

    status.addContext("Rappture::AVTranslate::writeVideoframe()");
    codecPtr = _avStreamPtr->codec;

    /* encode the image */
    int size;
    size = avcodec_encode_video(codecPtr, _videoOutbuf, _videoOutbufSize, 
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

    pkt.pts = av_rescale_q(codecPtr->coded_frame->pts, codecPtr->time_base,
			   _avStreamPtr->time_base);
    if (codecPtr->coded_frame->key_frame) {
	pkt.flags |= PKT_FLAG_KEY;
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

