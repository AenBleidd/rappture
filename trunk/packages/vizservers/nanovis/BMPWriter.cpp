/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (c) 2004-2013  HUBzero Foundation, LLC
 *
 * Authors:
 *   Wei Qiao <qiaow@purdue.edu>
 *   Insoo Woo <iwoo@purdue.edu>
 *   George A. Howlett <gah@purdue.edu>
 */

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "nanovis.h"
#include "BMPWriter.h"
#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif
#include "Trace.h"

using namespace nv;

// used internally to build up the BMP file header
// Writes an integer value into the header data structure at pos
static inline void
bmpHeaderAddInt(unsigned char* header, int& pos, int data)
{
#ifdef WORDS_BIGENDIAN
    header[pos++] = (data >> 24) & 0xFF;
    header[pos++] = (data >> 16) & 0xFF;
    header[pos++] = (data >> 8)  & 0xFF;
    header[pos++] = (data)       & 0xFF;
#else
    header[pos++] = data & 0xff;
    header[pos++] = (data >> 8) & 0xff;
    header[pos++] = (data >> 16) & 0xff;
    header[pos++] = (data >> 24) & 0xff;
#endif
}

/**
 * \brief Write image data to a bitmap (BMP) file
 *
 * Note that this function assumes the imgData has 4 byte aligned
 * rows (bottom to top) of RGB pixel data.
 */
bool
nv::writeBMPFile(int frameNumber, const char *directoryName,
                 unsigned char *imgData, int width, int height)
{
    unsigned char header[SIZEOF_BMP_HEADER];

    // BE CAREFUL:  BMP files must have an even multiple of 4 bytes
    // on each scan line.  If need be, we add padding to each line.
    int pad = 0;
    if ((3 * width) % 4 > 0) {
        pad = 4 - ((3 * width) % 4);
    }

    size_t headerLength = SIZEOF_BMP_HEADER;
    size_t pixelBytes = (3 * width + pad) * height;
    int fsize = headerLength + pixelBytes;

    // Prepare header
    int pos = 0;
    header[pos++] = 'B';
    header[pos++] = 'M';

    // file size in bytes
    bmpHeaderAddInt(header, pos, fsize);

    // reserved value (must be 0)
    bmpHeaderAddInt(header, pos, 0);

    // offset in bytes to start of bitmap data
    bmpHeaderAddInt(header, pos, headerLength);

    // size of the BITMAPINFOHEADER
    bmpHeaderAddInt(header, pos, 40);

    // width of the image in pixels
    bmpHeaderAddInt(header, pos, width);

    // height of the image in pixels
    bmpHeaderAddInt(header, pos, height);

    // 1 plane + (24 bits/pixel << 16)
    bmpHeaderAddInt(header, pos, 1572865);

    // no compression
    // size of image for compression
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // x pixels per meter
    // y pixels per meter
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // number of colors used (0 = compute from bits/pixel)
    // number of important colors (0 = all colors important)
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // BE CAREFUL: BMP format wants BGR ordering for screen data
    unsigned char *scr = imgData;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            unsigned char tmp = scr[2];
            scr[2] = scr[0];  // B
            scr[0] = tmp;     // R
            scr += 3;
        }
        scr += pad;  // skip over padding already in screen data
    }

    bool ret = true;
    FILE *filp;
    char filename[100];
    if (frameNumber >= 0) {
        if (directoryName)
            sprintf(filename, "%s/image%03d.bmp", directoryName, frameNumber);
        else
            sprintf(filename, "/tmp/flow_animation/image%03d.bmp", frameNumber);

        TRACE("Writing %s", filename);
        filp = fopen(filename, "wb");
        if (filp == NULL) {
            ERROR("cannot create file");
            ret = false;
        }
    } else {
        filp = fopen("/tmp/image.bmp", "wb");
        if (filp == NULL) {
            ERROR("cannot create file");
            ret = false;
        }
    }
    if (fwrite(header, headerLength, 1, filp) != 1) {
        ERROR("can't write header: short write");
        ret = false;
    }
    if (fwrite(imgData, pixelBytes, 1, filp) != 1) {
        ERROR("can't write data: short write");
        ret = false;
    }
    fclose(filp);
    return ret;
}

#ifdef USE_THREADS

/**
 * \brief Write image data to a bitmap (BMP) stream
 *
 * Note that this function assumes the imgData has 4 byte aligned
 * rows (bottom to top) of RGB pixel data.
 */
