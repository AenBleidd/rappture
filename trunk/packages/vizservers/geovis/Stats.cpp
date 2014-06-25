/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2013-2014  HUBzero Foundation, LLC
 *
 * Author: Leif Delgass <ldelgass@purdue.edu>
 */

#include <cstring>
#include <errno.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <md5.h>

#include <iostream>
#include <string>
#include <sstream>

#include "Stats.h"
#include "RenderServer.h"
#include "Trace.h"

using namespace GeoVis;

/* 
 * Session information:
 *   - Name of render server
 *   - Process ID
 *   - Hostname where server is running
 *   - Start date of session
 *   - Start date of session in seconds
 *   - Number of data sets loaded from client
 *   - Number of data bytes total loaded from client
 *   - Number of frames returned
 *   - Number of bytes total returned (in frames)
 *   - Number of commands received
 *   - Total elapsed time of all commands
 *   - Total elapsed time of session
 *   - Exit code of vizserver
 *   - User time 
 *   - System time
 *   - User time of children 
 *   - System time of children 
 */
int
GeoVis::serverStats(const Stats& stats, int code) 
{
    double start, finish;
    {
	struct timeval tv;

	/* Get ending time. */
	gettimeofday(&tv, NULL);
	finish = CVT2SECS(tv);
	tv = stats.start;
	start = CVT2SECS(tv);
    }

    std::ostringstream oss;
    char buf[BUFSIZ];
    gethostname(buf, BUFSIZ-1);
    buf[BUFSIZ-1] = '\0';
    std::string hostname(buf);
    strcpy(buf, ctime(&stats.start.tv_sec));
    buf[strlen(buf) - 1] = '\0';
    std::string date(buf);

    oss << "render_stop "
        << "renderer geovis "
        << "pid " << getpid() << " "
        << "host " << hostname << " "
        << "date {" << date << "} "
        << "date_secs " << stats.start.tv_sec << " "
        << "num_data_sets " << stats.nDataSets << " "
        << "data_set_bytes " << stats.nDataBytes << " "
        << "num_frames " << stats.nFrames << " "
        << "frame_bytes " << stats.nFrameBytes << " "
        << "num_commands " << stats.nCommands << " "
        << "cmd_time " << stats.cmdTime << " "
        << "session_time " << (finish - start) << " "
        << "status " << code << " ";

    {
	long clocksPerSec = sysconf(_SC_CLK_TCK);
	double clockRes = 1.0 / clocksPerSec;
	struct tms tms;

	memset(&tms, 0, sizeof(tms));
	times(&tms);

        oss << "utime " << (tms.tms_utime * clockRes) << " "
            << "stime " << (tms.tms_stime * clockRes) << " "
            << "cutime " << (tms.tms_cutime * clockRes) << " "
            << "cstime " << (tms.tms_cstime * clockRes) << " ";
    }

    oss << std::endl;

    int fd = getStatsFile();
    std::string ostr = oss.str();
    int result = writeToStatsFile(fd, ostr.c_str(), ostr.length());
    close(fd);
    return result;
}

int
GeoVis::getStatsFile(const char *key)
{
    char fileName[33];
    md5_state_t state;
    md5_byte_t digest[16];

    if (key == NULL || g_statsFile >= 0) {
        return g_statsFile;
    }

    std::ostringstream keyStream;
    keyStream << key << " "
              << getpid();
    std::string keystr = keyStream.str();
    TRACE("Stats file key: '%s'", keystr.c_str());

    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)keystr.c_str(), keystr.length());
    md5_finish(&state, digest);
    for (int i = 0; i < 16; i++) {
        sprintf(fileName + i * 2, "%02x", digest[i]);
    }

    std::ostringstream oss;
    oss << STATSDIR << "/" << fileName;
    g_statsFile = open(oss.str().c_str(), O_EXCL | O_CREAT | O_WRONLY, 0600);
    if (g_statsFile < 0) {
	ERROR("can't open \"%s\": %s", fileName, strerror(errno));
	return -1;
    }
    return g_statsFile;
}

int
GeoVis::writeToStatsFile(int fd, const char *s, size_t length)
{
    if (fd >= 0) {
        ssize_t numWritten;

        numWritten = write(fd, s, length);
        if (numWritten == (ssize_t)length) {
            close(dup(fd));
        }
    }
    return 0;
}
