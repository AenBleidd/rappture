/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Rappture-Octave Bindings Header
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2005-2007
 *  Purdue Research Foundation, West Lafayette, IN
 * ======================================================================
 */

#ifndef _Rp_OCTAVE_HELPER_H
#define _Rp_OCTAVE_HELPER_H

#include "rappture.h"
#include "RpBindingsDict.h"

// include the octave api header
#include "octave/oct.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

// octave versions before 3.0 used a version of the
// print_usage function that accepted a parameter
// telling the name of the function to print the
// usage of. octave 3.0 introduced a print_usage()
// function that automatically finds the name of
// the function it is being called from.
#define OCTAVE_VERSION_MAJOR 2
#if (OCTAVE_VERSION_MAJOR >= 3)
    #define _PRINT_USAGE(x) print_usage()
#else
    #define _PRINT_USAGE(x) print_usage(x)
#endif // (OCTAVE_VERSION_MAJOR >= 3)

#endif // _Rp_OCTAVE_HELPER_H