void
nv::queueBMP(ResponseQueue *queue, const char *cmdName,
             unsigned char *imgData, int width, int height)
{
    unsigned char header[SIZEOF_BMP_HEADER];

    // BE CAREFUL:  BMP files must have an even multiple of 4 bytes
    // on each scan line.  If need be, we add padding to each line.
    int pad = 0;
    if ((3 * width) % 4 > 0) {
        pad = 4 - ((3 * width) % 4);
    }

    size_t headerLength = SIZEOF_BMP_HEADER;
    size_t pixelBytes = (3 * width + pad) * height;
    int fsize = headerLength + pixelBytes;

    // Prepare header
    int pos = 0;
    header[pos++] = 'B';
    header[pos++] = 'M';

    // file size in bytes
    bmpHeaderAddInt(header, pos, fsize);

    // reserved value (must be 0)
    bmpHeaderAddInt(header, pos, 0);

    // offset in bytes to start of bitmap data
    bmpHeaderAddInt(header, pos, headerLength);

    // size of the BITMAPINFOHEADER
    bmpHeaderAddInt(header, pos, 40);

    // width of the image in pixels
    bmpHeaderAddInt(header, pos, width);

    // height of the image in pixels
    bmpHeaderAddInt(header, pos, height);
    // 1 plane + (24 bits/pixel << 16)
    bmpHeaderAddInt(header, pos, 1572865);

    // no compression
    // size of image for compression
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // x pixels per meter
    // y pixels per meter
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // number of colors used (0 = compute from bits/pixel)
    // number of important colors (0 = all colors important)
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    char command[200];
    sprintf(command, "%s %lu\n", cmdName,
            (unsigned long)headerLength + pixelBytes);

    size_t cmdLength;
    cmdLength = strlen(command);

    size_t length = cmdLength + headerLength + pixelBytes;
    unsigned char *mesg = (unsigned char *)malloc(length);
    if (mesg == NULL) {
        ERROR("can't allocate %ld bytes for the image message", length);
        return;
    }
    memcpy(mesg, command, cmdLength);
    memcpy(mesg + cmdLength, header, headerLength);

    // BE CAREFUL: BMP format wants BGR ordering for screen data
    unsigned char *scr = imgData;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            unsigned char tmp = scr[2];
            scr[2] = scr[0];  // B
            scr[0] = tmp;     // R
            scr += 3;
        }
        scr += pad;  // skip over padding already in screen data
    }

    memcpy(mesg + cmdLength + headerLength, imgData, pixelBytes);

    Response *response;
    if (strncmp(cmdName, "nv>legend", 9) == 0) {
        response = new Response(Response::LEGEND);
    } else {
        response = new Response(Response::IMAGE);
    }
    response->setMessage(mesg, length, Response::DYNAMIC);
    queue->enqueue(response);
}
#else
/**
 * \brief Write image data to a bitmap (BMP) stream
 *
 * Note that this function assumes the imgData has 4 byte aligned
 * rows (bottom to top) of RGB pixel data.
 */
void
nv::writeBMP(int fd, const char *prefix,
             unsigned char *imgData, int width, int height)
{
    unsigned char header[SIZEOF_BMP_HEADER];

    // BE CAREFUL:  BMP files must have an even multiple of 4 bytes
    // on each scan line.  If need be, we add padding to each line.
    int pad = 0;
    if ((3 * width) % 4 > 0) {
        pad = 4 - ((3 * width) % 4);
    }

    size_t headerLength = SIZEOF_BMP_HEADER;
    size_t pixelBytes = (3 * width + pad) * height;
    int fsize = headerLength + pixelBytes;

    char command[200];
    sprintf(command, "%s %d\n", prefix, fsize);
    if (write(fd, command, strlen(command)) != (ssize_t)strlen(command)) {
        ERROR("Short write");
        return;
    }

    // Prepare header
    int pos = 0;
    header[pos++] = 'B';
    header[pos++] = 'M';

    // file size in bytes
    bmpHeaderAddInt(header, pos, fsize);

    // reserved value (must be 0)
    bmpHeaderAddInt(header, pos, 0);

    // offset in bytes to start of bitmap data
    bmpHeaderAddInt(header, pos, headerLength);

    // size of the BITMAPINFOHEADER
    bmpHeaderAddInt(header, pos, 40);

    // width of the image in pixels
    bmpHeaderAddInt(header, pos, width);

    // height of the image in pixels
    bmpHeaderAddInt(header, pos, height);

    // 1 plane + (24 bits/pixel << 16)
    bmpHeaderAddInt(header, pos, 1572865);

    // no compression
    // size of image for compression
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // x pixels per meter
    // y pixels per meter
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // number of colors used (0 = compute from bits/pixel)
    // number of important colors (0 = all colors important)
    bmpHeaderAddInt(header, pos, 0);
    bmpHeaderAddInt(header, pos, 0);

    // BE CAREFUL: BMP format wants BGR ordering for screen data
    unsigned char *scr = imgData;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            unsigned char tmp = scr[2];
            scr[2] = scr[0];  // B
            scr[0] = tmp;     // R
            scr += 3;
        }
        scr += pad;  // skip over padding already in screen data
    }

    if (write(fd, header, headerLength) != (ssize_t)headerLength) {
        ERROR("Short write");
        return;
    }
    if (write(fd, imgData, pixelBytes) != (ssize_t)pixelBytes) {
        ERROR("Short write");
        return;
    }
}
#endif
