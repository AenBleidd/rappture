/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2013  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef GEOVIS_RENDERSERVER_H
#define GEOVIS_RENDERSERVER_H

#include <sys/types.h>
#include <sys/time.h>

namespace GeoVis {

class Renderer;
class ReadBuffer;
class CommandQueue;
class ResponseQueue;

#define GEOVIS_VERSION_STRING "0.2"

#define MSECS_ELAPSED(t1, t2) \
    ((t1).tv_sec == (t2).tv_sec ? (((t2).tv_usec - (t1).tv_usec)/1.0e+3) : \
     (((t2).tv_sec - (t1).tv_sec))*1.0e+3 + (double)((t2).tv_usec - (t1).tv_usec)/1.0e+3)

#define CVT2SECS(x)  ((double)(x).tv_sec) + ((double)(x).tv_usec * 1.0e-6)

#ifndef STATSDIR
#define STATSDIR	"/var/tmp/visservers"
#endif

typedef struct {
    pid_t pid;
    size_t nDataSets;       /**< # of data sets received */
    size_t nDataBytes;      /**< # of bytes received as data sets */
    size_t nFrames;         /**< # of frames sent to client. */
    size_t nFrameBytes;     /**< # of bytes for all frames. */
    size_t nCommands;       /**< # of commands executed */
    double cmdTime;         /**< Elasped time spend executing commands. */
    struct timeval start;   /**< Start of elapsed time. */
} Stats;

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
extern int getStatsFile(Tcl_Interp * interp, Tcl_Obj *objPtr);

}

#endif
