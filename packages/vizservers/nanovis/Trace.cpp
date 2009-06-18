#include <Trace.h>
#include <stdio.h>
#include <stdarg.h>

#include <GL/glew.h>
#include <GL/glut.h>

static bool trace = true;

void 
Trace(const char* format, ...)
{
    char buff[1024];
    va_list lst;
    
    if (trace) {
	va_start(lst, format);
	vsnprintf(buff, 1023, format, lst);
	buff[1023] = '\0';
	fprintf(stdout, "%s\n", buff);
	fflush(stdout);
    }
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
	fprintf(stderr, "FB Status: %s: UNKNOWN framebuffer status %u\n", 
		prefix, (unsigned int)status);
	return;
    }
    fprintf(stderr, "FB Status: %s: %s\n", prefix, mesg);
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
	fprintf(stderr, "GL Status: %s: Unknown status %d\n", prefix, status);
	return false;
    } 
    fprintf(stderr, "GL Status: %s: %s\n", prefix, mesg);
    return false;
}


