/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef GEOVIS_TGAWRITER_H
#define GEOVIS_TGAWRITER_H

#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif

namespace GeoVis {
#ifdef USE_THREADS
extern
void queueTGA(ResponseQueue *queue, const char *cmdName, 
              const unsigned char *data, int width, int height, 
              int bytesPerPixel);
#else
extern
void writeTGA(int fd, const char *cmdName, 
              const unsigned char *data, int width, int height, 
              int bytesPerPixel);
#endif
extern
void writeTGAFile(const char *filename, 
                  const unsigned char *data, int width, int height, 
                  int bytesPerPixel, bool srcIsRGB = false);
}

#endif