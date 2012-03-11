/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef __TRACE_H__
#define __TRACE_H__

#include <GL/glew.h>
#include <syslog.h>

#include "config.h"

#define MAKE_STRING(x) #x
#define NEWSTRING(x) MAKE_STRING(x)
#define AT __FILE__ ":" NEWSTRING(__LINE__)

extern void LogMessage(int priority, const char *fileName, int lineNum,
			 const char* format, ...);

#define ERROR(...)	LogMessage(LOG_ERR, __FILE__, __LINE__, __VA_ARGS__)
#ifdef WANT_TRACE
#define TRACE(...)	LogMessage(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#else 
#define TRACE(...) 
#endif
#define WARN(...)	LogMessage(LOG_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...)	LogMessage(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)


extern bool CheckFBO(GLenum *statusPtr);
extern void PrintFBOStatus(GLenum status, const char *prefix);
extern bool CheckGL(const char *prefix);
#endif
