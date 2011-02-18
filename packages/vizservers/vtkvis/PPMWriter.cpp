/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: ?
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/uio.h>

#include "PPMWriter.h"
#include "Trace.h"

using namespace Rappture::VtkVis;

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
 */
void
Rappture::VtkVis::writePPM(int fd, const char *cmdName, const unsigned char *data,
                           int width, int height)
{
#define PPM_MAXVAL 255
    char header[200];

    TRACE("Entering (%dx%d)\n", width, height);
    // Generate the PPM binary file header
    snprintf(header, sizeof(header), "P6 %d %d %d\n", width, height, PPM_MAXVAL);

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
