/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2011, Purdue Research Foundation
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#ifndef __TRACE_H__
#define __TRACE_H__

#include <syslog.h>

namespace Rappture {
namespace VtkVis {

extern void InitLog();

extern void CloseLog();

extern void LogMessage(int priority, const char *funcname, const char *fileName, int lineNum,
                       const char* format, ...);

#define ERROR(...)	LogMessage(LOG_ERR, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#ifdef WANT_TRACE
#define TRACE(...)	LogMessage(LOG_DEBUG, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else 
#define TRACE(...) 
#endif
#define WARN(...)	LogMessage(LOG_WARNING, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...)	LogMessage(LOG_INFO, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)

}
}

#endif
