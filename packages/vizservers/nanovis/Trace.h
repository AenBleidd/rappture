#ifndef __TRACE_H__
#define __TRACE_H__

#include <GL/glew.h>
#include <GL/glut.h>

#define MAKE_STRING(x) #x
#define NEWSTRING(x) MAKE_STRING(x)
#define AT __FILE__ ":" NEWSTRING(__LINE__)

extern void PrintMessage(const char *mesg, const char *fileName, int lineNum,
			 const char* format, ...);

#define ERROR(...)	PrintMessage("Error", __FILE__, __LINE__, __VA_ARGS__)
#ifdef WANT_TRACE
#define TRACE(...)	PrintMessage("Trace", __FILE__, __LINE__, __VA_ARGS__)
#else 
#define TRACE(...) 
#endif
#define WARN(...)	PrintMessage("Warning", __FILE__, __LINE__, __VA_ARGS__)
#define INFO(...)	PrintMessage("Info", __FILE__, __LINE__, __VA_ARGS__)


extern bool CheckFBO(GLenum *statusPtr);
extern void PrintFBOStatus(GLenum status, const char *prefix);
extern bool CheckGL(const char *prefix);
#endif
