/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 */
#ifndef NANOVIS_SERVER_H
#define NANOVIS_SERVER_H

#include <sys/types.h> // For pid_t
#include <sys/time.h>  // For struct timeval
#include <cstddef>     // For size_t
#include <cstdio>      // For FILE *

#include <tcl.h>

#include "config.h"

#define MSECS_ELAPSED(t1, t2) \
    ((t1).tv_sec == (t2).tv_sec ? (((t2).tv_usec - (t1).tv_usec)/1.0e+3) : \
     (((t2).tv_sec - (t1).tv_sec))*1.0e+3 + (double)((t2).tv_usec - (t1).tv_usec)/1.0e+3)

#define CVT2SECS(x)  ((double)(x).tv_sec) + ((double)(x).tv_usec * 1.0e-6)

namespace nv {
    class ReadBuffer;
    class ResponseQueue;

    typedef struct {
        pid_t pid;
        size_t nFrames;            /**< # of frames sent to client. */
        size_t nFrameBytes;        /**< # of bytes for all frames. */
        size_t nCommands;          /**< # of commands executed */
        double cmdTime;            /**< Elasped time spend executing
                                    * commands. */
        struct timeval start;      /**< Start of elapsed time. */
    } Stats;

    extern Stats g_stats;

    extern int g_statsFile;        ///< Stats file descriptor
    extern int g_fdIn;             ///< Input file descriptor
    extern int g_fdOut;            ///< Output file descriptor
    extern FILE *g_fIn;            ///< Input file handle
    extern FILE *g_fOut;           ///< Output file handle
    extern FILE *g_fLog;           ///< Trace logging file handle
    extern ReadBuffer *g_inBufPtr; ///< Socket read buffer
#ifdef USE_THREADS
    extern ResponseQueue *g_queue;
#endif

#ifdef KEEPSTATS
    extern int getStatsFile(Tcl_Interp *interp, Tcl_Obj *objPtr);
    extern int writeToStatsFile(int f, const char *s, size_t length);
#endif
    extern void sendDataToClient(const char *command, const char *data,
                                 size_t dlen);
}

#endif
