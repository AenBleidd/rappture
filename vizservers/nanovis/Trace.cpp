#include <Trace.h>
#include <stdio.h>
#include <stdarg.h>

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

