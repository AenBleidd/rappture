/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include "nanovis.h"
#include <Trace.h>
#include <stdio.h>
#include <stdarg.h>

#include <GL/glew.h>
#include <GL/glut.h>
#include <syslog.h>

static const char *syslogLevels[] = {
    "emergency",			/* System is unusable */
    "alert",				/* Action must be taken immediately */
    "critical",				/* Critical conditions */
    "error",				/* Error conditions */
    "warning",				/* Warning conditions */
    "notice",				/* Normal but significant condition */
    "info",				/* Informational */
    "debug",				/* Debug-level messages */
};

void 
LogMessage(int priority, const char *path, int lineNum, const char* fmt, ...)
{
#define MSG_LEN	(2047)
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
    length = snprintf(message, MSG_LEN, "nanovis (%d) %s: %s:%d ", 
		      getpid(), syslogLevels[priority],  s, lineNum);
    length += vsnprintf(message + length, MSG_LEN - length, fmt, lst);
    message[MSG_LEN] = '\0';
    syslog(priority, message, length);
}

bool
CheckFBO(GLenum *statusPtr) 
{
    *statusPtr = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    return (*statusPtr == GL_FRAMEBUFFER_COMPLETE_EXT);
}

void
PrintFBOStatus(GLenum status, const char *prefix) 
{
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
	TRACE("FB Status: %s: UNKNOWN framebuffer status %u\n", 
	       prefix, (unsigned int)status);
	return;
    }
    TRACE("FB Status: %s: %s\n", prefix, mesg);
}

bool
CheckGL(const char *prefix)
{
    const char *mesg;
    GLenum status = (GLenum)glGetError();
    switch(status) {
    case GL_NO_ERROR:
	return true;
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
	TRACE("GL Status: %s: Unknown status %d\n", prefix, status);
	return false;
    } 
    TRACE("GL Status: %s: %s\n", prefix, mesg);
    return false;
}


