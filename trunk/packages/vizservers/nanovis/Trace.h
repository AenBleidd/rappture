#ifndef __TRACE_H__
#define __TRACE_H__

#include <GL/glew.h>
#include <GL/glut.h>

#define MAKE_STRING(x) #x
#define NEWSTRING(x) MAKE_STRING(x)
#define AT __FILE__ ":" NEWSTRING(__LINE__)

extern void Trace(const char* format, ...);
extern bool CheckFBO(GLenum *statusPtr);
extern void PrintFBOStatus(GLenum status, const char *prefix);
extern bool CheckGL(const char *prefix);
#endif
