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

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <assert.h>

#include "RpMediaPlayer.h"

using namespace Rappture;

MediaPlayer::MediaPlayer() :
    _width (0),
    _height (0),
    _bitRate(400000),
    _frameRate(25.0f),
    _pFormatCtx(NULL),
    _videoStream(-1),
    _pCodecCtx(NULL),
    _pCodec(NULL),
    _pFrame(NULL),
    _pFrameRGB(NULL),
    _packet(),
    _buffer(NULL),
    _swsctx(NULL)
{
}

/**
 * Copy constructor
 * @param MediaPlayer object to copy
 */
 /*
MediaPlayer::MediaPlayer(const MediaPlayer& o)
{}
*/

MediaPlayer::~MediaPlayer()
{
    // release();
}


bool
MediaPlayer::init(Outcome &status, const char *filename)
{
    status.addContext("Rappture::MediaPlayer::init()");

    /* Register all codecs and formats */
    av_register_all();
    return load(status, filename);
}

bool
MediaPlayer::load(Outcome &status, const char *filename)
{
    status.addContext("Rappture::MediaPlayer::load()");

    // Open video file
    if(av_open_input_file(&_pFormatCtx, filename, NULL, 0, NULL)!=0) {
        status.addError("Could not open file");
        return false;
    }

    // Retrieve stream information
    if(av_find_stream_info(_pFormatCtx)<0) {
        status.addError("Could not find stream information");
        return false;
    }

    // Dump information about file onto standard error
    dump_format(_pFormatCtx, 0, filename, 0);

    // Find the first video stream
    _videoStream=-1;
    for(size_t i=0; i<_pFormatCtx->nb_streams; i++) {
        if(_pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
            _videoStream=i;
            break;
        }
    }

    if(_videoStream==-1) {
        status.addError("Did not find a video stream");
        return false;
    }

    // Get a pointer to the codec context for the video stream
    _pCodecCtx=_pFormatCtx->streams[_videoStream]->codec;

    // Find the decoder for the video stream
    _pCodec=avcodec_find_decoder(_pCodecCtx->codec_id);
    if(_pCodec==NULL) {
        status.addError("Unsupported codec");
        return false;
    }

    // Open codec
    if(avcodec_open(_pCodecCtx, _pCodec)<0) {
        status.addError("Could not open codec");
        return false;
    }

    // Allocate video frame
    _pFrame=avcodec_alloc_frame();

    // Allocate an AVFrame structure
    _pFrameRGB=avcodec_alloc_frame();
    if(_pFrameRGB==NULL) {
        status.addError("Failed to allocate avcodec frame");
        return false;
    }

    // Determine required buffer size and allocate buffer
    int numBytes=avpicture_get_size(PIX_FMT_RGB24, _pCodecCtx->width,
                                    _pCodecCtx->height);
    _buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    avpicture_fill((AVPicture *)_pFrameRGB, _buffer, PIX_FMT_RGB24,
                    _pCodecCtx->width, _pCodecCtx->height);

    /* setup sws context */
    if (_swsctx == NULL) {
        _swsctx = sws_getContext(_pCodecCtx->width, _pCodecCtx->height,
                                 _pCodecCtx->pix_fmt, _pCodecCtx->width,
                                 _pCodecCtx->height, PIX_FMT_RGB24,
                                 SWS_BILINEAR, NULL, NULL, NULL);
        if (_swsctx == NULL) {
            status.addError("Cannot initialize the conversion context");
            return false;
        }
    }

    return true;
}


bool
MediaPlayer::release()
{
    // status.addContext("Rappture::MediaPlayer::release()");

    // Free the RGB image
    if (_buffer) {
        av_free(_buffer);
        _buffer = NULL;
    }

    if (_pFrameRGB) {
        av_free(_pFrameRGB);
        _pFrameRGB = NULL;
    }

    // Free the YUV frame
    if (_pFrame) {
        av_free(_pFrame);
        _pFrame = NULL;
    }

    // Close the codec
    if (_pCodecCtx) {
        avcodec_close(_pCodecCtx);
        _pCodecCtx = NULL;
    }

    // Close the video file
    if (_pFormatCtx) {
        av_close_input_file(_pFormatCtx);
        _pFormatCtx = NULL;
    }

    return true;
}

