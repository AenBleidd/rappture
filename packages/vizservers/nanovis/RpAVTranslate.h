/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * ======================================================================
 *  Rappture::AVTranslate
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RP_AVTRANSLATE_H
#define RP_AVTRANSLATE_H 1

#include "nvconf.h"

extern "C" {
#ifndef INT64_C
#if SIZEOF_LONG == 8
#  define INT64_C(c)  c ## L
#  define UINT64_C(c) c ## UL
#else
#  define INT64_C(c)  c ## LL
#  define UINT64_C(c) c ## ULL
# endif
#endif
#ifdef HAVE_FFMPEG_AVFORMAT_H
#include <ffmpeg/avformat.h>
#endif
#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
#include <libavformat/avformat.h>
#endif
}
#include <RpOutcome.h>

namespace Rappture {

class AVTranslate
{
public:
    enum VideoFormats {
        MPEG1,
        MPEG4,
        THEORA,
        QUICKTIME
    };

    AVTranslate(size_t width, size_t height);

    AVTranslate(size_t width, size_t height, size_t bitRate, float frameRate);

    virtual ~AVTranslate();

    bool init(Outcome &status, const char *filename);

    bool append(Outcome &status, uint8_t *rgbData, size_t linePad);

    bool done(Outcome &status);

private:
    bool addVideoStream(Outcome &status, CodecID codecId, AVStream **stream);

    bool allocPicture(Outcome &status, PixelFormat pixFmt, AVFrame **pic );

    bool openVideo(Outcome &status);

    bool writeVideoFrame(Outcome &status);

    bool closeVideo(Outcome &status);

    size_t width()
    {
        return _width;
    }

    void width(size_t width)
    {
        _width = width;
    }

    size_t height()
    {
        return _width;
    }

    void height(size_t width)
    {
        _width = width;
    }

    size_t bitRate()
    {
        return _bitRate;
    }

    void bitRate(size_t bitRate)
    {
        _bitRate = bitRate;
    }

    float frameRate()
    {
        return _frameRate;
    }

    void frameRate(size_t frameRate)
    {
        _frameRate = frameRate;
    }

    size_t _width;
    size_t _height;
    size_t _bitRate;
    float _frameRate;         ///< frames/seconds
    size_t _videoOutbufSize;
    uint8_t *_videoOutbuf;

    AVOutputFormat *_fmtPtr;
    AVFormatContext *_ocPtr;
    AVStream *_avStreamPtr;
    AVFrame *_pictPtr, *_rgbPictPtr;

};

}
 
#endif
