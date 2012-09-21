/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2012  HUBzero Foundation, LLC
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <syslog.h>

#include "Trace.h"

using namespace Rappture::VtkVis;

#define MSG_LEN	2047

/**
 * \brief Open syslog for writing
 */
void
Rappture::VtkVis::initLog()
{
    openlog("vtkvis", LOG_CONS | LOG_PERROR | LOG_PID,  LOG_USER);
}

/**
 * \brief Close syslog
 */
void
Rappture::VtkVis::closeLog()
{
    closelog();
}

/**
 * \brief Write a message to syslog
 */
void 
Rappture::VtkVis::logMessage(int priority, const char *funcname,
                             const char *path, int lineNum, const char* fmt, ...)
{
    char message[MSG_LEN + 1];
    const char *s;
    int length;
    va_list lst;

    va_start(lst, fmt);
    s = strrchr(path, '/');
    if (s == NULL) {
	s = path;
    } else {
	s++;
    }
    if (priority & LOG_DEBUG) {
        length = snprintf(message, MSG_LEN, "%s:%d(%s): ", s, lineNum, funcname);
    } else {
        length = snprintf(message, MSG_LEN, "%s:%d: ", s, lineNum);
    }
    length += vsnprintf(message + length, MSG_LEN - length, fmt, lst);
    message[MSG_LEN] = '\0';

    syslog(priority, message, length);
}
