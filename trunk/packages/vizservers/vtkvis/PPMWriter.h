/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#ifndef __RAPPTURE_VTKVIS_PPMWRITER_H__
#define __RAPPTURE_VTKVIS_PPMWRITER_H__

#ifdef USE_THREADS
#include "ResponseQueue.h"
#endif

namespace Rappture {
namespace VtkVis {
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
}
#endif
