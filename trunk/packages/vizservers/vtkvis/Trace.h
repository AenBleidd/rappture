/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#ifndef VTKVIS_TRACE_H
#define VTKVIS_TRACE_H

#include <string>

#include <syslog.h>

namespace VtkVis {

extern void logUserMessage(const char* format, ...);

extern std::string getUserMessages();

extern void clearUserMessages();

extern void initLog();

extern void closeLog();

extern void logMessage(int priority, const char *funcname, const char *fileName,
                       int lineNum, const char* format, ...);

#define ERROR(...)	VtkVis::logMessage(LOG_ERR, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#ifdef WANT_TRACE
#define TRACE(...)	VtkVis::logMessage(LOG_DEBUG, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else 
#define TRACE(...) 
#endif  /*WANT_TRACE*/
#define WARN(...)	VtkVis::logMessage(LOG_WARNING, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...)	VtkVis::logMessage(LOG_INFO, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)

#define USER_ERROR(...) VtkVis::logUserMessage(__VA_ARGS__)

}

#endif
