/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef VTKVIS_TGAWRITER_H
#define VTKVIS_TGAWRITER_H

#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif

namespace VtkVis {
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
