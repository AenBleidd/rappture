/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/uio.h>

#include "Trace.h"
#include "PPMWriter.h"
#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif

using namespace VtkVis;

#ifdef USE_THREADS

/**
 * \brief Writes image data as PPM binary data to the client.
 *
 * The PPM binary format is very simple.
 *
 *     P6 w h 255\n
 *     3-byte RGB pixel data.
 *
 * The client (using the TkImg library) will do less work to unpack this 
 * format, as opposed to BMP or PNG.
 *
 * Note that currently the image data has bottom to top scanlines.  This 
 * routine could be made even simpler (faster) if the image data had top 
 * to bottom scanlines.
 *
 * \param[in] queue Pointer to ResponseQueue to write to
 * \param[in] cmdName Command name to send (byte length will be appended)
 * \param[in] data Image data
 * \param[in] width Width of image in pixels
 * \param[in] height Height of image in pixels
 */
void
VtkVis::queuePPM(ResponseQueue *queue, const char *cmdName, 
                 const unsigned char *data, int width, int height)
{
#define PPM_MAXVAL 255
    char header[200];

    TRACE("Entering (%dx%d)\n", width, height);
    // Generate the PPM binary file header
    snprintf(header, sizeof(header), "P6 %d %d %d\n", width, height, 
             PPM_MAXVAL);

    size_t headerLength = strlen(header);
    size_t dataLength = width * height * 3;

    char command[200];
    snprintf(command, sizeof(command), "%s %lu\n", cmdName, 
             (unsigned long)headerLength + dataLength);

    size_t cmdLength;
    cmdLength = strlen(command);

    size_t length;
    unsigned char *mesg;

    length = headerLength + dataLength + cmdLength;
    mesg = (unsigned char *)malloc(length);
    if (mesg == NULL) {
        ERROR("can't allocate %ld bytes for the image message", length);
        return;
    }
    memcpy(mesg, command, cmdLength);
    memcpy(mesg + cmdLength, header, headerLength);

    size_t bytesPerRow = width * 3;
    unsigned char *destRowPtr = mesg + length - bytesPerRow;
    int y;
    unsigned char *srcRowPtr = const_cast<unsigned char *>(data);
    for (y = 0; y < height; y++) {
        memcpy(destRowPtr, srcRowPtr, bytesPerRow);
        srcRowPtr += bytesPerRow;
        destRowPtr -= bytesPerRow;
    }

    Response *response;
    if (strncmp(cmdName, "nv>legend", 9) == 0) {
        response = new Response(Response::LEGEND);
    } else {
        response = new Response(Response::IMAGE);
    }
    response->setMessage(mesg, length, Response::DYNAMIC);
    queue->enqueue(response);
    TRACE("Leaving (%dx%d)\n", width, height);
}
#else

/**
 * \brief Writes image data as PPM binary data to the client.
 *
 * The PPM binary format is very simple.
 *
 *     P6 w h 255\n
 *     3-byte RGB pixel data.
 *
 * The client (using the TkImg library) will do less work to unpack this 
 * format, as opposed to BMP or PNG.
 *
 * Note that currently the image data has bottom to top scanlines.  This 
 * routine could be made even simpler (faster) if the image data had top 
 * to bottom scanlines.
 *
 * \param[in] fd File descriptor that will be written to
 * \param[in] cmdName Command name to send (byte length will be appended)
 * \param[in] data Image data
 * \param[in] width Width of image in pixels
 * \param[in] height Height of image in pixels
 */
void
VtkVis::writePPM(int fd, const char *cmdName, 
                 const unsigned char *data, int width, int height)
{
#define PPM_MAXVAL 255
    char header[200];

    TRACE("Entering (%dx%d)\n", width, height);
    // Generate the PPM binary file header
    snprintf(header, sizeof(header), "P6 %d %d %d\n", width, height,
             PPM_MAXVAL);

    size_t headerLength = strlen(header);
    size_t dataLength = width * height * 3;

    char command[200];
    snprintf(command, sizeof(command), "%s %lu\n", cmdName, 
             (unsigned long)headerLength + dataLength);

    size_t nRecs = height + 2;

    struct iovec *iov;
    iov = (struct iovec *)malloc(sizeof(struct iovec) * nRecs);

    // Write the command, then the image header and data.
    // Command
    iov[0].iov_base = command;
    iov[0].iov_len = strlen(command);
    // Header of image data
    iov[1].iov_base = header;
    iov[1].iov_len = headerLength;

    // Image data
    size_t bytesPerRow = width * 3;
    int y;
    unsigned char *srcRowPtr = const_cast<unsigned char *>(data);
    for (y = height + 1; y >= 2; y--) {
        iov[y].iov_base = srcRowPtr;
        iov[y].iov_len = bytesPerRow;
        srcRowPtr += bytesPerRow;
    }
    if (writev(fd, iov, nRecs) < 0) {
        ERROR("write failed: %s\n", strerror(errno));
    }
    free(iov);

    TRACE("Leaving (%dx%d)\n", width, height);
}

#endif /*USE_THREADS*/
