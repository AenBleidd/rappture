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
    AVTranslate( size_t width,
                 size_t height );

    AVTranslate( size_t width,
                 size_t height,
                 size_t bit_rate,
                 double frame_rate );

    virtual ~AVTranslate();

    Outcome init( const char *filename );

    Outcome append( uint8_t *rgb_data,
                    size_t line_pad );

    Outcome close();

private:

    size_t _width;
    size_t _height;
    size_t _bit_rate;
    double _stream_frame_rate; // frames/seconds
    size_t _video_outbuf_size;
    uint8_t *_video_outbuf;


    AVOutputFormat *_fmt;
    AVFormatContext *_oc;
    AVStream *_video_st;
    AVFrame *_picture;
    AVFrame *_rgb_picture;

    Outcome __add_video_stream( CodecID codec_id,
                                AVStream **stream);

    Outcome __alloc_picture( int pix_fmt,
                             AVFrame **pic );

    Outcome __open_video();

    Outcome __write_video_frame();

    void __close_video();
};

} // namespace Rappture
 
#ifdef __cplusplus
    }
#endif // ifdef __cplusplus

#endif // RAPPTURE_AVTRANSLATE_H
