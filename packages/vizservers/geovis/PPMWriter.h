/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#ifndef GEOVIS_PPMWRITER_H
#define GEOVIS_PPMWRITER_H

#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif

namespace GeoVis {
#ifdef USE_THREADS
extern
void queuePPM(ResponseQueue *queue, const char *cmdName, 
              const unsigned char *data, int width, int height);
#else
extern
void writePPM(int fd, const char *cmdName, 
              const unsigned char *data, int width, int height);
#endif
}
#endif