size_t
MediaPlayer::read(Outcome &status, SimpleCharBuffer *b)
{
    status.addContext("Rappture::MediaPlayer::read()");

    int frameFinished = 0;

    while(av_read_frame(_pFormatCtx, &_packet)>=0) {
        // Is this a packet from the video stream?
        if(_packet.stream_index==_videoStream) {
            // Decode video frame
            avcodec_decode_video2(_pCodecCtx, _pFrame, &frameFinished,
                                  &_packet);

            // Did we get a video frame?
            if (frameFinished) {
                // Convert the image from its native format to RGB
                sws_scale(_swsctx,
                          (const uint8_t * const*)_pFrame->data,
                          _pFrame->linesize, 0, _pCodecCtx->height,
                          _pFrameRGB->data, _pFrameRGB->linesize);

                __frame2ppm(b);
                break;
            }
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&_packet);
    }
    return 1;
}

size_t
MediaPlayer::nframes() const
{
    // status.addContext("Rappture::MediaPlayer::nframes()");
    return 0;
}
/*
int packet_queue_put(PacketQueue *q, AVPacket *pkt) {

  AVPacketList *pkt1;
  if(pkt != &flush_pkt && av_dup_packet(pkt) < 0) {
    return -1;
  }
    if(packet_queue_get(&is->audioq, pkt, 1) < 0) {
      return -1;
    }
    if(packet->data == flush_pkt.data) {
      avcodec_flush_buffers(is->audio_st->codec);
      continue;
    }
*/


int
MediaPlayer::seek(long offset, int whence)
{
    // status.addContext("Rappture::MediaPlayer::seek()");
    int stream_index= -1;
    int64_t seek_target = is->seek_pos;

    if (is->videoStream >= 0) {
        stream_index = is->videoStream;
    } else if (is->audioStream >= 0) {
        stream_index = is->audioStream;
    }

    if(stream_index>=0){
        seek_target= av_rescale_q(seek_target, AV_TIME_BASE_Q,
                      pFormatCtx->streams[stream_index]->time_base);
    }
    if(av_seek_frame(is->pFormatCtx, stream_index, 
                    seek_target, is->seek_flags) < 0) {
        fprintf(stderr, "%s: error while seeking\n",
            is->pFormatCtx->filename);
    } else {
        /* handle packet queues... more later... */
        if(is->audioStream >= 0) {
            packet_queue_flush(&is->audioq);
            packet_queue_put(&is->audioq, &flush_pkt);
        }
        if(is->videoStream >= 0) {
            packet_queue_flush(&is->videoq);
            packet_queue_put(&is->videoq, &flush_pkt);
        }
    }
    is->seek_req = 0;
    return 0;
}

int
MediaPlayer::tell() const
{
    // status.addContext("Rappture::MediaPlayer::tell()");
    return 0;
}

size_t
MediaPlayer::set(size_t nframes)
{
    // status.addContext("Rappture::MediaPlayer::set()");
    return 0;
}

bool
MediaPlayer::good() const
{
    // status.addContext("Rappture::MediaPlayer::good()");
    return true;
}

bool
MediaPlayer::bad() const
{
    // status.addContext("Rappture::MediaPlayer::bad()");
    return false;
}

bool
MediaPlayer::eof() const
{
    // status.addContext("Rappture::MediaPlayer::eof()");
    return false;
}

void
MediaPlayer::__frame2ppm(SimpleCharBuffer *b)
{
    // status.addContext("Rappture::MediaPlayer::__frame2ppm()");

    // FIXME: look into using rewind or some other method that
    //        does not free the already allocated memory. just
    //        reuses the block.
    b->clear();

    // Write header
    b->appendf("P6\n%d %d\n255\n", _pCodecCtx->width, _pCodecCtx->height);

    // Write pixel data
    for(int y=0; y<_pCodecCtx->height; y++) {
        // really want to store a void*...
        b->append((const char *)(_pFrameRGB->data[0]+y*_pFrameRGB->linesize[0]), _pCodecCtx->width*3);
    }
}
