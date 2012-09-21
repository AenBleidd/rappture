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
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <stdlib.h>
#include <time.h>
#include <RpLibrary.h>
#include <errno.h>

void
rpResult(RpLibrary* lib) 
{
    char outputFile[100];
    std::string xtext;
    FILE* fp;
    time_t t;

    xtext = lib->xml();

    // create output filename
    snprintf(outputFile, 50, "run%d.xml", (int)time(&t));

    // write out the result file
    fp = fopen(outputFile, "w");
    if(!fp) {
        fprintf(stderr,"can't save results: %s\n", strerror(errno));
        return;
    }
    int fsize = fwrite(xtext.c_str(), xtext.length(), sizeof(char), fp);
    if (fsize != (int)xtext.length()) {
        fprintf(stderr, "short write: can't save results: %s\n", 
		strerror(errno));
        fclose(fp);
        return;
    }
    fclose(fp);
    // tell Rappture the file name
    printf("=RAPPTURE-RUN=>%s\n", outputFile);
}
