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

void 
CheckFramebuffer(const char *string) 
{
  fprintf(stderr, "FB Status: %s: ", string);
  GLenum status = (GLenum)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  switch(status) {
    case GL_FRAMEBUFFER_COMPLETE_EXT:
      fprintf(stderr, "<<<< OK >>>>\n");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
      fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT\n");
      break;
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
      fprintf(stderr, "GL_FRAMEBUFFER_UNSUPPORTED_EXT\n");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
      fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT\n");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
      fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT\n");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
      fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT\n");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
      fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT\n");
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
      fprintf(stderr, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT\n");
      break;
    default:
      fprintf(stderr, "UNKNOWN framebuffer status %d\n", status);
  }
}

bool
CheckGL(const char *string)
{
  fprintf(stderr, "GL Status: %s: ", string);
  GLenum status = (GLenum)glGetError();
  switch(status) {
    case GL_NO_ERROR:
      fprintf(stderr, "OK\n");
      return true;
    case GL_INVALID_ENUM:
      fprintf(stderr, "GL_INVALID_ENUM\n");
      break;
    case GL_INVALID_VALUE:
      fprintf(stderr, "GL_INVALID_VALUE\n");
      break;
    case GL_INVALID_OPERATION:
      fprintf(stderr, "GL_INVALID_OPERATION\n");
      break;
    case GL_STACK_OVERFLOW:
      fprintf(stderr, "GL_STACK_OVERFLOW\n");
      break;
    case GL_STACK_UNDERFLOW:
      fprintf(stderr, "GL_STACK_UNDERFLOW\n");
      break;
    case GL_OUT_OF_MEMORY:
      fprintf(stderr, "GL_OUT_OF_MEMORY\n");
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
      fprintf(stderr, "GL_INVALID_FRAMEBUFFER_OPERATION_EXT\n");
      break;
    default:
      fprintf(stderr, "UNKNOWN GL status %d\n", status);
  } 
  return false;
}


