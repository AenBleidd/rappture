/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
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

/**
 * \brief Writes image command + data to supplied file descriptor.
 *
 * The image data must be supplied in BGR order with bottom to 
 * top scanline ordering.
 */
void
Rappture::VtkVis::writeTGA(int fd, const char *cmdName, const unsigned char *data,
                           int width, int height)
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
    header[16] = (char)24; // bits per pixel

    size_t dataLength = width * height * 3;

    char command[200];
    snprintf(command, sizeof(command), "%simage -type image -bytes %lu\n", cmdName, 
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
    // Image data **must be BGR!**
    iov[2].iov_base = const_cast<unsigned char *>(data);
    iov[2].iov_len = dataLength;

    if (writev(fd, iov, nRecs) < 0) {
	ERROR("write failed: %s\n", strerror(errno));
    }
    free(iov);

    TRACE("Leaving (%dx%d)\n", width, height);
}

/**
 * \brief Writes image data to supplied file name
 *
 * The image data must be supplied in BGR order with bottom to 
 * top scanline ordering.
 */
void
Rappture::VtkVis::writeTGAFile(const char *filename, const unsigned char *imgData,
                               int width, int height)
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
    header[16] = (char)24; // bits per pixel

    outfile.write(header, sizeof(header));

    // RGB -> BGR
    for (int i = 0; i < width * height; i++) {
        outfile << imgData[i*3+2]
                << imgData[i*3+1]
                << imgData[i*3];
    }

    outfile.close();
}
