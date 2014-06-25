/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <syslog.h>

#include <string>
#include <sstream>

#include "Trace.h"

using namespace GeoVis;

static std::ostringstream g_UserErrorString;

#define MSG_LEN	2047

/**
 * \brief Open syslog for writing
 */
void
GeoVis::initLog()
{
    openlog("geovis", LOG_CONS | LOG_PERROR | LOG_PID,  LOG_USER);
}

/**
 * \brief Close syslog
 */
void
GeoVis::closeLog()
{
    closelog();
}

/**
 * \brief Write a message to syslog
 */
void 
GeoVis::logMessage(int priority, const char *funcname,
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

    syslog(priority, "%s", message);
}

/**
 * \brief Write a user message to buffer
 */
void 
GeoVis::logUserMessage(const char* fmt, ...)
{
    char message[MSG_LEN + 1];
    int length = 0;
    va_list lst;

    va_start(lst, fmt);

    length += vsnprintf(message, MSG_LEN, fmt, lst);
    message[MSG_LEN] = '\0';

    g_UserErrorString << message << "\n";
}

std::string
GeoVis::getUserMessages()
{
    return g_UserErrorString.str();
}

void 
GeoVis::clearUserMessages()
{
    g_UserErrorString.str(std::string());
}
