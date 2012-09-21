/*
 * ======================================================================
 *  Rappture::MediaPlayer
 *
 *  AUTHOR:  Derrick Kearney, Purdue University
 *
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 * ----------------------------------------------------------------------
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifndef RP_MEDIAPLAYER_H
#define RP_MEDIAPLAYER_H 1

#include "config.h"

extern "C" {
#define __STDC_CONSTANT_MACROS 1

#ifdef HAVE_FFMPEG_AVFORMAT_H
#include <ffmpeg/avformat.h>
#endif
#ifdef HAVE_LIBAVFORMAT_AVFORMAT_H
#include <libavformat/avformat.h>
#endif

#ifdef HAVE_FFMPEG_AVCODEC_H
#include <ffmpeg/avcodec.h>
#endif
#ifdef HAVE_LIBAVCODEC_AVCODEC_H
#include <libavcodec/avcodec.h>
#endif

#ifdef HAVE_FFMPEG_SWSCALE_H
#include <ffmpeg/swscale.h>
#endif
#ifdef HAVE_LIBSWSCALE_SWSCALE_H
#include <libswscale/swscale.h>
#endif
}


#include "RpOutcome.h"
#include "RpSimpleBuffer.h"

namespace Rappture {

class MediaPlayer {
public:
    enum VideoFormats { MPEG1, MPEG4, THEORA, QUICKTIME };
    MediaPlayer();
    virtual ~MediaPlayer();

    bool init(Outcome &status, const char *filename);
    bool load(Outcome &status, const char *filename);
    bool release();

    size_t nframes() const;

    size_t read(Outcome &status, SimpleCharBuffer *b);
    int seek(long offset, int whence);
    int tell() const;
    size_t set(size_t nframes);

    bool good() const;
    bool bad() const;
    bool eof() const;

private:
    void __frame2ppm(SimpleCharBuffer *b);

    size_t _width;
    size_t _height;
    size_t _bitRate;
    float _frameRate;                // frames/seconds

    AVFormatContext *_pFormatCtx;
    int _videoStream;
    AVCodecContext *_pCodecCtx;
    AVCodec *_pCodec;
    AVFrame *_pFrame;
    AVFrame *_pFrameRGB;
    AVPacket _packet;
    uint8_t *_buffer;
    struct SwsContext *_swsctx;



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

};

} // namespace Rappture

#endif /* RP_MEDIAPLAYER_H */
