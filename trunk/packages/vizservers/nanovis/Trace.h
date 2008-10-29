#ifndef __TRACE_H__
#define __TRACE_H__

#include <GL/glew.h>
#include <GL/glut.h>

extern void Trace(const char* format, ...);
extern bool CheckFBO(GLenum *statusPtr);
extern void PrintFBOStatus(GLenum status, const char *prefix);
extern bool CheckGL(const char *prefix);
#endif
