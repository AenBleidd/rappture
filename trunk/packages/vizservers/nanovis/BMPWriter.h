/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#ifndef NV_BMPWRITER_H
#define NV_BMPWRITER_H

#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif

#define SIZEOF_BMP_HEADER   54

namespace nv {
extern
bool writeBMPFile(int frameNumber, const char *directoryName,
                  unsigned char *imgData, int width, int height);

#ifdef USE_THREADS
extern
void queueBMP(ResponseQueue *queue, const char *cmdName, 
              unsigned char *data, int width, int height);
#else
extern
void writeBMP(int fd, const char *cmdName, 
              unsigned char *data, int width, int height);
#endif
}
#endif
