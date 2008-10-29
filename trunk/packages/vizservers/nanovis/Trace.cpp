#include <Trace.h>
#include <stdio.h>
#include <stdarg.h>

#include <GL/glew.h>
#include <GL/glut.h>

void Trace(const char* format, ...)
{
    char buff[1024];
    va_list lst;

    va_start(lst, format);
    vsnprintf(buff, 1023, format, lst);
    buff[1023] = '\0';
    printf(buff);
    fflush(stdout);
}

bool
CheckFramebuffer(GLenum *statusPtr) 
{
    *statusPtr = (GLenum)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    return (*statusPtr == GL_FRAMEBUFFER_COMPLETE_EXT);
}



void
PrintFramebufferStatus(GLenum status, const char *string) 
{
  const char *mesg;
  fprintf(stderr, "FB Status: %s: ", string);
  switch(status) {
    case GL_FRAMEBUFFER_COMPLETE_EXT:
      mesg = "<<<< OK >>>>";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
      mesg = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT";
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
      mesg = "GL_FRAMEBUFFER_UNSUPPORTED_EXT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
      mesg = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
      mesg = "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
      mesg = "GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
      mesg = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT";
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
      mesg = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT";
      break;
    default:
      fprintf(stderr, "UNKNOWN framebuffer status %u\n", (unsigned int)status);
      return;
  }
  fprintf(stderr, "%d\n", mesg);
}

bool
CheckGL(const char *string)
{
  const char *mesg;
  GLenum status = (GLenum)glGetError();
  switch(status) {
    case GL_NO_ERROR:
      return true;
    case GL_INVALID_ENUM:
      mesg = "GL_INVALID_ENUM"; break;
    case GL_INVALID_VALUE:
      mesg = "GL_INVALID_VALUE"; break;
    case GL_INVALID_OPERATION:
      mesg = "GL_INVALID_OPERATION"; break;
    case GL_STACK_OVERFLOW:
      mesg = "GL_STACK_OVERFLOW"; break;
    case GL_STACK_UNDERFLOW:
      mesg = "GL_STACK_UNDERFLOW"; break;
    case GL_OUT_OF_MEMORY:
      mesg = "GL_OUT_OF_MEMORY"; break;
    case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
      mesg = "GL_INVALID_FRAMEBUFFER_OPERATION_EXT"; break;
    default:
      fprintf(stderr, "UNKNOWN GL status %d: %s\n", status, string);
      return false;
  } 
  fprintf(stderr, "GL Status: %s: %s", string, mesg);
  return false;
}


