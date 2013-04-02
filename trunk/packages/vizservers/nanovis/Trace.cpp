/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * Copyright (C) 2004-2013  HUBzero Foundation, LLC
 *
 * Author: George A. Howlett <gah@purdue.edu>
 */
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>

#include <string>
#include <sstream>

#include <GL/glew.h>

#include "nanovis.h"
#include "Trace.h"

using namespace nv;

static std::ostringstream g_UserErrorString;

#define MSG_LEN	2047

/**
 * \brief Open syslog for writing
 */
void
nv::initLog()
{
    openlog("nanovis", LOG_CONS | LOG_PERROR | LOG_PID,  LOG_USER);
}

/**
 * \brief Close syslog
 */
void
nv::closeLog()
{
    closelog();
}

/**
 * \brief Write a message to syslog
 */
void 
nv::LogMessage(int priority, const char *funcname,
               const char *path, int lineNum, const char* fmt, ...)
{
    char message[MSG_LEN+1];
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
nv::logUserMessage(const char* fmt, ...)
{
    char message[MSG_LEN + 1];
    int length = 0;
    va_list lst;

    va_start(lst, fmt);

    length += vsnprintf(message, MSG_LEN, fmt, lst);
    message[MSG_LEN] = '\0';

    g_UserErrorString << message << "\n";
}

const char *
nv::getUserMessages()
{
    return g_UserErrorString.str().c_str();
}

void 
nv::clearUserMessages()
{
    g_UserErrorString.str(std::string());
}

bool
CheckFBO(GLenum *status) 
{
    *status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    return (*status == GL_FRAMEBUFFER_COMPLETE_EXT);
}

void
PrintFBOStatus(GLenum status, const char *prefix) 
{
#ifdef WANT_TRACE
    const char *mesg;

    switch(status) {
    case GL_FRAMEBUFFER_COMPLETE_EXT:
	mesg = "<<<< OK >>>>";						break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
	mesg = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT";		break;
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
	mesg = "GL_FRAMEBUFFER_UNSUPPORTED_EXT";			break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
	mesg = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT";	break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
	mesg = "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT";		break;
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
	mesg = "GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT";			break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
	mesg = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT";		break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
	mesg = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT";		break;
    default:
	TRACE("FB Status: %s: UNKNOWN framebuffer status %u", 
	       prefix, (unsigned int)status);
	return;
    }
    TRACE("FB Status: %s: %s", prefix, mesg);
#endif  /*WANT_TRACE*/
}

bool
CheckGL(const char *prefix)
{
    GLenum status = (GLenum)glGetError();
    if (status == GL_NO_ERROR) {
	return true;
    }
#ifdef WANT_TRACE
    const char *mesg;                   

    switch(status) {
    case GL_INVALID_ENUM:
	mesg = "GL_INVALID_ENUM";			break;
    case GL_INVALID_VALUE:
	mesg = "GL_INVALID_VALUE";			break;
    case GL_INVALID_OPERATION:
	mesg = "GL_INVALID_OPERATION";			break;
    case GL_STACK_OVERFLOW:
	mesg = "GL_STACK_OVERFLOW";			break;
    case GL_STACK_UNDERFLOW:
	mesg = "GL_STACK_UNDERFLOW";			break;
    case GL_OUT_OF_MEMORY:
	mesg = "GL_OUT_OF_MEMORY";			break;
    case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
	mesg = "GL_INVALID_FRAMEBUFFER_OPERATION_EXT";	break;
    default:
	TRACE("GL Status: %s: Unknown status %d", prefix, status);
	return false;
    } 
    TRACE("GL Status: %s: %s", prefix, mesg);
#endif
    return false;
}
