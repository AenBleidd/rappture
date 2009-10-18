
int testStringVal( const char *testname, const char *desc,
    const char *expected, const char *received);

int testDoubleVal( const char *testname, const char *desc,
    double expected, double received);

size_t readFile ( const char *filePath, const char **buf);
