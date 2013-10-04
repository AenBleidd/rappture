/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#ifndef GEOVIS_TRACE_H
#define GEOVIS_TRACE_H

#include <syslog.h>

namespace GeoVis {

extern void logUserMessage(const char* format, ...);

extern const char *getUserMessages();

extern void clearUserMessages();

extern void initLog();

extern void closeLog();

extern void logMessage(int priority, const char *funcname, const char *fileName,
                       int lineNum, const char* format, ...);

#define ERROR(...)	GeoVis::logMessage(LOG_ERR, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#ifdef WANT_TRACE
#define TRACE(...)	GeoVis::logMessage(LOG_DEBUG, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else 
#define TRACE(...) 
#endif  /*WANT_TRACE*/
#define WARN(...)	GeoVis::logMessage(LOG_WARNING, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...)	GeoVis::logMessage(LOG_INFO, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)

#define USER_ERROR(...) GeoVis::logUserMessage(__VA_ARGS__)

}

#endif
