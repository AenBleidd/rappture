/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#ifndef __TRACE_H__
#define __TRACE_H__

#include <syslog.h>

namespace Rappture {
namespace VtkVis {

extern void initLog();

extern void closeLog();

extern void logMessage(int priority, const char *funcname, const char *fileName,
                       int lineNum, const char* format, ...);

#define ERROR(...)	Rappture::VtkVis::logMessage(LOG_ERR, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#ifdef WANT_TRACE
#define TRACE(...)	Rappture::VtkVis::logMessage(LOG_DEBUG, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else 
#define TRACE(...) 
#endif  /*WANT_TRACE*/
#define WARN(...)	Rappture::VtkVis::logMessage(LOG_WARNING, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...)	Rappture::VtkVis::logMessage(LOG_INFO, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)

}
}

#endif
