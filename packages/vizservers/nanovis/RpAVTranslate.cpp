
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

#include "RpAVTranslate.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>

#include <fstream>
#include <assert.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace Rappture;

extern int img_convert(AVPicture *dst, int dst_pix_fmt,
                const AVPicture *src, int src_pix_fmt,
                int src_width, int src_height);

AVTranslate::AVTranslate(size_t width, size_t height )
  : _width (width),
    _height (height),
    _bit_rate(400000),
    _stream_frame_rate(25),
    _video_outbuf_size (200000),
    _video_outbuf (NULL),
    _fmt (NULL),
    _oc (NULL),
    _video_st (NULL),
    _picture(NULL),
    _rgb_picture(NULL)
{}

AVTranslate::AVTranslate(size_t width, size_t height, size_t bit_rate,
			 double frame_rate)
  : _width (width),
    _height (height),
    _bit_rate(bit_rate),
    _stream_frame_rate(frame_rate),
    _video_outbuf_size (200000),
    _video_outbuf (NULL),
    _fmt (NULL),
    _oc (NULL),
    _video_st (NULL),
    _picture(NULL),
    _rgb_picture(NULL)
{}

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
    /* initialize libavcodec, and register all codecs and formats */
    av_register_all();

    /* auto detect the output format from the name. default is
       mpeg. */
    _fmt = guess_format(NULL, filename, NULL);
    if (_fmt == NULL) {
        /*
        printf(  "Could not deduce output format from"
                 "file extension: using MPEG.\n");
        */
        _fmt = guess_format("mpeg", NULL, NULL);
    }
    if (_fmt == NULL) {
        status.addError("Could not find suitable output format");
        status.addContext("Rappture::AVTranslate::init()");
        return false;
    }

    /* allocate the output media context */
    _oc = av_alloc_format_context();
    if (!_oc) {
        status.addError("Memory error while allocating format context");
        return false;
    }
    _oc->oformat = _fmt;
    snprintf(_oc->filename, sizeof(_oc->filename), "%s", filename);

    /* add the video stream using the default format codecs
       and initialize the codecs */
    _video_st = NULL;
    if (_fmt->video_codec != CODEC_ID_NONE) {
        if ( (!addVideoStream(status, _fmt->video_codec,&_video_st)) ) {
            status.addContext("Rappture::AVTranslate::init()");
            return false;
        }
    }

    /* set the output parameters (must be done even if no
       parameters). */
    if (av_set_parameters(_oc, NULL) < 0) {
        status.addError("Invalid output format parameters");
        status.addContext("Rappture::AVTranslate::init()");
        return false;
    }

    dump_format(_oc, 0, filename, 1);

    /* now that all the parameters are set, we can open the
       video codec and allocate the necessary encode buffers */
    if (_video_st) {
        if (!openVideo(status)) {
            status.addContext("Rappture::AVTranslate::init()");
            return false;
        }
    }

    /* open the output file, if needed */
    if (!(_fmt->flags & AVFMT_NOFILE)) {
        if (url_fopen(&_oc->pb, filename, URL_WRONLY) < 0) {
            status.addError("Could not open '%s'", filename);
            status.addContext("Rappture::AVTranslate::init()");
            return false;
        }
    }

    /* write the stream header, if any */
    av_write_header(_oc);
    return true;
}

bool
AVTranslate::append(Outcome &status, uint8_t *rgb_data, size_t line_pad)
{

    if (rgb_data == NULL) {
        status.addError("rdb_data pointer is NULL");
        status.addContext("Rappture::AVTranslate::append()");
        return false;
    }

    uint8_t *pdatat = _rgb_picture->data[0];
    uint8_t *rgbcurr = rgb_data;

    for (size_t y = 0; y < _height; y++) {
        for (size_t x = 0; x < _width; x++) {
            pdatat[0] = rgbcurr[0];
            pdatat[1] = rgbcurr[1];
            pdatat[2] = rgbcurr[2];

            rgbcurr += 3;
            pdatat +=3;
        }
        rgbcurr += line_pad;
    }

    // use img_convert instead of sws_scale because img_convert
    // is lgpl nad sws_scale is gpl
    img_convert((AVPicture *)_picture, PIX_FMT_YUV420P,
                (AVPicture *)_rgb_picture, PIX_FMT_RGB24,
                _width, _height);

    writeVideoFrame(status);

    return true;
}


bool
AVTranslate::done(Outcome &status)
{
    size_t i = 0;

    /* close each codec */
    if (_video_st) {
        closeVideo(status);
    }

    /* write the trailer, if any */
    av_write_trailer(_oc);

    /* free the streams */
    for(i = 0; i < _oc->nb_streams; i++) {
        av_freep(&_oc->streams[i]->codec);
        // _oc->streams[i]->codec = NULL;

        av_freep(&_oc->streams[i]);
        // _oc->streams[i] = NULL;
    }

    if (!(_fmt->flags & AVFMT_NOFILE)) {
        /* close the output file */
        url_fclose(_oc->pb);
    }

    /* free the stream */
    av_free(_oc);
    _oc = NULL;
    return true;
}


