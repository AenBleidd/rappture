/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/uio.h>

#include <iostream>
#include <fstream>

#include "TGAWriter.h"
#include "Trace.h"
#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif

#ifdef USE_THREADS

/**
 * \brief Writes image command + data to supplied file descriptor.
 *
 * The image data must be supplied in BGR(A) order with bottom to 
 * top scanline ordering.
 *
 * \param[in] queue Pointer to ResponseQueue to write to
 * \param[in] cmdName Command name to send (byte length will be appended)
 * \param[in] data Image data
 * \param[in] width Width of image in pixels
 * \param[in] height Height of image in pixels
 * \param[in] bytesPerPixel Should be 3 or 4, depending on alpha
 */
void
VtkVis::queueTGA(ResponseQueue *queue, const char *cmdName, 
                 const unsigned char *data,
                 int width, int height,
                 int bytesPerPixel)
{
    TRACE("(%dx%d)\n", width, height);

    size_t headerLength = 18;

    char header[headerLength];
    memset(header, 0, headerLength);
    header[2] = (char)2;  // image type (2 = uncompressed true-color)
    header[12] = (char)width;
    header[13] = (char)(width >> 8);
    header[14] = (char)height;
    header[15] = (char)(height >> 8);
    header[16] = (char)(bytesPerPixel*8); // bits per pixel

    size_t dataLength = width * height * bytesPerPixel;
    size_t cmdLength;

    char command[200];
    cmdLength = snprintf(command, sizeof(command), "%s %lu\n", cmdName, 
                         (unsigned long)headerLength + dataLength);

    size_t length;
    unsigned char *mesg = NULL;

    length = headerLength + dataLength + cmdLength;
    mesg = (unsigned char *)malloc(length);
    if (mesg == NULL) {
        ERROR("can't allocate %ld bytes for the image message", length);
        return;
    }
    memcpy(mesg, command, cmdLength);
    memcpy(mesg + cmdLength, header, headerLength);
    memcpy(mesg + cmdLength + headerLength, 
           const_cast<unsigned char *>(data), dataLength);

    Response *response = NULL;
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
 * \brief Writes image command + data to supplied file descriptor.
 *
 * The image data must be supplied in BGR(A) order with bottom to 
 * top scanline ordering.
 *
 * \param[in] fd File descriptor that will be written to
 * \param[in] cmdName Command name to send (byte length will be appended)
 * \param[in] data Image data
 * \param[in] width Width of image in pixels
 * \param[in] height Height of image in pixels
 * \param[in] bytesPerPixel Should be 3 or 4, depending on alpha
 */
void
VtkVis::writeTGA(int fd, const char *cmdName,
                 const unsigned char *data,
                 int width, int height,
                 int bytesPerPixel)
{
    TRACE("(%dx%d)\n", width, height);

    size_t headerLength = 18;

    char header[headerLength];
    memset(header, 0, headerLength);
    header[2] = (char)2;  // image type (2 = uncompressed true-color)
    header[12] = (char)width;
    header[13] = (char)(width >> 8);
    header[14] = (char)height;
    header[15] = (char)(height >> 8);
    header[16] = (char)(bytesPerPixel*8); // bits per pixel

    size_t dataLength = width * height * bytesPerPixel;

    char command[200];
    snprintf(command, sizeof(command), "%s %lu\n", cmdName, 
             (unsigned long)headerLength + dataLength);

    size_t nRecs = 3;

    struct iovec *iov;
    iov = (struct iovec *)malloc(sizeof(struct iovec) * nRecs);

    // Write the command, then the image header and data.
    // Command
    iov[0].iov_base = command;
    iov[0].iov_len = strlen(command);
    // Header of image data
    iov[1].iov_base = header;
    iov[1].iov_len = headerLength;
    // Image data **must be BGR(A)!**
    iov[2].iov_base = const_cast<unsigned char *>(data);
    iov[2].iov_len = dataLength;

    if (writev(fd, iov, nRecs) < 0) {
	ERROR("write failed: %s\n", strerror(errno));
    }
    free(iov);

    TRACE("Leaving (%dx%d)\n", width, height);
}
#endif  /*USE_THREADS*/

/**
 * \brief Writes image data to supplied file name
 *
 * The image data must be supplied with bottom to top 
 * scanline ordering.  Source data should have BGR(A) 
 * ordering, unless srcIsRGB is true, in which case 
 * the source data will be converted from RGB(A) to 
 * BGR(A).  Note that this is slow and it is better 
 * to pass in BGR(A) data.
 *
 * \param[in] filename Path to file that will be written
 * \param[in] imgData Image data
 * \param[in] width Width of image in pixels
 * \param[in] height Height of image in pixels
 * \param[in] bytesPerPixel Should be 3 or 4, depending on alpha
 * \param[in] srcIsRGB If true source data will be re-ordered
 */
void
VtkVis::writeTGAFile(const char *filename,
                     const unsigned char *imgData,
                     int width, int height,
                     int bytesPerPixel,
                     bool srcIsRGB)
{
    TRACE("%s (%dx%d)\n", filename, width, height);

    std::ofstream outfile(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    char header[18];
    memset(header, 0, 18);
    header[2] = (char)2;  // image type (2 = uncompressed true-color)
    header[12] = (char)width;
    header[13] = (char)(width >> 8);
    header[14] = (char)height;
    header[15] = (char)(height >> 8);
    header[16] = (char)(bytesPerPixel*8); // bits per pixel

    outfile.write(header, sizeof(header));

    if (!srcIsRGB) {
        outfile.write((const char *)imgData, width * height * bytesPerPixel);
    } else {
        // RGB(A) -> BGR(A)
        for (int i = 0; i < width * height; i++) {
            outfile << imgData[i*bytesPerPixel+2]
                    << imgData[i*bytesPerPixel+1]
                    << imgData[i*bytesPerPixel];
            if (bytesPerPixel == 4) {
                outfile << imgData[i*bytesPerPixel+3];
            }
        }
    }

    outfile.close();
}
