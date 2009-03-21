/*
 * ======================================================================
 *  Rappture::AVTranslate
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2009  Purdue Research Foundation
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RAPPTURE_AVTRANSLATE_H
#define RAPPTURE_AVTRANSLATE_H

#include "nvconf.h"
#include "RpOutcome.h"

#ifdef __cplusplus
    extern "C" {
#endif // ifdef __cplusplus

#ifdef HAVE_FFMPEG_AVFORMAT_H
#include <ffmpeg/avformat.h>
#endif
#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
#include <libavformat/avformat.h>
#endif

namespace Rappture {

class AVTranslate {
public:
    AVTranslate(size_t width, size_t height);

    AVTranslate(size_t width, size_t height, size_t bitRate, float frameRate);

    virtual ~AVTranslate();

    bool init(Outcome &status, const char *filename );
    bool append(Outcome &status, uint8_t *rgbData, size_t linePad);
    bool done(Outcome &status);

private:
    bool addVideoStream(Outcome &status, CodecID codecId, AVStream **stream);
    bool allocPicture(Outcome &status, int pixFmt, AVFrame **pic );
    bool openVideo(Outcome &status);
    bool writeVideoFrame(Outcome &status);
    bool closeVideo(Outcome &status);

    size_t _width;
    size_t _height;
    size_t _bitRate;
    float _frameRate;		// frames/seconds
    size_t _videoOutbufSize;
    uint8_t *_videoOutbuf;

    size_t width(void) {
	return _width;
    }
    void width(size_t width) {
	_width = width;
    }
    size_t height(void) {
	return _width;
    }
    void height(size_t width) {
	_width = width;
    }
    size_t bitRate(void) {
	return _bitRate;
    }
    void bitRate(size_t bitRate) {
	_bitRate = bitRate;
    }
    float frameRate(void) {
	return _frameRate;
    }
    void frameRate(size_t frameRate) {
	_frameRate = frameRate;
    }
    AVOutputFormat *_fmtPtr;
    AVFormatContext *_ocPtr;
    AVStream *_avStreamPtr;
    AVFrame *_pictPtr, *_rgbPictPtr;

};

} // namespace Rappture
 
#ifdef __cplusplus
    }
#endif // ifdef __cplusplus

#endif // RAPPTURE_AVTRANSLATE_H