/* add a video output stream */
bool
AVTranslate::addVideoStream(Outcome &status, CodecID codec_id, AVStream **st)
{
    AVCodecContext *c;
    // AVStream *st;

    if (!st) {
        status.addError("AVStream **st is NULL");
        status.addContext("Rappture::AVTranslate::add_video_stream()");
        return false;
    }

    *st = av_new_stream(_oc, 0);
    if (!(*st)) {
        status.addError("Could not alloc stream");
        status.addContext("Rappture::AVTranslate::add_video_stream()");
        return false;
    }

    c = (*st)->codec;
    c->codec_id = codec_id;
    c->codec_type = CODEC_TYPE_VIDEO;

    /* put sample parameters */
    c->bit_rate = _bit_rate;
    /* resolution must be a multiple of two */
    c->width = _width;
    c->height = _height;
    /* time base: this is the fundamental unit of time (in seconds) in terms
       of which frame timestamps are represented. for fixed-fps content,
       timebase should be 1/framerate and timestamp increments should be
       identically 1. */
    c->time_base.den = _stream_frame_rate;
    c->time_base.num = 1;
    c->gop_size = 12; /* emit one intra frame every twelve frames at most */
    c->pix_fmt = PIX_FMT_YUV420P;
    if (c->codec_id == CODEC_ID_MPEG2VIDEO) {
        /* just for testing, we also add B frames */
        c->max_b_frames = 2;
    }
    if (c->codec_id == CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
           This does not happen with normal video, it just happens here as
           the motion of the chroma plane does not match the luma plane. */
        c->mb_decision=2;
    }
    // some formats want stream headers to be separate
    if(    !strcmp(_oc->oformat->name, "mp4")
        || !strcmp(_oc->oformat->name, "mov")
        || !strcmp(_oc->oformat->name, "3gp")) {
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }

    return true;
}

bool
AVTranslate::allocPicture(Outcome &status, int pix_fmt, AVFrame **p)
{
    // AVFrame *p = NULL;
    uint8_t *p_buf = NULL;
    int size = 0;

    if (p == NULL) {
        status.addError("AVFrame **p == NULL");
        status.addContext("Rappture::AVTranslate::allocPicture()");
        return false;
    }

    *p = avcodec_alloc_frame();
    if (*p == NULL) {
        status.addError("Memory error: Could not alloc frame");
        status.addContext("Rappture::AVTranslate::allocPicture()");
        return false;
    }
    size = avpicture_get_size(pix_fmt, _width, _height);
    p_buf = (uint8_t *) av_malloc(size);
    if (!p_buf) {
        av_free(*p);
        *p = NULL;
        status.addError("Memory error: Could not alloc picture buffer");
        status.addContext("Rappture::AVTranslate::allocPicture()");
        return false;
    }
    avpicture_fill((AVPicture *)(*p), p_buf, pix_fmt, _width, _height);
    return true;
}

bool
AVTranslate::openVideo(Outcome &status)
{
    AVCodec *codec;
    AVCodecContext *c;

    c = _video_st->codec;

    /* find the video encoder */
    codec = avcodec_find_encoder(c->codec_id);
    if (!codec) {
        status.addError("codec not found");
        status.addContext("Rappture::AVTranslate::open_video()");
        return false;
    }

    /* open the codec */
    if (avcodec_open(c, codec) < 0) {
        status.addError("could not open codec");
        status.addContext("Rappture::AVTranslate::open_video()");
        return false;
    }

    _video_outbuf = NULL;
    if (!(_oc->oformat->flags & AVFMT_RAWPICTURE)) {
        /* allocate output buffer */
        /* XXX: API change will be done */
        /* buffers passed into lav* can be allocated any way you prefer,
           as long as they're aligned enough for the architecture, and
           they're freed appropriately (such as using av_free for buffers
           allocated with av_malloc) */
        _video_outbuf = (uint8_t *) av_malloc(_video_outbuf_size);
    }

    /* allocate the encoded raw picture */
    if (!allocPicture(status, c->pix_fmt,&_picture)) {
        status.addContext("Rappture::AVTranslate::openVideo()");
        return false;
    }

    /*
    if (!picture) {
        status.addError("Could not allocate picture");
        status.addContext("Rappture::AVTranslate::openVideo()");
        return false;
    }
    */

    if (!allocPicture(status, PIX_FMT_RGB24,&_rgb_picture)) {
        status.addContext("Rappture::AVTranslate::openVideo()");
        return false;
    }

    /*
    if (!rgb_picture) {
        status.addError("Could not allocate temporary picture");
        status.addContext("Rappture::AVTranslate::open_video()");
        return false;
    }
    */

    return true;
}

bool
AVTranslate::writeVideoFrame(Outcome &status)
{
    int out_size, ret;
    AVCodecContext *c;

    c = _video_st->codec;

    /* encode the image */
    out_size = avcodec_encode_video(c, _video_outbuf,
                                    _video_outbuf_size, _picture);

    /* if zero size, it means the image was buffered */
    if (out_size > 0) {
        AVPacket pkt;
        av_init_packet(&pkt);

        pkt.pts = av_rescale_q( c->coded_frame->pts,
                                c->time_base,
                                _video_st->time_base);

        if (c->coded_frame->key_frame) {
            pkt.flags |= PKT_FLAG_KEY;
        }
        pkt.stream_index = _video_st->index;
        pkt.data = _video_outbuf;
        pkt.size = out_size;

        /* write the compressed frame in the media file */
        ret = av_write_frame(_oc, &pkt);
    } else {
        ret = 0;
    }

    if (ret != 0) {
        status.addError("Error while writing video frame");
        status.addContext("Rappture::AVTranslate::writeVideoframe()");
	return false;
    }
    return true;
}


bool
AVTranslate::closeVideo(Outcome &status)
{
    avcodec_close(_video_st->codec);

    av_free(_picture->data[0]);
    _picture->data[0] = NULL;

    av_free(_picture);
    _picture = NULL;

    av_free(_rgb_picture->data[0]);
    _rgb_picture->data[0] = NULL;

    av_free(_rgb_picture);
    _rgb_picture = NULL;

    av_free(_video_outbuf);
    _video_outbuf = NULL;
    return true;
}

/*
status.addError("error while opening file");
status.addContext("Rappture::Buffer::dump()");
return status;
*/

