#ifndef __TRACE_H__
#define __TRACE_H__

#include <GL/glew.h>
#include <GL/glut.h>

extern void Trace(const char* format, ...);
extern bool CheckFramebuffer(GLenum *statusPtr);
extern void PrintFramebufferStatus(GLenum status, const char *string);
extern bool CheckGL(const char *string);
#endif
