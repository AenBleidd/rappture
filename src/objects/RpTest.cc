#include <iostream>
#include <fstream>
#include <cstring>
#include <errno.h>
#include <sys/stat.h>

// size_t indent = 0;
// size_t tabstop = 4;

int testStringVal(
    const char *testname,
    const char *desc,
    const char *expected,
    const char *received)
{
    if ((!expected && received) ||
        (expected && !received) ||
        (expected && received && strcmp(expected,received) != 0)) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%s\"\n",expected);
        printf("\treceived \"%s\"\n",received);
        return 1;
    }
    return 0;
}

int testDoubleVal(
    const char *testname,
    const char *desc,
    double expected,
    double received)
{
    if (expected != received) {
        printf("Error: %s\n", testname);
        printf("\t%s\n", desc);
        printf("\texpected \"%g\"\n",expected);
        printf("\treceived \"%g\"\n",received);
        return 1;
    }
    return 0;
}

size_t
readFile (
    const char *filePath,
    const char **buf)
{
    if (buf == NULL) {
        fprintf(stderr,"buf is NULL while opening file \"%s\"", filePath);
        return 0;
    }

    FILE *f;
    f = fopen(filePath, "rb");
    if (f == NULL) {
        fprintf(stderr,"can't open \"%s\": %s", filePath, strerror(errno));
        return 0;
    }
    struct stat stat;
    if (fstat(fileno(f), &stat) < 0) {
        fprintf(stderr,"can't stat \"%s\": %s", filePath, strerror(errno));
        return 0;
    }
    off_t size;
    size = stat.st_size;
    char* memblock;
    memblock = new char [size+1];
    if (memblock == NULL) {
        fprintf(stderr,"can't allocate %zu bytes for file \"%s\": %s",
            (size_t)size, filePath, strerror(errno));
        fclose(f);
        return 0;
    }

    size_t nRead;
    nRead = fread(memblock, sizeof(char), size, f);
    fclose(f);

    if (nRead != (size_t)size) {
        fprintf(stderr,"can't read %zu bytes from \"%s\": %s",
            (size_t) size, filePath, strerror(errno));
        return 0;
    }

    memblock[size] = '\0';
    *buf = memblock;
    return nRead;
}

