/*
 * ----------------------------------------------------------------------
 *  FUNCTION: rpResult
 *
 *  Clients call this function at the end of their simulation, to
 *  pass the simulation result back to the Rappture GUI.  It writes
 *  out the given XML object to a runXXX.xml file, and then writes
 *  out the name of that file to stdout.
 * ======================================================================
 *  AUTHOR:  Michael McLennan, Purdue University
 *  Copyright (c) 2004-2005
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */
#include <stdlib.h>
#include <time.h>

void
rpResult(PyObject* lib) {
    char outputFile[100];
    char* xtext;
    FILE* fp;
    time_t t;

    xtext = rpXml(lib);

    // create output filename
    snprintf(outputFile, 50, "run%d.xml", (int)time(&t));

    // write out the result file
    fp = fopen(outputFile, "w");
    if(!fp) {
        fprintf(stderr,"can't save results (err=%d)\n", errno);
        return;
    }
    int fsize = fwrite(xtext, strlen(xtext), sizeof(char), fp);
    fclose(fp);

    // tell Rappture the file name
    printf("=RAPPTURE-RUN=>%s\n", outputFile);
}
