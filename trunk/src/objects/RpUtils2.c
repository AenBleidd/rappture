#include <stdio.h>
#include <stdarg.h>

void
Rp_Assert(testExpr, fileName, lineNumber)
    char *testExpr;
    char *fileName;
    int lineNumber;
{
    fprintf(stderr, "line %d of %s: Assert \"%s\" failed\n",
    lineNumber, fileName, testExpr);
    fflush(stderr);
    abort();
}

void
Rp_Panic (
    const char *format,
    ...)
{
    va_list argList;

    va_start(argList, format);
    vfprintf(stderr, format, argList);
    fprintf(stderr, "\n");
    fflush(stderr);
    abort();
}
