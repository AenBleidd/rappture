/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2013-2014  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef GEOVIS_RENDERSERVER_H
#define GEOVIS_RENDERSERVER_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>

namespace GeoVis {

class Renderer;
class ReadBuffer;
class CommandQueue;
class ResponseQueue;
class Stats;

#define GEOVIS_VERSION_STRING "0.4"

#define MSECS_ELAPSED(t1, t2) \
    ((t1).tv_sec == (t2).tv_sec ? (((t2).tv_usec - (t1).tv_usec)/1.0e+3) : \
     (((t2).tv_sec - (t1).tv_sec))*1.0e+3 + (double)((t2).tv_usec - (t1).tv_usec)/1.0e+3)

#define CVT2SECS(x)  ((double)(x).tv_sec) + ((double)(x).tv_usec * 1.0e-6)

extern Stats g_stats;

extern int g_fdIn;
extern int g_fdOut;
extern FILE *g_fOut;
extern FILE *g_fLog;
extern Renderer *g_renderer;
extern ReadBuffer *g_inBufPtr;
#ifdef USE_THREADS
#ifdef USE_READ_THREAD
extern CommandQueue *g_inQueue;
#endif
extern ResponseQueue *g_outQueue;
#endif
extern int g_statsFile;
extern int writeToStatsFile(int f, const char *s, size_t length);
extern int getStatsFile(const char *key = NULL);

}

#endif
