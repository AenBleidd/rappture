/*
 * ----------------------------------------------------------------------
 *  P2P: performance test program for worker nodes
 *
 *  This little program gets executed from time to time by each worker
 *  to measure the performance and available resources of this node.
 *  It performs a non-trivial calculation involving memory access and
 *  floating point operations, and returns a single number representing
 *  the wall time (in microseconds) for this calculation.  That wall
 *  time rolls up the power of the node and its current availability,
 *  giving a reasonable estimate of how long a similar calculation
 *  would take.  We call this unit of work a "wonk" (WOrker Node
 *  Kapability).
 * ----------------------------------------------------------------------
 *  Michael McLennan (mmclennan@purdue.edu)
 * ======================================================================
 *  Copyright (c) 2004-2012  HUBzero Foundation, LLC
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define LARGE_CHUNK_SIZE 10000000
#define SMALL_CHUNK_SIZE 1000

double ran3();

int
main(argc,argv)
    int argc; char **argv;  /* ignored */
{
    int i, j;
    double d, *dvals;
    long tval;
    struct timeval tstart, tend;

    /* query the starting wall time for this calculation */
    if (gettimeofday(&tstart, (struct timezone*)NULL) < 0) {
        exit(1);
    }

    /*
     *  Allocate a large chunk of memory and zero it out.
     *  This tests available memory and speed of allocation
     *  to some extent.
     */
    dvals = malloc((size_t)(LARGE_CHUNK_SIZE*sizeof(double)));
    if (dvals == NULL) {
        printf("out of memory\n");
        exit(1);
    }
    memset(dvals, 0, (size_t)(LARGE_CHUNK_SIZE*sizeof(double)));

    /*
     *  Scan through the chunk of memory and fill with random
     *  numbers.  This hits lots of memory pages and causes lots
     *  of integer arithmetic operations.
     */
    ran3(-1234);
    for (j=0; j < SMALL_CHUNK_SIZE; j++) {
        for (i=0; i < LARGE_CHUNK_SIZE; i += SMALL_CHUNK_SIZE) {
            dvals[i+j] = ran3(0);
        }
    }

    /*
     *  Scan back through again and multiply random numbers together.
     *  This hits logs of memory pages again and causes lots of
     *  floating point operations.  Use division, since it's more
     *  expensive than multiplication.
     */
    d = 1.0;
    for (j=0; j < SMALL_CHUNK_SIZE; j++) {
        for (i=0; i < LARGE_CHUNK_SIZE; i += SMALL_CHUNK_SIZE) {
            d /= dvals[i+j];
        }
    }

    /* query the ending wall time and report the overall time */
    if (gettimeofday(&tend, (struct timezone*)NULL) < 0) {
        exit(1);
    }

    tval = (tend.tv_sec - tstart.tv_sec)*1000000
             + tend.tv_usec - tstart.tv_usec;

    printf("rappture-wonks/v1 %ld\n", tval);
    exit(0);
}

/*
 * ----------------------------------------------------------------------
 *  FUNCTION: ran3(seed)
 *
 *  This function generates a random number in the range 0.0-1.0.
 *  Set seed to any negative number to reset the sequence with that
 *  seed value.
 *
 *  This routine is taken from "Numerical Recipes in C" by by William
 *  H. Press, Brian P. Flannery, Saul A. Teukolsky, William T.
 *  Vetterling, Chapter 7, p. 283.
 * ----------------------------------------------------------------------
 */
#define MBIG 1000000000
#define MSEED 161803398
#define MZ 0
#define FAC (1.0/MBIG)

double
ran3(seed)
    long seed;  /* pass in negative value to restart */
{
    static int inext, inextp;
    static long ma[56];
    static int iff=0;
    long mj, mk;
    int i, ii, k;

    if (seed < 0 || iff == 0) {
        iff = 1;
        mj = MSEED - (seed < 0 ? -seed : seed);
        mj %= MBIG;
        ma[55] = mj;
        mk = 1;
        for (i=1; i < 54; i++) {
            ii = (21*i) % 55;
            ma[ii] = mk;
            mk = mj - mk;
            if (mk < MZ) mk += MBIG;
            mj = ma[ii];
        }
        for (k=1; k <= 4; k++) {
            for (i=1; i <= 55; i++) {
                ma[i] -= ma[1+(i+30) % 55];
                if (ma[i] < MZ) ma[i] += MBIG;
            }
        }
        inext = 0;
        inextp = 31;
    }
    if (++inext == 56) inext=1;
    if (++inextp == 56) inextp=1;
    mj = ma[inext] - ma[inextp];
    if (mj < MZ) mj += MBIG;
    ma[inext] = mj;
    return mj*FAC;
}
