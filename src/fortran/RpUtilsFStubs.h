/*
 * ----------------------------------------------------------------------
 *  INTERFACE: Fortran Rappture Utils Stub Headers
 *
 * ======================================================================
 *  AUTHOR:  Derrick Kearney, Purdue University
 *  Copyright (c) 2004-2007  Purdue Research Foundation
 *
 *  See the file "license.terms" for information on usage and
 *  redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ======================================================================
 */

#ifdef __cplusplus
    extern "C" {
#endif


/**********************************************************/
// INTERFACE FUNCTIONS - ONE UNDERSCORE

int rp_progress_ ( int* percent, char* text, int text_len );


/**********************************************************/
// INTERFACE FUNCTIONS - TWO UNDERSCORES

int rp_progress__ ( int* percent, char* text, int text_len );


/**********************************************************/
// INTERFACE STUB FUNCTIONS - ALL CAPS

int RP_PROGRESS ( int* percent, char* text, int text_len );


/**********************************************************/
/**********************************************************/

#ifdef __cplusplus
    }
#endif
