/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2013-2014  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#ifndef GEOVIS_STATS_H
#define GEOVIS_STATS_H

#include <cstdlib>
#include <sys/types.h>
#include <sys/time.h>

#ifndef STATSDIR
#define STATSDIR	"/var/tmp/visservers"
#endif

namespace GeoVis {

class Stats {
public:
    pid_t pid;
    size_t nDataSets;       /**< # of data sets received */
    size_t nDataBytes;      /**< # of bytes received as data sets */
    size_t nFrames;         /**< # of frames sent to client. */
    size_t nFrameBytes;     /**< # of bytes for all frames. */
    size_t nCommands;       /**< # of commands executed */
    double cmdTime;         /**< Elasped time spend executing commands. */
    struct timeval start;   /**< Start of elapsed time. */
};

extern int serverStats(const Stats& stats, int code); 

extern int getStatsFile(const char *key);

extern int writeToStatsFile(int fd, const char *s, size_t length);

}

#endif
