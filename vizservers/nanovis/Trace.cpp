#include <Trace.h>
#include <stdio.h>

void Trace(char* format, ...)
{
    char buff[1024];

    va_list lst;
    va_start(lst, format);
    vsprintf(buff, format, lst);

    printf(buff);
    fflush(stdout);
}

